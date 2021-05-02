#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// inet_ntops
// TODO: ERRNO error checking

#define AAAA_RR_TYPE 28

typedef struct {
    uint16_t id;
    bool qr;
    uint8_t opcode;
    bool aa;
    bool tc;
    bool rd;
    bool ra;
    uint8_t rcode;
} dns_message_t;

uint16_t read_field(uint16_t *field);
uint8_t read_octet(uint8_t *octet);

int main(int argc, char *argv[]) {
    uint16_t size_field;
    if (read(STDIN_FILENO, &size_field, sizeof(size_field)) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    size_t nbytes = ntohs(size_field);
    printf("%lu\n", nbytes);

    dns_message_t message;
    uint16_t *field = malloc(sizeof(*field));
    message.id = read_field(field);

    uint16_t flags = read_field(field);
    message.qr = (flags >> 15) & 1;
    message.opcode = (flags >> 11) & 15;
    message.aa = (flags >> 10) & 1;
    message.tc = (flags >> 9) & 1;
    message.rd = (flags >> 8) & 1;
    message.ra = (flags >> 7) & 1;
    // 3 bits of z ignored
    message.rcode = (flags >> 0) & 15;

    uint16_t qdcount = read_field(field);
    uint16_t ancount = read_field(field);
    uint16_t nscount = read_field(field);
    uint16_t arcount = read_field(field);

    uint8_t *octet = malloc(sizeof(*octet));

    // if at least 1 query then read the first one
    if (qdcount > 1) {
        uint8_t label_size = 0;
        uint8_t *domain = malloc(nbytes);
        *domain = '\0';
        while ((label_size = read_octet(octet)) != 0) {
            int i;
            uint8_t *label = malloc(label_size * sizeof(*label) + 1);
            for (i = 0; i < label_size; i++) {
                label[i] = read_octet(octet);
            }
            label[label_size] = '\0';

            if (strlen((char *)domain) != 0) {
                strcat((char *)domain, ".");
            }
            strncat((char *)domain, (char *)label, label_size);
        }
        uint16_t qtype = read_field(field);
        uint16_t qclass = read_field(field);
        if (qtype != AAAA_RR_TYPE) {
            // do something or nothing
        }
        free(domain);
    }

    free(octet);
    free(field);

    // // assume 1 query
    // uint8_t octet;
    // uint8_t label_len;
    // uint8_t *label_bytes;
    // uint8_t *domain = malloc(nbytes);
    // size_t domain_len;

    // memcpy(&octet, bytes + i, sizeof(octet));
    // label_len = octet;
    // i += sizeof(octet);

    // // while (label_len != 0) {

    // // }
    // label_bytes = malloc(label_len * sizeof(*label_bytes) + 1);
    // strncpy((char *)label_bytes, (char *)(bytes + i),
    //         label_len * sizeof(*label_bytes));
    // label_bytes[label_len] = '\0';
    // // memcpy(label_bytes, bytes + i, label_len * sizeof(*label_bytes));
    // i += label_len * sizeof(*label_bytes);

    // strncat((char *)domain, (char *)label_bytes, label_len);

    // free(label_bytes);

    // memcpy(&octet, bytes + i, sizeof(octet));
    // label_len = octet;
    // i += sizeof(octet);

    // label_bytes = malloc(label_len * sizeof(*label_bytes) + 1);
    // // memcpy(label_bytes, bytes + i, label_len * sizeof(*label_bytes));
    // strncpy((char *)label_bytes, (char *)(bytes + i),
    //         label_len * sizeof(*label_bytes));
    // label_bytes[label_len] = '\0';
    // i += label_len * sizeof(*label_bytes);

    // strncat((char *)domain, ".", 1);
    // strncat((char *)domain, (char *)label_bytes, label_len);

    // free(label_bytes);

    // free(bytes);

    return 0;
}

uint16_t read_field(uint16_t *field) {
    if (read(STDIN_FILENO, field, sizeof(*field)) == -1) {
        free(field);
        perror("read");
        exit(EXIT_FAILURE);
    }
    return ntohs(*field);
}

uint8_t read_octet(uint8_t *octet) {
    if (read(STDIN_FILENO, octet, sizeof(*octet)) == -1) {
        free(octet);
        perror("read");
        exit(EXIT_FAILURE);
    }
    return *octet;
}

// int i = 0;
// uint16_t field;

// memcpy(&field, bytes + i, sizeof(field));
// uint16_t id = ntohs(field);
// i += sizeof(field);

// memcpy(&field, bytes + i, sizeof(field));
// uint16_t flags = ntohs(field);
// i += sizeof(field);

// bool qr = (flags >> 15) & 1;
// uint8_t opcode = (flags >> 11) & 15;
// bool aa = (flags >> 10) & 1;
// bool tc = (flags >> 9) & 1;
// bool rd = (flags >> 8) & 1;
// bool ra = (flags >> 7) & 1;
// // z ignored
// uint8_t rcode = (flags >> 0) & 15;

// memcpy(&field, bytes + i, sizeof(field));
// uint16_t qdcount = ntohs(field);
// i += sizeof(field);

// memcpy(&field, bytes + i, sizeof(field));
// uint16_t ancount = ntohs(field);
// i += sizeof(field);

// memcpy(&field, bytes + i, sizeof(field));
// uint16_t nscount = ntohs(field);
// i += sizeof(field);

// memcpy(&field, bytes + i, sizeof(field));
// uint16_t arcount = ntohs(field);
// i += sizeof(field);