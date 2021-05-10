#include "dns_message.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint16_t read_field(uint16_t *field, bytes_t *bytes);
uint8_t read_octet(uint8_t *octet, bytes_t *bytes);
uint8_t *read_domain(uint8_t *domain, bytes_t *bytes);
char *read_ip_addr(char *addr, uint16_t rdlen, bytes_t *bytes);
void read_header(dns_message_t *msg);
void read_queries(dns_message_t *msg);
void read_answers(dns_message_t *msg);

// Allocates a dns_message, based on the length of the message in bytes
// `nbytes`, and returns it uninitialised, The queries and answer section are
// not allocated, (they will be allocated at initialisation).
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

uint8_t *read_domain(uint8_t *domain, bytes_t *bytes) {
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
    return domain;
}

char *read_ip_addr(char *addr, uint16_t rdlen, bytes_t *bytes) {
    inet_ntop(AF_INET6, bytes->data + bytes->offset, addr, INET6_ADDRSTRLEN);
    bytes->offset += rdlen * sizeof(*addr);
    return addr;
}

void read_header(dns_message_t *msg) {
    bytes_t *bytes = msg->bytes;

    read_field(&msg->id, bytes);

    uint16_t flags;
    read_field(&flags, bytes);
    // TODO: consider shift and shift to read or #define'ing masks
    msg->qr = (flags >> 15) & 1;
    msg->opcode = (flags >> 11) & 0xF;
    msg->aa = (flags >> 10) & 1;
    msg->tc = (flags >> 9) & 1;
    msg->rd = (flags >> 8) & 1;
    msg->ra = (flags >> 7) & 1;
    // 3 bits of z ignored
    msg->rcode = (flags >> 0) & 0xF;

    read_field(&msg->qdcount, bytes);
    read_field(&msg->ancount, bytes);
    read_field(&msg->nscount, bytes);
    read_field(&msg->arcount, bytes);
}

void read_queries(dns_message_t *msg) {
    bytes_t *bytes = msg->bytes;
    query_t *queries = malloc(msg->qdcount * sizeof(*queries));

    for (size_t i = 0; i < msg->qdcount; i++) {
        query_t query;
        query.qname = malloc(bytes->size * sizeof(*query.qname));
        *query.qname = '\0';

        read_domain(query.qname, bytes);
        read_field(&query.qtype, bytes);
        read_field(&query.qclass, bytes);

        queries[i] = query;
    }
    msg->queries = queries;
}

void read_answers(dns_message_t *msg) {
    bytes_t *bytes = msg->bytes;
    record_t *answers = malloc(msg->ancount * sizeof(*answers));

    for (size_t i = 0; i < msg->ancount; i++) {
        record_t answer;
        uint16_t name_offset = 0;
        read_field(&name_offset, bytes);

        // check that first (leftmost) two bits in 16-bit field are 0b11.
        assert(((name_offset >> 14) ^ 0x3) == 0);

        // clear first two bits (16th and 15th)
        name_offset &= ~((1 << 15) | (1 << 14));

        uint16_t old_offset = bytes->offset;
        bytes->offset = name_offset;
        answer.name = malloc(bytes->size * sizeof(*answer.name));
        *answer.name = '\0';
        read_domain(answer.name, bytes);
        bytes->offset = old_offset;

        read_field(&answer.type, bytes);
        read_field(&answer.class, bytes);

        uint16_t field1, field2;
        read_field(&field1, bytes);
        read_field(&field2, bytes);
        answer.ttl = field1 << 0x10 | field2;

        read_field(&answer.rdlen, bytes);

        char *addr = malloc(INET6_ADDRSTRLEN);
        read_ip_addr(addr, answer.rdlen, bytes);
        answer.rdata = addr;

        answers[i] = answer;
    }
    msg->answers = answers;
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

uint16_t get_flags(dns_message_t *msg) {
    uint16_t flags = 0;
    flags |= msg->qr << QR_POS;
    flags |= msg->opcode << OPCODE_POS;
    flags |= msg->aa << AA_POS;
    flags |= msg->tc << TC_POS;
    flags |= msg->rd << RD_POS;
    flags |= msg->ra << RA_POS;
    flags |= msg->rcode << RCODE_POS;
    return flags;
}
