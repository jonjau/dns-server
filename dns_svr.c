#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <arpa/inet.h>

// inet_ntops
// TODO: ERRNO error checking

typedef struct {
    uint16_t id;
    bool qr;
    uint8_t opcode;
    bool aa;
    bool tc;
    bool rd;
    bool ra;
    uint8_t rcode;
} dns_message_t;

int main(int argc, char* argv[]) {

    uint16_t size_field;
    if (read(STDIN_FILENO, &size_field, sizeof(size_field)) == -1) {
        return -1;
    }
    size_t nbytes = ntohs(size_field);
    printf("%lu\n", nbytes);

    uint8_t *bytes = malloc(nbytes * sizeof(*bytes));
    if (read(STDIN_FILENO, bytes, nbytes) == -1) {
        return -1;
    }


    int i = 0;
    // unsigned int id;
    uint16_t field;

    memcpy(&field, bytes, sizeof(field));
    uint16_t id = ntohs(field);
    i += sizeof(field);

    memcpy(&field, bytes+i, sizeof(field));
    uint16_t flags = ntohs(field);
    i += sizeof(field);

    // unsigned char flags1 = bytes[i++];
    bool qr = (flags >> 15) & 1;
    unsigned char opcode = (flags >> 11) & 0b1111;
    bool aa = (flags >> 10) & 1;
    bool tc = (flags >> 9) & 1;
    bool rd = (flags >> 8) & 1;

    // unsigned char flags2 = bytes[i++];
    bool ra = (flags >> 7) & 1;
    // z ignored
    unsigned char rcode = (flags >> 0) & 0b1111;

    memcpy(&field, bytes+i, sizeof(field));
    uint16_t qdcount = ntohs(field);
    i += sizeof(field);

    memcpy(&field, bytes+i, sizeof(field));
    uint16_t ancount = ntohs(field);
    i += sizeof(field);

    memcpy(&field, bytes+i, sizeof(field));
    uint16_t nscount = ntohs(field);
    i += sizeof(field);

    memcpy(&field, bytes+i, sizeof(field));
    uint16_t arcount = ntohs(field);
    i += sizeof(field);

    // if (read(STDIN_FILENO, &field, sizeof(field)) == -1) {
    //     return -1;
    // }
    // uint16_t qdcount = ntohs(field);
    // if (read(STDIN_FILENO, &field, sizeof(field)) == -1) {
    //     return -1;
    // }
    // uint16_t ancount = ntohs(field);
    // if (read(STDIN_FILENO, &field, sizeof(field)) == -1) {
    //     return -1;
    // }
    // uint16_t nscount = ntohs(field);

    

    // for (int j=0; j < 2; j++) {
    //     buf[j] = bytes[i++];
    // }
    // unsigned int qdcount = (buf[0] << 8) + (buf[1]);

    // for (int j=0; j < 2; j++) {
    //     buf[j] = bytes[i++];
    // }
    // unsigned int ancount = (buf[0] << 8) + (buf[1]);

    // for (int j=0; j < 2; j++) {
    //     buf[j] = bytes[i++];
    // }
    // unsigned int nscount = (buf[0] << 8) + (buf[1]);

    // for (int j=0; j < 2; j++) {
    //     buf[j] = bytes[i++];
    // }
    // unsigned int arcount = (buf[0] << 8) + (buf[1]);


    free(bytes);

    return 0;
}

