#ifndef DNS_MESSAGE_H
#define DNS_MESSAGE_H

#include <stdint.h>
#include <stdbool.h>

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

#endif
