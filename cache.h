/**
 * COMP30023 Project 2
 * Author: Jonathan Jauhari 1038331
 *
 * Cache module containing functions for manipulation of resource record
 * caches, for a DNS server. The cache is assumed to only hold a set
 * number of IPv6 resource records. The eviction policy is based on least
 * TTL.
 */

#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>
#include <stdint.h>

#include "dns_message.h"
#include "cache_entry.h"
#include "list.h"

// A cache has a set capacity, and contains list of entries, which contain the
// resource records and the time they were cached.
typedef struct {
    list_t *entries;
    size_t capacity;
} cache_t;

cache_t *new_cache(size_t capacity);
void free_cache(cache_t *cache);

cache_entry_t *cache_get(cache_t *cache, char *name);
cache_entry_t *cache_put(cache_t *cache, record_t *record);

#endif
