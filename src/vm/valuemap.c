/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <stddef.h>
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

void lf_valuemap_delete(lfValueMap *map) {
    for (size_t i = 0; i < size(map); i++) {
        if ((*map)[i])
            lf_valuebucket_deleter((*map) + i);
    }
}

size_t lf_valuemap_compute_hash(const lfValue *value) {
    switch (value->type) {
        case LF_INT:
            return value->v.integer;
        case LF_BOOL:
            return value->v.boolean;
        case LF_STRING: {
            size_t hash = 0;
            for (size_t i = 0; i < length(&value->v.string); i++) {
                hash = (31 * hash + value->v.string[i]);
            }
            return hash;
        }
        case LF_NULL:
            return 0;
        case LF_CLOSURE:
            return (size_t)value->v.cl.f.lf.proto; /* func for C closures */
    }
}

bool lf_valuemap_compare_values(const lfValue *lhs, const lfValue *rhs) {
    if (lhs->type != rhs->type) {
        return false;
    }
    switch (lhs->type) {
        case LF_INT:
            return rhs->v.integer == lhs->v.integer;
        case LF_BOOL:
            return rhs->v.boolean == lhs->v.boolean;
        case LF_STRING:
            return length(&lhs->v.string) == length(&rhs->v.string) && !strncmp(rhs->v.string, lhs->v.string, length(&lhs->v.string));
        case LF_NULL:
            return true;
        case LF_CLOSURE:
            return lhs->v.cl.f.lf.proto == rhs->v.cl.f.lf.proto;
    }
}

lfValueMap lf_valuemap_create(size_t size) {
    lfValueMap map = array_new(lfValueBucket *, lf_valuebucket_deleter);
    array_reserve(&map, size);
    for (size_t i = 0; i < size; i++)
        map[i] = NULL;
    return map;
}

lfValueMap lf_valuemap_clone(const lfValueMap *map) {
    size_t map_size = size(map);
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

bool lf_valuemap_lookup(const lfValueMap *map, const lfValue *key, lfValue *out) {
    size_t hash = lf_valuemap_compute_hash(key) % size(map);
    lfValueBucket *b = (*map)[hash];
    if (b == NULL) return false;
    if (b->next == NULL) {
        if (out) *out = b->value;
        return true;
    }
    while (b) {
        if (lf_valuemap_compare_values(key, &b->key)) {
            if (out) *out = b->value;
            return true;
        }
        b = b->next;
    }
    return false;
}

void lf_valuemap_insert(lfValueMap *map, const lfValue *key, const lfValue *value) {
    if ((double)length(map) / (double)size(map) >= 0.75) { /* load factor over 75%, expand map */
        lfValueMap expanded = lf_valuemap_create(size(map) * 2);
        for (size_t i = 0; i < size(map); i++)
            if ((*map)[i]) {
                lfValueBucket *b = (*map)[i];
                do {
                    lf_valuemap_insert(&expanded, &b->key, &b->value);
                    b = b->next;
                } while (b);
            }
        lf_valuemap_delete(map);
        *map = expanded;
    }
    size_t hash = lf_valuemap_compute_hash(key) % size(map);
    lfValueBucket *next = malloc(sizeof(lfValueBucket));
    next->key = *key;
    next->value = *value;
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
    length(map)++;
}
