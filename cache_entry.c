
#include "cache_entry.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

cache_entry_t *new_cache_entry(record_t *record, time_t cached_time) {
    cache_entry_t *entry = malloc(sizeof(*entry));
    record_t *new_record = malloc(sizeof(*new_record));
    assert(entry && new_record);

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

    return entry;
}

void free_cache_entry(cache_entry_t *cache_entry) {
    free(cache_entry->record->name);
    free(cache_entry->record->rdata);
    free(cache_entry->record);
    free(cache_entry);
}

bool cache_entry_is_expired(cache_entry_t *cache_entry) {
    // time_t curr_time = time(NULL);
    // double diff = difftime(curr_time, cache_entry->cached_time);
    return cache_entry->record->ttl == 0;
}

char *cache_entry_get_expiry(cache_entry_t *cache_entry, char *timestamp, size_t len) {
    time_t curr_time = time(NULL);
    time_t expiry = curr_time + cache_entry->record->ttl;
    struct tm *tm= gmtime(&expiry);

    strftime(timestamp, len, "%FT%T%z", tm);
    return timestamp;
}

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

int cache_entry_eq(cache_entry_t *entry1, cache_entry_t *entry2) {
    return strcmp((char *)entry1->record->name,
                  (char *)entry2->record->name) == 0;
}