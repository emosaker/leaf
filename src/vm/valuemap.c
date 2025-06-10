/*
 * This file is part of the leaf programming language
 */

#include <string.h>

#include "vm/value.h"
#include "lib/array.h"

void lf_valuebucket_deleter(lfValueBucket **bucket) {
    lfValueBucket *current = *bucket;
    if (current) {
        while (current->next != NULL) current = current->next;
        while (current->previous) {
            lfValueBucket *tbf = current;
            current = current->previous;
            free(tbf);
        }
        free(current);
    }
}

size_t lf_valuemap_compute_hash(const char *s) {
    int p = 59;
    int m = 1e9 + 9;
    size_t hash_value = 0;
    size_t p_pow = 1;
    size_t len = strlen(s);
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c >= 'a' && c <= 'z') hash_value = (hash_value + (s[i] - 'a' + 1) * p_pow) % m;
        else if (c >= 'A' && c <= 'Z') hash_value = (hash_value + (s[i] - 'A' + 27) * p_pow) % m;
        else hash_value = (hash_value + 53 * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }

    return hash_value;
}

lfValueMap lf_valuemap_create(size_t size) {
    lfValueMap map = array_new(lfValueBucket *, lf_valuebucket_deleter);
    array_reserve(&map, size);
    for (size_t i = 0; i < size; i++)
        array_push(&map, NULL);
    return map;
}

lfValueMap lf_valuemap_clone(const lfValueMap *map) {
    size_t map_size = length(map);
    lfValueMap new_map = lf_valuemap_create(map_size);

    for (size_t i = 0; i < map_size; ++i) {
        lfValueBucket *current = (*map)[i];
        lfValueBucket *prev_new_bucket = NULL;

        while (current) {
            lfValueBucket *new_bucket = malloc(sizeof(lfValueBucket));
            new_bucket->key = current->key;
            new_bucket->value = current->value;
            new_bucket->previous = prev_new_bucket;
            new_bucket->next = NULL;

            if (prev_new_bucket == NULL) {
                new_map[i] = new_bucket;
            } else {
                prev_new_bucket->next = new_bucket;
            }

            prev_new_bucket = new_bucket;
            current = current->next;
        }
    }

    return new_map;
}

bool lf_valuemap_lookup(const lfValueMap *map, const char *key, lfValue *out) {
    size_t hash = lf_valuemap_compute_hash(key) % length(map);
    lfValueBucket *b = (*map)[hash];
    if (b == NULL) return false;
    if (b->next == NULL) {
        if (out) *out = b->value;
        return true;
    }
    while (b) {
        if (!strcmp(key, b->key)) {
            if (out) *out = b->value;
            return true;
        }
        b = b->next;
    }
    return false;
}

void lf_valuemap_insert(lfValueMap *map, const char *key, lfValue value) {
    size_t hash = lf_valuemap_compute_hash(key) % length(map);
    lfValueBucket *next = malloc(sizeof(lfValueBucket));
    next->key = key;
    next->value = value;
    next->previous = NULL;
    next->next = NULL;
    lfValueBucket *b = (*map)[hash];
    if (b != NULL) {
        while (b->next) b = b->next;
        next->previous = b;
        b->next = next;
    } else {
        (*map)[hash] = next;
    }
}
