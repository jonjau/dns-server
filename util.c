/**
 * COMP30023 Project 2
 * Author: Jonathan Jauhari 1038331
 *
 * Util module containing miscellaneous functions
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#include "util.h"

// Reads `nbytes` bytes into `buf` from `fd`, if one call to read() does
// not fully read the message, this function continues until the entire
// message is read. Exits if error.
size_t read_fully(int fd, uint8_t *buf, size_t nbytes) {
    size_t total_nread = 0;
    size_t nread = 0;
    while (true) {
        nread = read(fd, buf + total_nread, nbytes);
        if (nread < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        total_nread += nread;

        if (total_nread == nbytes) {
            break;
        }
    }
    return total_nread;
}

// Writes `nbytes` bytes of `buf` to `fd`, if one call to write() does
// not fully write the message, this function continues until the entire
// message is written. Exits if error.
size_t write_fully(int fd, uint8_t *buf, size_t nbytes) {
    size_t total_nwritten = 0;
    size_t nwritten = 0;
    while (true) {
        nwritten = write(fd, buf + total_nwritten, nbytes);
        if (nwritten < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        total_nwritten += nwritten;

        if (total_nwritten == nbytes) {
            break;
        }
    }
    return total_nwritten;
}

// Get the current timestamp and put it in `timestamp`, which has
// length `len`, formatted like 2021-05-10T02:07:11+0000. Returns a pointer
// to `timestamp`
char *get_timestamp(char *timestamp, size_t len) {
    const time_t rawtime = time(NULL);
    struct tm *tm = gmtime(&rawtime);

    strftime(timestamp, len, "%FT%T%z", tm);
    return timestamp;
}
