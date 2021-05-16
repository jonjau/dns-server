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

#include "cache.h"
#include "cache_entry.h"
#include "dns_message.h"
#include "util.h"

#define CACHE

#define CACHE_CAPACITY 2

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

void handle_unimplemented(int sockfd, dns_message_t *msg);
dns_message_t *get_cached(dns_message_t *msg, cache_entry_t *cached);

// FIXME: respond with RCODE 4 AND NOTHING ELSE, CONSIDER STARTING FROM
// SCRATCH?
// TODO: malloc size problem???

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

    dns_message_t *msg_send, *msg_recv;
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
            log_unimplemented(log_fp);
            handle_unimplemented(sockfd, msg_send);
        } else {
            char *qname = (char *)msg_send->queries[0].qname;
            cache_entry_t *cached = cache_get(cache, (char *)qname);
            if (cached) {
                log_cached(log_fp, cached);
                msg_recv = get_cached(msg_send, cached);
                if (msg_recv->ancount > 0) {
                    record_t first_record = msg_recv->answers[0];
                    // spec: if first answer is not AAAA, then do not log any
                    if (first_record.type == AAAA_RR_TYPE) {
                        log_answer(log_fp, &first_record);
                    }
                }
                free_cache_entry(cached);
            } else {
                int ups_sockfd = setup_client_socket(ups, ups_port);

                // forward client's request to upstream server
                write_dns_message(ups_sockfd, msg_send);

                // wait for reply then log and forward the response to the
                // client
                msg_recv = read_dns_message(ups_sockfd);

                if (msg_recv->ancount > 0) {
                    record_t first_record = msg_recv->answers[0];

                    // spec: if first answer is not AAAA, then do not log any
                    if (first_record.type == AAAA_RR_TYPE) {
                        log_answer(log_fp, &first_record);

                        cache_entry_t *evicted =
                            cache_put(cache, &first_record);
                        cache_entry_t *cached =
                            cache_get(cache, (char *)first_record.name);
                        if (evicted) {
                            log_evicted(log_fp, cached, evicted);
                            free_cache_entry(evicted);
                        }
                        free_cache_entry(cached);
                    }
                }

                close(ups_sockfd);
            }

            write_dns_message(sockfd, msg_recv);
            free_dns_message(msg_recv);
        }
        free_dns_message(msg_send);
        close(sockfd);
    }
    close(serv_sockfd);
    fclose(log_fp);
    free_cache(cache);

    return 0;
}

// Given an accepted socket `sockfd`, and a message `msg` that contains a
// non-AAAA query, write back a message to the client, responding with RCODE
// NOT_IMPLEMENTED, deep copying the request `msg` to form a reply. Exits if
// error.
void handle_unimplemented(int sockfd, dns_message_t *msg) {
    dns_message_t *reply =
        init_dns_message(msg->bytes->data, msg->bytes->size);
    uint16_t flags = get_flags(reply);

    // Respond (QR=1) with RA = true, RCODE = NOT_IMPLEMENTED
    reply->ra = true;
    flags |= reply->ra << RA_OFFSET;
    reply->rcode = NOT_IMPLEMENTED_RCODE;
    flags |= reply->rcode << RCODE_OFFSET;
    reply->qr = true;
    flags |= reply->qr << QR_OFFSET;

    // update the raw bytes to reflect these changes, then write to client
    flags = htons(flags);
    uint16_t offset = offsetof(dns_message_t, qr);
    memcpy(reply->bytes->data + offset, &flags, sizeof(flags));

    write_dns_message(sockfd, reply);
    free_dns_message(reply);
}

void write_field(bytes_t *bytes, uint16_t field) {
    uint16_t field_val = htons(field);
    memcpy(bytes->data + bytes->offset, &field_val, sizeof(field));
    bytes->offset += sizeof(field);
}

void write32(bytes_t *bytes, uint32_t field) {
    uint32_t field_val = htonl(field);
    memcpy(bytes->data + bytes->offset, &field_val, sizeof(field));
    bytes->offset += sizeof(field);
}

dns_message_t *get_cached(dns_message_t *msg, cache_entry_t *cached) {
    // TODO: change id, ttl, ancount, bytes length

    size_t offset0 = msg->bytes->offset;

    bytes_t *bytes = malloc(sizeof(*bytes));
    uint16_t new_size = msg->bytes->size + 28;
    uint8_t *data = malloc(new_size);
    bytes->data = data;
    bytes->offset = 0;
    bytes->size = new_size;

    write_field(bytes, msg->id);

    uint16_t flags = get_flags(msg);
    flags |= true << RA_OFFSET;
    flags |= true << QR_OFFSET;
    write_field(bytes, flags);

    write_field(bytes, msg->qdcount);
    write_field(bytes, 1);
    write_field(bytes, msg->nscount);
    write_field(bytes, msg->arcount);

    uint16_t qlen = offset0 - bytes->offset;
    memcpy(bytes->data + bytes->offset, msg->bytes->data + bytes->offset,
           qlen);
    bytes->offset += qlen;

    record_t *record = cached->record;
    write_field(bytes, 0xc00c);
    write_field(bytes, record->type);
    write_field(bytes, record->class);
    write32(bytes, record->ttl);
    uint16_t rdlen = sizeof(struct in6_addr);
    write_field(bytes, rdlen);

    // unsigned char buf[sizeof(struct in6_addr)];

    // char test[50];
    // const char *a = (const char *)record->rdata;
    // int s = inet_pton(AF_INET6, a, buf);
    // inet_ntop(AF_INET6, buf, test, 50);

    // const char *b = "1:0:0:0:0:0:0:8";
    // s = inet_pton(AF_INET6, b, buf);
    // inet_ntop(AF_INET6, buf, test, 50);

    // uint8_t *asdasd = bytes->data + bytes->offset;
    inet_pton(AF_INET6, record->rdata, bytes->data + bytes->offset);
    bytes->offset += rdlen;
    // uint8_t *asdad = bytes->data + bytes->offset;

    uint16_t rest_len = msg->bytes->size - msg->bytes->offset;
    memcpy(bytes->data + bytes->offset,
           msg->bytes->data + msg->bytes->offset, rest_len);

    dns_message_t *reply = init_dns_message(bytes->data, bytes->size);

    free(bytes);
    return reply;
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

void log_cached(FILE *fp, cache_entry_t *entry) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    char expiry[TIMESTAMP_LEN];
    cache_entry_get_expiry(entry, expiry, TIMESTAMP_LEN);

    fprintf(fp, "%s %s expires at %s\n", timestamp, entry->record->name,
            expiry);
    fflush(fp);
}

void log_evicted(FILE *fp, cache_entry_t *entry, cache_entry_t *evicted) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    fprintf(fp, "%s replacing %s by %s\n", timestamp, evicted->record->name,
            entry->record->name);
    fflush(fp);
}
