/**
 * COMP30023 Project 2
 * Author: Jonathan Jauhari 1038331
 *
 * Main program: a DNS server that accepts requests for IPv6 addresses and
 * serves them either from its own cache or by querying servers higher up
 * the hierarchy (upstream). This server operates over TCP.
 */

#define _POSIX_C_SOURCE 200112L
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "bytes.h"
#include "cache.h"
#include "cache_entry.h"
#include "dns_message.h"
#include "util.h"

#define CACHE

// maximum number of answers to cache
#define CACHE_CAPACITY 5
// maximum number of connection requests to be queued up
#define CONNECTION_QUEUE_SIZE 5
// path to the .log file to be created/written to
#define LOG_FILE_PATH "./dns_svr.log"
// TCP port to listen on
#define SERVER_PORT "8053"

int setup_client_socket(const char *server_name, const char *port);
int setup_server_socket(const char *port);
int accept_client_connection(int serv_sockfd);

dns_message_t *read_dns_message(int fd);
void write_dns_message(int fd, dns_message_t *msg);

void log_query(FILE *fp, query_t *query);
void log_unimplemented(FILE *fp);
void log_answer(FILE *fp, record_t *answer);

void log_cached(FILE *fp, cache_entry_t *entry);
void log_evicted(FILE *fp, cache_entry_t *entry, cache_entry_t *evicted);

dns_message_t *respond_from_cache(dns_message_t *msg_query,
                                  cache_entry_t *cached, FILE *log_fp);
dns_message_t *forward_message(int ups_sockfd, dns_message_t *msg_query,
                               cache_t *cache, FILE *log_fp);

// Listens for DNS "AAAA" queries over TCP on a fixed port, forwarding the
// requests and responses to/from an upstream server specified by hostname
// and port given as two command line arguments. Logs this server's events
// in a .log file.
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *ups = argv[1];
    char *ups_port = argv[2];

    cache_t *cache = new_cache(CACHE_CAPACITY);

    // Open log file, creating it if it does not exist or overwriting
    FILE *log_fp = fopen(LOG_FILE_PATH, "a");
    if (!log_fp) {
        perror("open log file");
        exit(EXIT_FAILURE);
    }

    // setup a socket for listening, and another for forwarding to upstream
    int serv_sockfd = setup_server_socket(SERVER_PORT);

    // queue up to some number of connection requests
    if (listen(serv_sockfd, CONNECTION_QUEUE_SIZE) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    dns_message_t *msg_send, *msg_reply;
    while (true) {
        int sockfd = accept_client_connection(serv_sockfd);

        // read message from client, log if necessary
        msg_send = read_dns_message(sockfd);
        if (msg_send->qdcount > 0) {
            log_query(log_fp, &msg_send->queries[0]);
        }
        // we are allowed to assume only one question per message:
        // if the one question is not for AAAA, log and respond with RCODE 4
        if (msg_send->queries[0].qtype != AAAA_RR_TYPE) {
            msg_reply = new_unimplemented_message(msg_send);
            log_unimplemented(log_fp);
        } else {
            char *qname = (char *)msg_send->queries[0].qname;
            cache_entry_t *cached = cache_get(cache, (char *)qname);
            if (cached) {
                msg_reply = respond_from_cache(msg_send, cached, log_fp);
                free_cache_entry(cached);
            } else {
                int ups_sockfd = setup_client_socket(ups, ups_port);
                msg_reply =
                    forward_message(ups_sockfd, msg_send, cache, log_fp);
                close(ups_sockfd);
            }
        }
        write_dns_message(sockfd, msg_reply);
        free_dns_message(msg_reply);

        free_dns_message(msg_send);
        close(sockfd);
    }
    close(serv_sockfd);
    fclose(log_fp);
    free_cache(cache);

    return 0;
}

// Given a message `msg_query` from the client for a resource record that is
// in the cache `cached`, return a response message and log events.
dns_message_t *respond_from_cache(dns_message_t *msg_query,
                                  cache_entry_t *cached, FILE *log_fp) {
    dns_message_t *msg_reply =
        new_response_message(msg_query, cached->record);
    log_cached(log_fp, cached);
    // spec: if first answer is not AAAA, then do not log any
    if (msg_reply->ancount > 0 &&
        msg_reply->answers[0].type == AAAA_RR_TYPE) {
        log_answer(log_fp, &msg_reply->answers[0]);
    }
    return msg_reply;
}

// Given a message `msg_query` from the client and an accepted socket
// `ups_sockfd` to an upstream server, forward the message to upstream,
// and return the reply, caching the first answer if appropriate, and logging
// events.
dns_message_t *forward_message(int ups_sockfd, dns_message_t *msg_query,
                               cache_t *cache, FILE *log_fp) {
    // forward client's request to upstream server
    write_dns_message(ups_sockfd, msg_query);
    dns_message_t *msg_reply = read_dns_message(ups_sockfd);

    cache_entry_t *evicted, *cached;
    if (msg_reply->ancount > 0) {
        record_t first_record = msg_reply->answers[0];
        // spec: if first answer is not AAAA, then do not log any
        if (first_record.type == AAAA_RR_TYPE) {
            if (first_record.ttl != 0) {
                // cache if possible, logging evictions
                evicted = cache_put(cache, &first_record);
                cached = cache_get(cache, (char *)first_record.name);
                if (evicted) {
                    log_evicted(log_fp, cached, evicted);
                    free_cache_entry(evicted);
                }
                free_cache_entry(cached);
            }
            log_answer(log_fp, &first_record);
        }
    }
    return msg_reply;
}

// This function contains code from Lab 9 solutions. Creates and returns a
// socket for this server to listen on, bound to the given port, over IPv4
// and TCP.  This function reuses the port if possible. Exits if error.
int setup_server_socket(const char *port) {
    struct addrinfo hints, *addrinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_flags = AI_PASSIVE;      // will be listening

    int status = getaddrinfo(NULL, port, &hints, &addrinfo);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // Create socket
    int sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype,
                        addrinfo->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(addrinfo);
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Reuse port if possible
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
        0) {
        freeaddrinfo(addrinfo);
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind address to the socket
    if (bind(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0) {
        freeaddrinfo(addrinfo);
        perror("bind");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(addrinfo);
    return sockfd;
}

// Accepts a client connection request queued up for the given socket, and
// returns it. Exits if error.
int accept_client_connection(int serv_sockfd) {
    struct sockaddr_storage client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_addr_size = 0;
    int sockfd = accept(serv_sockfd, (struct sockaddr *)&client_addr,
                        &client_addr_size);
    if (sockfd < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

// This function contains code from Lab 9 solutions. Creates and returns a
// connected socket for this server to communicate with an upstream server,
// with the given name and port, over IPv4 and TCP. Exits if error.
int setup_client_socket(const char *server_name, const char *port) {
    struct addrinfo hints, *addrinfo, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    int status = getaddrinfo(server_name, port, &hints, &addrinfo);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // loop through linked list of addrinfo's to create a valid,
    // connected socket
    int sockfd;
    for (rp = addrinfo; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            // error in creating socket: keep on looking
            continue;
        }
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            // success
            break;
        }
        close(sockfd);
    }
    freeaddrinfo(addrinfo);
    if (rp == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

// Reads a DNS message (including the two-byte size header for TCP) from
// file decriptor `fd`. Returns the allocated message if read succesfully,
// exits otherwise.
dns_message_t *read_dns_message(int fd) {
    uint16_t size_header;
    // read size header
    if (read(fd, &size_header, sizeof(size_header)) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    uint16_t msg_len = ntohs(size_header);
    uint8_t msg[msg_len];

    // read the entire message
    read_fully(fd, msg, msg_len);

    return init_dns_message(msg, msg_len);
}

// Writes a DNS message `msg` (including the two-byte size header for TCP) to
// file decriptor `fd`. Exits if error.
void write_dns_message(int fd, dns_message_t *msg) {
    uint16_t msg_len = msg->bytes->size;
    uint16_t size_header = htons(msg_len);

    size_t header_len = sizeof(size_header);
    size_t buf_len = header_len + msg_len;

    // write both the header and the message's bytes
    uint8_t buf[buf_len];
    memcpy(buf, &size_header, header_len);
    memcpy(buf + header_len, msg->bytes->data, msg_len);

    write_fully(fd, buf, buf_len);
}

// Print to `fp` the timestamped logs for when a query `query` is received by
// this server.
void log_query(FILE *fp, query_t *query) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    fprintf(fp, "%s requested %s\n", timestamp, query->qname);
    fflush(fp);
}

// Print to `fp` the timestamped logs for when a query is detected
// as unimplemented by this server.
void log_unimplemented(FILE *fp) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    fprintf(fp, "%s unimplemented request\n", timestamp);
    fflush(fp);
}

// Print to `fp` the timestamped logs for when an resource record `answer`
// is to be returned by this server.
void log_answer(FILE *fp, record_t *answer) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    fprintf(fp, "%s %s is at %s\n", timestamp, answer->name, answer->rdata);
    fflush(fp);
}

// Print to `fp` the timestamped logs for when an cache entry that has the
// resource record being requested is found in the cache of this server.
void log_cached(FILE *fp, cache_entry_t *entry) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    char expiry[TIMESTAMP_LEN];
    cache_entry_get_expiry(entry, expiry, TIMESTAMP_LEN);

    fprintf(fp, "%s %s expires at %s\n", timestamp, entry->record->name,
            expiry);
    fflush(fp);
}

// Print to `fp` the timestamped logs for when an cache entry that has the
// resource record being requested replaced another in this server's cache.
void log_evicted(FILE *fp, cache_entry_t *entry, cache_entry_t *evicted) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    fprintf(fp, "%s replacing %s by %s\n", timestamp, evicted->record->name,
            entry->record->name);
    fflush(fp);
}
