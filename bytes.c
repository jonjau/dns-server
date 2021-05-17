#include <arpa/inet.h>
#include <string.h>
#include <assert.h>

#include "bytes.h"

bytes_t *new_bytes(size_t nbytes) {
    bytes_t *bytes = malloc(sizeof(*bytes));
    uint8_t *data = malloc(nbytes * sizeof(*data));
    assert(bytes && data);

    bytes->data = data;
    bytes->offset = 0;
    bytes->size = nbytes;

    return bytes;
}

void free_bytes(bytes_t *bytes) {
    free(bytes->data);
    free(bytes);
}

// Copies two octets from `bytes` to `field`, keeping track of the offset
// and returns the integer value in host byte order.
uint32_t read32(uint32_t *field, bytes_t *bytes) {
    memcpy(field, bytes->data + bytes->offset, sizeof(*field));
    bytes->offset += sizeof(*field);
    *field = ntohl(*field);
    return *field;
}

// Copies two octets from `bytes` to `field`, keeping track of the offset
// and returns the integer value in host byte order.
uint16_t read16(uint16_t *field, bytes_t *bytes) {
    memcpy(field, bytes->data + bytes->offset, sizeof(*field));
    bytes->offset += sizeof(*field);
    *field = ntohs(*field);
    return *field;
}

// Copies an octet from `bytes` to `field`, keeping track of the offset and
// returns the integer value of the octet read.
uint8_t read8(uint8_t *octet, bytes_t *bytes) {
    memcpy(octet, bytes->data + bytes->offset, sizeof(*octet));
    bytes->offset += sizeof(*octet);
    return *octet;
}

void write32(bytes_t *bytes, uint32_t field) {
    uint32_t field_val = htonl(field);
    memcpy(bytes->data + bytes->offset, &field_val, sizeof(field));
    bytes->offset += sizeof(field);
}

void write16(bytes_t *bytes, uint16_t field) {
    uint16_t field_val = htons(field);
    memcpy(bytes->data + bytes->offset, &field_val, sizeof(field));
    bytes->offset += sizeof(field);
}

void write8(bytes_t *bytes, uint16_t octet) {
    memcpy(bytes->data + bytes->offset, &octet, sizeof(octet));
    bytes->offset += sizeof(octet);
}
