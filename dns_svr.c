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

// inet_ntops
// TODO: ERRNO error checking, assert mallocs

#define AAAA_RR_TYPE 28
#define TIMESTAMP_LEN 41  // based on reasonable ISO 8601 limits

typedef struct {
    uint8_t *data;
    size_t size;
    size_t offset;
} bytes_t;

typedef struct {
    uint16_t id;
    bool qr;
    uint8_t opcode;
    bool aa;
    bool tc;
    bool rd;
    bool ra;
    uint8_t rcode;
    bytes_t *bytes;
} dns_message_t;

uint16_t read_field(uint16_t *field, bytes_t *bytes);
uint8_t read_octet(uint8_t *octet, bytes_t *bytes);

void read_domain(uint8_t *domain, bytes_t *bytes);
void read_ip_addr(char *addr, size_t rdlen, bytes_t *bytes);

void get_timestamp(char *timestamp, size_t len);

int main(int argc, char *argv[]) {
    uint16_t *field = malloc(sizeof(*field));
    if (read(STDIN_FILENO, field, sizeof(*field)) == -1) {
        free(field);
        perror("read");
        exit(EXIT_FAILURE);
    }
    size_t nbytes = ntohs(*field);

    // bytes_t *bytes = malloc(nbytes * sizeof(*bytes));
    bytes_t *bytes = malloc(sizeof(*bytes));
    // uint8_t *data = malloc(nbytes * sizeof(*data));
    uint8_t data[nbytes];
    if (read(STDIN_FILENO, data, nbytes * sizeof(*data)) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    bytes->data = data;
    bytes->size = nbytes;
    bytes->offset = 0;

    dns_message_t message;
    message.id = read_field(field, bytes);

    uint16_t flags = read_field(field, bytes);
    // TODO: consider shift and shift to read
    message.qr = (flags >> 15) & 1;
    message.opcode = (flags >> 11) & 0xF;
    message.aa = (flags >> 10) & 1;
    message.tc = (flags >> 9) & 1;
    message.rd = (flags >> 8) & 1;
    message.ra = (flags >> 7) & 1;
    // 3 bits of z ignored
    message.rcode = (flags >> 0) & 0xF;

    uint16_t qdcount = read_field(field, bytes);
    uint16_t ancount = read_field(field, bytes);
    uint16_t nscount = read_field(field, bytes);
    uint16_t arcount = read_field(field, bytes);

    uint8_t *octet = malloc(sizeof(*octet));

    // if at least 1 query then read the first one
    if (qdcount > 0) {
        uint8_t *domain = malloc(bytes->size);
        *domain = '\0';
        read_domain(domain, bytes);

        uint16_t qtype = read_field(field, bytes);
        uint16_t qclass = read_field(field, bytes);

        char *timestamp = malloc(TIMESTAMP_LEN);
        get_timestamp(timestamp, TIMESTAMP_LEN);

        if (qtype == AAAA_RR_TYPE) {
            printf("%s requested %s\n", timestamp, domain);
        } else {
            printf("%s unimplemented request\n", timestamp);
        }

        free(timestamp);
        free(domain);
    }

    if (ancount > 0) {
        uint16_t name = read_field(field, bytes);
        // check that first two bits in 16-bit field are 0b11.
        assert(((name >> 14) ^ 0x3) == 0);

        // clear first two bits (16th and 15th)
        name &= ~((1 << 15) | (1 << 14));

        size_t old_offset = bytes->offset;
        bytes->offset = name;
        uint8_t *domain = malloc(nbytes * sizeof(*domain));
        read_domain(domain, bytes);
        bytes->offset = old_offset;

        uint16_t type = read_field(field, bytes);
        uint16_t class = read_field(field, bytes);

        uint32_t ttl = read_field(field, bytes) << 0xffffffff | read_field(field, bytes);

        uint16_t rdlen = read_field(field, bytes);
        char *addr = malloc(INET6_ADDRSTRLEN);
        read_ip_addr(addr, rdlen, bytes);

        char *timestamp = malloc(TIMESTAMP_LEN);
        get_timestamp(timestamp, TIMESTAMP_LEN);

        printf("%s %s is at %s\n", timestamp, domain, addr);

        free(timestamp);
        free(addr);
        free(domain);
    }

    // no need to parse or log authority/additional records

    free(octet);
    free(field);

    return 0;
}

uint16_t read_field(uint16_t *field, bytes_t *bytes) {
    memcpy(field, bytes->data + bytes->offset, sizeof(*field));
    bytes->offset += sizeof(*field);
    return ntohs(*field);
}

uint8_t read_octet(uint8_t *octet, bytes_t *bytes) {
    memcpy(octet, bytes->data + bytes->offset, sizeof(*octet));
    bytes->offset += sizeof(*octet);
    return *octet;
}

void read_domain(uint8_t *domain, bytes_t *bytes) {
    uint8_t *octet = malloc(sizeof(*octet));
    uint8_t label_size = 0;
    while ((label_size = read_octet(octet, bytes)) != 0) {
        int i;
        uint8_t *label = malloc(label_size * sizeof(*label) + 1);
        for (i = 0; i < label_size; i++) {
            label[i] = read_octet(octet, bytes);
        }
        label[label_size] = '\0';

        if (strlen((char *)domain) != 0) {
            strcat((char *)domain, ".");
        }
        strncat((char *)domain, (char *)label, label_size);
        free(label);
    }
    free(octet);
}

void read_ip_addr(char *addr, size_t rdlen, bytes_t *bytes) {
    inet_ntop(AF_INET6, bytes->data + bytes->offset, addr, INET6_ADDRSTRLEN);
    bytes->offset += rdlen * sizeof(*addr);
}

void get_timestamp(char *timestamp, size_t len) {
    const time_t rawtime = time(NULL);
    struct tm *tm = gmtime(&rawtime);

    strftime(timestamp, len, "%FT%T%z", tm);
}