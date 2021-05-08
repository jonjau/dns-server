#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "dns_message.h"

// inet_ntops
// TODO: ERRNO error checking, assert mallocs

#define AAAA_RR_TYPE 28
#define TIMESTAMP_LEN 41  // based on reasonable ISO 8601 limits

char *get_timestamp(char *timestamp, size_t len);
uint16_t read_msg_len(int fd);

int main(int argc, char *argv[]) {
    size_t nbytes = read_msg_len(STDIN_FILENO);
    dns_message_t *msg = init_dns_message(STDIN_FILENO, nbytes);
    free_dns_message(msg);

    // if at least 1 query then read the first one

    // no need to parse or log authority/additional records

    // char *timestamp = malloc(TIMESTAMP_LEN);
    // get_timestamp(timestamp, TIMESTAMP_LEN);

    // if (qtype == AAAA_RR_TYPE) {
    //     printf("%s requested %s\n", timestamp, domain);
    // } else {
    //     printf("%s unimplemented request\n", timestamp);
    // }

    // char *timestamp = malloc(TIMESTAMP_LEN);
    // get_timestamp(timestamp, TIMESTAMP_LEN);

    // printf("%s %s is at %s\n", timestamp, domain, addr);

    // free(timestamp);
    // free(addr);
    // free(domain);

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

void log_query(query_t *query) {
    // char *timestamp = malloc(TIMESTAMP_LEN);
    // get_timestamp(timestamp, TIMESTAMP_LEN);
}