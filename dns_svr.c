#define _POSIX_C_SOURCE 200112L
#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>

#include "dns_message.h"

// inet_ntops
// TODO: ERRNO error checking, assert mallocs
// TODO: remove debug printf's

#define AAAA_RR_TYPE 28
#define TIMESTAMP_LEN 41  // based on reasonable ISO 8601 limits
#define LOG_FILE_PATH "./dns_svr.log"
#define TCP_PORT "8053"

char *get_timestamp(char *timestamp, size_t len);
uint16_t read_msg_len(int fd);
void log_query(FILE *fp, query_t *query);
void log_answer(FILE *fp, record_t *answer);

// 172.20.96.1:53
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *ups = argv[1];
    char *ups_port = argv[2];

    // adapted from lab 9 solutions
    struct addrinfo hints, *rp, *ups_addrinfo;

    // Create address
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    int status = getaddrinfo("8.8.8.8", "53", &hints, &ups_addrinfo);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // Reuse port if possible
    // int enable = 1;
    // if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))
    // < 0) { perror("setsockopt"); exit(1);
    // }

    int ups_sockfd;
    for (rp = ups_addrinfo; rp != NULL; rp = rp->ai_next) {
        ups_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (ups_sockfd == -1) {
            // keep on looking
            continue;
        }
        if (connect(ups_sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            // success
            perror("asdas");
            break;
        }
        close(ups_sockfd);
    }
    if (rp == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(EXIT_FAILURE);
    }

    size_t nbytes = read_msg_len(STDIN_FILENO);
    dns_message_t *msg = init_dns_message(STDIN_FILENO, nbytes);

    // FILE *fp = fopen(LOG_FILE_PATH, "w");
    FILE *fp = stdout;
    if (!fp) {
        perror("open log file");
        exit(EXIT_FAILURE);
    }

    int n = write(ups_sockfd, msg->bytes->data, msg->bytes->size);
    if (n < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    uint8_t raw[999];
    n = read(ups_sockfd, raw, msg->bytes->size);
    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    printf("%d %d %d\n", raw[0], raw[1], raw[2]);

    // no need to parse or log authority/additional records
    fclose(fp);
    free_dns_message(msg);

    close(ups_sockfd);
    freeaddrinfo(ups_addrinfo);

    return 0;
}

uint16_t read_msg_len(int fd) {
    uint16_t field;
    if (read(STDIN_FILENO, &field, sizeof(field)) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    return ntohs(field);
}

char *get_timestamp(char *timestamp, size_t len) {
    const time_t rawtime = time(NULL);
    struct tm *tm = gmtime(&rawtime);

    strftime(timestamp, len, "%FT%T%z", tm);
    return timestamp;
}

void log_query(FILE *fp, query_t *query) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    if (query->qtype == AAAA_RR_TYPE) {
        fprintf(fp, "%s requested %s\n", timestamp, query->qname);
    } else {
        fprintf(fp, "%s unimplemented request\n", timestamp);
    }
}

void log_answer(FILE *fp, record_t *answer) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    // TODO: necessary check?
    if (answer->type == AAAA_RR_TYPE) {
        fprintf(fp, "%s %s is at %s\n", timestamp, answer->name,
                answer->rdata);
    }
}

/*


    // Create socket
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                    servinfo->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind address to the socket
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo);

    // Listen on socket - means we're ready to accept connections,
    // incoming connection requests will be queued, man 3 listen
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (true) {
        // accept connection, we don't care from where
        int newsockfd = accept(sockfd, NULL, NULL);
        if (newsockfd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        size_t nbytes = read_msg_len(newsockfd);
        dns_message_t *msg = init_dns_message(newsockfd, nbytes);

        int status2 = getaddrinfo(dst, dst_port, &hints, &upstreaminfo);
        if (status2 != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status2));
            exit(EXIT_FAILURE);
        }

        for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
            upstreamsockfd = socket(rp->ai_family, rp->ai_socktype,
   rp->ai_protocol); if (sockfd == -1) { continue;
            }
            if (connect(upstreamsockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
                break;  // success
            }
            close(sockfd);
        }
        if (rp == NULL) {
            fprintf(stderr, "failed to connect\n");
            exit(EXIT_FAILURE);
        }
        freeaddrinfo(servinfo);

        // TODO: what if we can't write everything?
        int n = write(upstreamsockfd, msg->bytes->data, msg->bytes->size);

        free_dns_message(msg);
    }

*/