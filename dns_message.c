#include <stdlib.h>
#include <assert.h>

#include "dns_message.h"

dns_message_t *new_dns_message() {

    dns_message_t *message = malloc(sizeof(*message));
    assert(message);

    return message;
}

void free_message(dns_message_t *message) {
    free(message);
}