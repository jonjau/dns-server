/**
 * COMP30023 Project 2
 * Author: Jonathan Jauhari 1038331
 * 
 * DNS message module containing functions for creating (partial DNS
 * messages), parsing from bytes and interpreting them.
 */

#ifndef DNS_MESSAGE_H
#define DNS_MESSAGE_H

#include <stdint.h>
#include <stdbool.h>

#include "bytes.h"

// resource record type designating AAAA or IPv6
#define AAAA_RR_TYPE 28

// Represents a 'question' in the questions section of a DNS message
typedef struct {
    uint16_t qtype;
    uint16_t qclass;
    uint8_t *qname;
} query_t;

// Represents a 'resource record' in the answers section of a DNS message
typedef struct {
    uint8_t *name;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlen;
    char *rdata;
} record_t;

// Represents a (partial) DNS message. The 'Authority' and 'Additional'
// sections are omitted. This structure also contains its representation in
// bytes, in a `bytes_t`. The order of the fields is significant, as their
// offset is used when setting certain fields by the DNS server implementation
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

void free_dns_message(dns_message_t *msg);

dns_message_t *new_unimplemented_message(dns_message_t *msg);
dns_message_t *new_response_message(dns_message_t *msg, record_t *record);

#endif
