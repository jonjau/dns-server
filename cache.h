#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>
#include <stdint.h>

#include "dns_message.h"
#include "cache_entry.h"
#include "list.h"

typedef struct {
    list_t *entries;
    size_t capacity;
} cache_t;

cache_t *new_cache(size_t capacity);
void free_cache(cache_t *cache);

cache_entry_t *cache_get(cache_t *cache, char *name);
cache_entry_t *cache_put(cache_t *cache, record_t *record);

#endif
