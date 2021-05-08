#ifndef DNS_MESSAGE_H
#define DNS_MESSAGE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;
    size_t size;
    size_t offset;
} bytes_t;

typedef struct {
    uint16_t qtype;
    uint16_t qclass;
    uint8_t *qname;
} query_t;

typedef struct {
    uint8_t *name;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlen;
    char *rdata;
} record_t;

typedef struct {
    bytes_t *bytes;
    uint16_t id;
    bool qr;
    uint8_t opcode;
    bool aa;
    bool tc;
    bool rd;
    bool ra;
    uint8_t rcode;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
    query_t *queries;
    record_t *answers;
} dns_message_t;

dns_message_t *new_dns_message(size_t nbytes);
dns_message_t *init_dns_message(int fd, size_t nbytes);

void free_dns_message(dns_message_t *msg);

#endif
