/**
 * COMP30023 Project 2
 * Author: Jonathan Jauhari 1038331
 *
 * Util module containing miscellaneous functions
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdlib.h>

#define TIMESTAMP_LEN 41  // maximum length based on ISO 8601 limits

char *get_timestamp(char *timestamp, size_t len);
size_t read_fully(int fd, uint8_t *buf, size_t nbytes);
size_t write_fully(int fd, uint8_t *buf, size_t nbytes);

#endif
