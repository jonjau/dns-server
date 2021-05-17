/**
 * COMP30023 Project 2
 * Author: Jonathan Jauhari 1038331
 * 
 * Bytes array module containing functions for manipulation
 * (checked read/write) of bytes into/from an array.
 */

#ifndef BYTES_H
#define BYTES_H

#include <stdint.h>
#include <stdlib.h>

// An array of bytes along with a stored offset to start reading from and
// a size
typedef struct {
    uint8_t *data;
    uint16_t size;
    uint16_t offset;
} bytes_t;

bytes_t *new_bytes(size_t nbytes);
void free_bytes(bytes_t *bytes);

uint32_t read32(uint32_t *field, bytes_t *bytes);
uint16_t read16(uint16_t *field, bytes_t *bytes);
uint8_t read8(uint8_t *octet, bytes_t *bytes);

void write32(bytes_t *bytes, uint32_t field);
void write16(bytes_t *bytes, uint16_t field);
void write8(bytes_t *bytes, uint16_t octet);

#endif
