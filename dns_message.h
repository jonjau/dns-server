#ifndef DNS_MESSAGE_H
#define DNS_MESSAGE_H

#include <stdint.h>
#include <stdbool.h>

#define QR_POS 15
#define OPCODE_POS 14
#define AA_POS 10
#define TC_POS 9
#define RD_POS 8
#define RA_POS 7
#define RCODE_POS 0

#define AAAA_RR_TYPE 28
#define NOT_IMPLEMENTED_RCODE 4

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
    uint8_t *data;
    uint16_t size;
    uint16_t offset;
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
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
    query_t *queries;
    record_t *answers;
    bytes_t *bytes;
} dns_message_t;

dns_message_t *new_dns_message(uint16_t nbytes);
dns_message_t *init_dns_message(uint8_t *data,  uint16_t nbytes);
uint16_t get_flags(dns_message_t *msg);

void free_dns_message(dns_message_t *msg);

#endif
