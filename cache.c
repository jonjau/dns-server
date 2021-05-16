
// TODO: COMMENTS

#include "cache.h"

#include <stdbool.h>
#include <string.h>

dns_message_t *cache_evict(cache_t *cache, dns_message_t *msg);
cache_entry_t *cache_find(cache_t *cache, char *name);

cache_t *new_cache(size_t capacity) {
    cache_t *cache = malloc(sizeof(*cache));
    list_t *messages = new_list();

    cache->entries = messages;
    cache->capacity = capacity;

    return cache;
}

void free_cache(cache_t *cache) {
    node_t *curr = cache->entries->head;
    while (curr) {
        free_cache_entry(curr->data);
        curr = curr->next;
    }
    free_list(cache->entries);
    free(cache);
}

cache_entry_t *cache_get(cache_t *cache, char *name) {
    cache_entry_t *entry = cache_find(cache, name);
    if (entry && !cache_entry_is_expired(entry)) {
        time_t curr_time = time(NULL);
        int diff = (int)difftime(curr_time, entry->cached_time);
        cache_entry_t *new_entry =
            new_cache_entry(entry->record, entry->cached_time);
        if (diff < 0) {
            new_entry->record->ttl = 0;
        } else {
            new_entry->record->ttl = entry->record->ttl - diff;
        }
        return new_entry;
    }
    return NULL;
}

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

bool cache_is_full(cache_t *cache) {
    return list_size(cache->entries) == cache->capacity;
}

cache_entry_t *cache_put(cache_t *cache, record_t *record) {
    if (!cache || !record) {
        return NULL;
    }
    time_t curr_time = time(NULL);
    cache_entry_t *new_entry = new_cache_entry(record, curr_time);

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