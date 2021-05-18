
/**
 * COMP30023 Project 2
 * Author: Jonathan Jauhari 1038331
 *
 * Cache module containing functions for manipulation of resource record
 * caches, for a DNS server. The cache is assumed to only hold a set
 * number of IPv6 resource records. The eviction policy is based on least
 * TTL.
 */

#include "cache.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

cache_entry_t *cache_find(cache_t *cache, char *name);
bool cache_is_full(cache_t *cache);

// Creates and returns a new cache with a set `capacity`
cache_t *new_cache(size_t capacity) {
    cache_t *cache = malloc(sizeof(*cache));
    assert(cache);

    cache->entries = new_list();
    ;
    cache->capacity = capacity;

    return cache;
}

// Frees a cache and the linked list that backs it, and the entries in it
void free_cache(cache_t *cache) {
    node_t *curr = cache->entries->head;
    while (curr) {
        free_cache_entry(curr->data);
        curr = curr->next;
    }
    free_list(cache->entries);
    free(cache);
}

// Attempt to retrieve from `cache` an unexpired cache entry for a resource
// record with name `name`. If such an entry exists, its TTL is updated and
// a deep copy of the entry is returned (remember to free). Otherwise, NULL
// is returned.
cache_entry_t *cache_get(cache_t *cache, char *name) {
    cache_entry_t *entry = cache_find(cache, name);
    if (entry && !cache_entry_is_expired(entry)) {
        time_t curr_time = time(NULL);
        int diff = (int)difftime(curr_time, entry->cached_time);
        if (diff > entry->record->ttl) {
            entry->record->ttl = 0;
        } else {
            entry->record->ttl = entry->record->ttl - diff;
        }
        cache_entry_t *new_entry = new_cache_entry(
            entry->record, entry->cached_time, entry->expiry_time);
        if (!cache_entry_is_expired(new_entry)) {
            return new_entry;
        }
    }
    return NULL;
}

// Returns the first entry in `cache` (this should be the only one if any)
// that holds a record with name `name`, NULL otherwise.
cache_entry_t *cache_find(cache_t *cache, char *name) {
    if (!cache) {
        return NULL;
    }
    node_t *curr = cache->entries->head;
    while (curr) {
        if (strcmp((char *)curr->data->record->name, name) == 0) {
            return curr->data;
        }
        curr = curr->next;
    }
    return NULL;
}

// Returns true if the cache is full, false otherwise
bool cache_is_full(cache_t *cache) {
    return list_size(cache->entries) == cache->capacity;
}

// Puts a resource record `record` into `cache`. If an expired entry holding
// that record exists, it is evicted and replaced by `record`. Otherwise, if
// the cache is full, then the record with the lowest TTL is replaced
// instead. In both cases, the record evicted is returned (remember to free
// this). If no record is evicted, then this function returns NULL.
cache_entry_t *cache_put(cache_t *cache, record_t *record) {
    assert(cache && record);

    time_t curr_time = time(NULL);
    cache_entry_t *new_entry =
        new_cache_entry(record, curr_time, curr_time + record->ttl);

    cache_entry_t *to_evict = cache_find(cache, (char *)record->name);
    if (to_evict && cache_entry_is_expired(to_evict)) {
        list_remove(cache->entries, to_evict);
        list_add_end(cache->entries, new_entry);
        return to_evict;
    }
    if (cache_is_full(cache)) {
        cache_entry_t *to_evict = list_min(cache->entries, cache_entry_cmp);
        list_remove(cache->entries, to_evict);
        list_add_end(cache->entries, new_entry);
        return to_evict;
    }
    list_add_end(cache->entries, new_entry);
    return NULL;
}