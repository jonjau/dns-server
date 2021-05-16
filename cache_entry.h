#ifndef CACHE_ENTRY_H
#define CACHE_ENTRY_H

#include <time.h>

#include "dns_message.h"

typedef struct {
    record_t *record;
    time_t cached_time;
} cache_entry_t;

cache_entry_t *new_cache_entry(record_t *record, time_t cached_time);
void free_cache_entry(cache_entry_t *cache_entry);

bool cache_entry_is_expired(cache_entry_t *cache_entry);
int cache_entry_cmp(cache_entry_t *entry1, cache_entry_t *entry2);
int cache_entry_eq(cache_entry_t *entry1, cache_entry_t *entry2);

char *cache_entry_get_expiry(cache_entry_t *cache_entry, char *timestamp, size_t len);

#endif
