#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "dns_message.h"
#include "util.h"

#define AAAA_RR_TYPE 28
#define TIMESTAMP_LEN 41  // based on reasonable ISO 8601 limits
#define LOG_FILE_PATH "./dns_svr.log"

uint16_t read_msg_len(int fd);
void log_query(FILE *fp, query_t *query);
void log_answer(FILE *fp, record_t *answer);

int main(int argc, char *argv[]) {
    if (argc < 2) {
		fprintf(stderr, "usage %s [query|response]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

    uint16_t msg_len = read_msg_len(STDIN_FILENO);
    uint8_t buf[msg_len];
    if (read(STDIN_FILENO, buf, msg_len) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    dns_message_t *msg = init_dns_message(buf, msg_len);

    FILE *fp = fopen(LOG_FILE_PATH, "w");
    if (!fp) {
        perror("open log file");
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[1], "query") == 0 && msg->qdcount > 0) {
        log_query(fp, &msg->queries[0]);
    } else if (strcmp(argv[1], "response") == 0 && msg->ancount > 0) {
        log_answer(fp, &msg->answers[0]);
    }

    // no need to parse or log authority/additional records
    fclose(fp);
    free_dns_message(msg);

    return 0;
}

// Read and returns the message length, read from `fd`.
uint16_t read_msg_len(int fd) {
    uint16_t field;
    if (read(fd, &field, sizeof(field)) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    return ntohs(field);
}

// Print to `fp` the timestamped logs for when a query `query` is received by
// this server
void log_query(FILE *fp, query_t *query) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    fprintf(fp, "%s requested %s\n", timestamp, query->qname);
    fflush(fp);
    if (query->qtype != AAAA_RR_TYPE) {
        fprintf(fp, "%s unimplemented request\n", timestamp);
        fflush(fp);
    }
}

// Print to `fd` the timestamped logs for when an resource record `answer`
// is to be returned by this server.
void log_answer(FILE *fp, record_t *answer) {
    char timestamp[TIMESTAMP_LEN];
    get_timestamp(timestamp, TIMESTAMP_LEN);

    // just in case, ensure answer is IPv6
    if (answer->type == AAAA_RR_TYPE) {
        fprintf(fp, "%s %s is at %s\n", timestamp, answer->name,
                answer->rdata);
        fflush(fp);
    }
}