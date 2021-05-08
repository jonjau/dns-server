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

// TODO: ERRNO error checking, assert mallocs
// TODO: remove debug printf's

#define AAAA_RR_TYPE 28
#define TIMESTAMP_LEN 41  // based on reasonable ISO 8601 limits
#define LOG_FILE_PATH "./dns_svr.log"

char *get_timestamp(char *timestamp, size_t len);
uint16_t read_msg_len(int fd);
void log_query(FILE *fp, query_t *query);
void log_answer(FILE *fp, record_t *answer);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    size_t nbytes = read_msg_len(STDIN_FILENO);
    dns_message_t *msg = init_dns_message(STDIN_FILENO, nbytes);
    FILE *fp = fopen(LOG_FILE_PATH, "w");
    if (!fp) {
        perror("read");
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
    char *timestamp = malloc(TIMESTAMP_LEN);
    get_timestamp(timestamp, TIMESTAMP_LEN);

    if (query->qtype == AAAA_RR_TYPE) {
        fprintf(fp, "%s requested %s\n", timestamp, query->qname);
    } else {
        fprintf(fp, "%s unimplemented request\n", timestamp);
    }

    // fflush();
    free(timestamp);
}

void log_answer(FILE *fp, record_t *answer) {
    char *timestamp = malloc(TIMESTAMP_LEN);
    get_timestamp(timestamp, TIMESTAMP_LEN);

    // TODO: necessary check?
    if (answer->type == AAAA_RR_TYPE) {
        fprintf(fp, "%s %s is at %s\n", timestamp, answer->name,
                answer->rdata);
    }
    free(timestamp);
}