/**
 * COMP30023 Project 2
 * Author: Jonathan Jauhari 1038331
 * 
 * DNS message module containing functions for creating (partial DNS
 * messages), parsing from bytes and interpreting them.
 */

#include "dns_message.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// bitmasks for reading flags/codes in the second 2-byte field of the message
#define QR_MASK (1 << QR_OFFSET)
#define OPCODE_MASK (0xF << OPCODE_OFFSET)
#define AA_MASK (1 << AA_OFFSET)
#define TC_MASK (1 << TC_OFFSET)
#define RD_MASK (1 << RD_OFFSET)
#define RA_MASK (1 << RA_OFFSET)
#define RCODE_MASK (0xF << RCODE_OFFSET)

// bit positions to shift right, used in reading these codes in the message
#define OPCODE_LOFFSET 11
#define RCODE_LOFFSET 0

uint16_t read_field(uint16_t *field, bytes_t *bytes);
uint8_t read_octet(uint8_t *octet, bytes_t *bytes);

uint8_t *read_domain(uint8_t *domain, bytes_t *bytes);
char *read_ip_addr(char *addr, uint16_t rdlen, bytes_t *bytes);

void read_header(dns_message_t *msg);
void read_queries(dns_message_t *msg);
void read_answers(dns_message_t *msg);

// Allocates a dns_message, based on the length of the message in bytes
// `nbytes`, and returns it *uninitialised*. The queries and answer section
// are not allocated, (they will be allocated at initialisation).
dns_message_t *new_dns_message(uint16_t nbytes) {
    dns_message_t *msg = malloc(sizeof(*msg));
    bytes_t *bytes = malloc(sizeof(*bytes));
    uint8_t *data = malloc(nbytes * sizeof(*data));
    assert(msg && bytes && data);

    bytes->data = data;
    bytes->size = nbytes;
    bytes->offset = 0;

    msg->bytes = bytes;
    msg->queries = NULL;
    msg->answers = NULL;

    return msg;
}

// Creates and initialises a dns_message then returns it, filling in the
// header, question and answers section and nothing more, from bytes `data`
// of length `nbytes`.
dns_message_t *init_dns_message(uint8_t *data, uint16_t nbytes) {
    dns_message_t *msg = new_dns_message(nbytes);
    memcpy(msg->bytes->data, data, nbytes);
    msg->bytes->size = nbytes;
    msg->bytes->offset = 0;

    read_header(msg);
    read_queries(msg);
    read_answers(msg);

    return msg;
}

// Frees a dns_message: make sure it was initialised before calling this.
void free_dns_message(dns_message_t *msg) {
    free(msg->bytes->data);
    free(msg->bytes);

    for (size_t i = 0; i < msg->qdcount; i++) {
        free(msg->queries[i].qname);
    }
    free(msg->queries);

    for (size_t i = 0; i < msg->ancount; i++) {
        free(msg->answers[i].name);
        free(msg->answers[i].rdata);
    }
    free(msg->answers);

    free(msg);
}

// Copies two octets from `bytes` to `field`, keeping track of the offset
// and returns the integer value in host byte order.
uint16_t read_field(uint16_t *field, bytes_t *bytes) {
    memcpy(field, bytes->data + bytes->offset, sizeof(*field));
    bytes->offset += sizeof(*field);
    *field = ntohs(*field);
    return *field;
}

// Copies an octet from `bytes` to `field`, keeping track of the offset and
// returns the integer value of the octet read.
uint8_t read_octet(uint8_t *octet, bytes_t *bytes) {
    memcpy(octet, bytes->data + bytes->offset, sizeof(*octet));
    bytes->offset += sizeof(*octet);
    return *octet;
}

// Read a domain (a sequence of labels separated by '.') from `bytes` into a
// null-terminated array of octets `domain` which can be treated as a string,
// since we are allowed to assume domain names are ASCII only. Returns a
// pointer to `domain`.
uint8_t *read_domain(uint8_t *domain, bytes_t *bytes) {
    uint8_t octet;
    uint8_t label_size = 0;
    while ((label_size = read_octet(&octet, bytes)) != 0) {
        int i;
        uint8_t label[label_size + 1];
        for (i = 0; i < label_size; i++) {
            label[i] = read_octet(&octet, bytes);
        }
        label[label_size] = '\0';

        if (strlen((char *)domain) != 0) {
            strcat((char *)domain, ".");
        }
        strncat((char *)domain, (char *)label, label_size);
    }
    return domain;
}

// Read an IPv6 IP address from `rdlen` bytes of `bytes` into a string `addr`,
// returning a pointer to `addr`. Conversion from binary network format to
// presentation form is done by `inet_ntop()`.
char *read_ip_addr(char *addr, uint16_t rdlen, bytes_t *bytes) {
    inet_ntop(AF_INET6, bytes->data + bytes->offset, addr, INET6_ADDRSTRLEN);
    bytes->offset += rdlen * sizeof(*addr);
    return addr;
}

// Set the header fields in `msg`, based on the bytes array that it contains
void read_header(dns_message_t *msg) {
    bytes_t *bytes = msg->bytes;

    read_field(&msg->id, bytes);

    uint16_t flags;
    read_field(&flags, bytes);
    // // TODO: consider #define'ing
    // msg->qr = (flags >> 15) & 1;        // leftmost bit is QR
    // msg->opcode = (flags >> 11) & 0xF;  // next 4 bits is OPCODE
    // msg->aa = (flags >> 10) & 1;        // then 1 bit for AA
    // msg->tc = (flags >> 9) & 1;         // then 1 bit for TC
    // msg->rd = (flags >> 8) & 1;         // then 1 bit for RD
    // msg->ra = (flags >> 7) & 1;         // then 1 bit for RA
    // // 3 bits of z is ignored
    // msg->rcode = (flags >> 0) & 0xF;    // rightmost 4 bits is RCODE

    // 1bit QR, 4bits OPCODE, 1bit for each of AA, TC, RD, RA, then 4 bits
    // for RCODE.
    msg->qr = (flags & QR_MASK) >> QR_OFFSET;
    msg->opcode = (flags & OPCODE_MASK) >> OPCODE_LOFFSET;
    msg->aa = (flags & AA_MASK) >> AA_OFFSET;     
    msg->tc = (flags & TC_MASK) >> TC_OFFSET;     
    msg->rd = (flags & RD_MASK) >> RD_OFFSET;     
    msg->ra = (flags & RA_MASK) >> RA_OFFSET;         
    // 3 bits of z is ignored
    msg->rcode = (flags >> RCODE_MASK) >> RCODE_LOFFSET;

    read_field(&msg->qdcount, bytes);
    read_field(&msg->ancount, bytes);
    read_field(&msg->nscount, bytes);
    read_field(&msg->arcount, bytes);
}

// Set the queries field in `msg`, based on the bytes array that it contains.
// This function allocates the queries in `msg`.
void read_queries(dns_message_t *msg) {
    bytes_t *bytes = msg->bytes;
    query_t *queries = malloc(msg->qdcount * sizeof(*queries));
    assert(queries);

    for (size_t i = 0; i < msg->qdcount; i++) {
        query_t query;
        query.qname = malloc(bytes->size * sizeof(*query.qname));
        assert(query.qname);
        *query.qname = '\0';

        read_domain(query.qname, bytes);
        read_field(&query.qtype, bytes);
        read_field(&query.qclass, bytes);

        queries[i] = query;
    }
    msg->queries = queries;
}

// Set the queries field in `msg`, based on the bytes array that it contains
// This function allocates the answers in `msg`.
void read_answers(dns_message_t *msg) {
    bytes_t *bytes = msg->bytes;
    record_t *answers = malloc(msg->ancount * sizeof(*answers));
    assert(answers);

    for (size_t i = 0; i < msg->ancount; i++) {
        record_t answer;
        uint16_t name_offset = 0;
        read_field(&name_offset, bytes);

        // check that first (leftmost) two bits in 16-bit field are 0b11.
        assert(((name_offset >> 14) ^ 0x3) == 0);

        // clear first two bits (16th and 15th), this is the offset needed
        name_offset &= ~((1 << 15) | (1 << 14));

        // save the old offset before temporarily setting the name offset
        // to read the domain name, then resetting back to the old offset.
        uint16_t old_offset = bytes->offset;
        bytes->offset = name_offset;
        answer.name = malloc(bytes->size * sizeof(*answer.name));
        assert(answer.name);
        *answer.name = '\0';
        read_domain(answer.name, bytes);
        bytes->offset = old_offset;

        read_field(&answer.type, bytes);
        read_field(&answer.class, bytes);

        // read 2 16-bit pieces and get the 32-bit TTL
        uint16_t field1, field2;
        read_field(&field1, bytes);
        read_field(&field2, bytes);
        answer.ttl = field1 << 0x10 | field2;

        read_field(&answer.rdlen, bytes);

        char *addr = malloc(INET6_ADDRSTRLEN);
        assert(addr);
        read_ip_addr(addr, answer.rdlen, bytes);
        answer.rdata = addr;

        answers[i] = answer;
    }
    msg->answers = answers;
}

// Return the integer value of the 2nd 2-byte field in the binary
// representation of `msg`.
uint16_t get_flags(dns_message_t *msg) {
    uint16_t flags = 0;
    flags |= msg->qr << QR_OFFSET;
    flags |= msg->opcode << OPCODE_OFFSET;
    flags |= msg->aa << AA_OFFSET;
    flags |= msg->tc << TC_OFFSET;
    flags |= msg->rd << RD_OFFSET;
    flags |= msg->ra << RA_OFFSET;
    flags |= msg->rcode << RCODE_OFFSET;
    return flags;
}
