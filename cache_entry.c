/**
 * COMP30023 Project 2
 * Author: Jonathan Jauhari 1038331
 *
 * Cache entry module containing functions for manipulation of cached
 * resource record entries.
 */

#include "cache_entry.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Create and returns a new cache entry containing the record (this function
// will copy it) and the time it was cached/will expire. May be used as a deep
// copy function.
cache_entry_t *new_cache_entry(record_t *record, time_t cached_time,
                               time_t expiry_time) {
    cache_entry_t *entry = malloc(sizeof(*entry));
    record_t *new_record = malloc(sizeof(*new_record));
    assert(entry && new_record);

    // copy the record
    new_record->name = malloc(strlen((char *)record->name) + 1);
    strcpy((char *)new_record->name, (char *)record->name);

    new_record->type = record->type;
    new_record->class = record->class;
    new_record->ttl = record->ttl;
    new_record->rdlen = record->rdlen;

    new_record->rdata = malloc(strlen(record->rdata) + 1);
    strcpy(new_record->rdata, record->rdata);

    entry->record = new_record;
    entry->cached_time = cached_time;
    entry->expiry_time = expiry_time;

    return entry;
}

// Frees a cache entry and the resource record it holds
void free_cache_entry(cache_entry_t *cache_entry) {
    free(cache_entry->record->name);
    free(cache_entry->record->rdata);
    free(cache_entry->record);
    free(cache_entry);
}

// Returns true if the time-to-live of `cache_entry` is 0, false otherwise
bool cache_entry_is_expired(cache_entry_t *cache_entry) {
    return cache_entry->record->ttl == 0;
}

// Compares two cache entries in the context of cache eviction.
// `entry1` goes before `entry2` if its current TTL is less, breaking ties
// with the name of the resource records the entries hold.
int cache_entry_cmp(cache_entry_t *entry1, cache_entry_t *entry2) {
    record_t *record1 = entry1->record;
    record_t *record2 = entry2->record;
    if (record1->ttl < record2->ttl) {
        return -1;
    } else if (record1->ttl > record2->ttl) {
        return +1;
    } else {
        return strcmp((char *)record1->name, (char *)record2->name);
    }
}

// Returns true if `entry1` and `entry2` hold resource records with the same
// name
bool cache_entry_eq(cache_entry_t *entry1, cache_entry_t *entry2) {
    return strcmp((char *)entry1->record->name,
                  (char *)entry2->record->name) == 0;
}

// Get the time `cache_entry` expires and put it in `timestamp`, which has
// length `len`, formatted like 2021-05-10T02:07:11+0000. Returns a pointer
// to `timestamp`.
char *cache_entry_get_expiry(cache_entry_t *cache_entry, char *timestamp,
                             size_t len) {
    struct tm *tm = gmtime(&cache_entry->expiry_time);

    strftime(timestamp, len, "%FT%T%z", tm);
    return timestamp;
}
