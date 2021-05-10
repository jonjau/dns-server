#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdlib.h>

char *get_timestamp(char *timestamp, size_t len);
size_t read_fully(int fd, uint8_t *buf, size_t nbytes);
size_t write_fully(int fd, uint8_t *buf, size_t nbytes);

#endif
