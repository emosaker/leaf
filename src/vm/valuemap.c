/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "vm/value.h"
#include "lib/array.h"

void lf_valuemap_delete(lfValueMap *map) {
    array_delete(map);
}

static inline int hash_ptr_mix(size_t a, size_t b) {
    size_t h = a ^ (b + 0x9e3779b97f4a7c15 + (a << 6) + (a >> 2));
    return (int)(h & 0x7FFFFFFF);
}

unsigned int lf_valuemap_compute_hash(const lfValue *value) {
    switch (value->type) {
        case LF_INT:
            return value->v.integer;
        case LF_BOOL:
            return value->v.boolean;
        case LF_STRING: {
            unsigned int hash = 0;
            for (int i = 0; i < lf_string(value)->length; i++) {
                hash = (31 * hash + lf_string(value)->string[i]);
            }
            return hash;
        }
        case LF_NULL:
            return 0;
        case LF_CLOSURE: {
            const lfClosure *cl = lf_cl(value);
            if (cl->is_c) {
                return (unsigned int)((size_t)cl->f.c.func & 0x7FFFFFFF);
            } else {
                size_t base = (size_t)cl->f.lf.proto;
                unsigned int hash = (int)(base & 0x7FFFFFFF);
                /* TODO: Add cycle detection before uncommenting this
                for (int i = 0; i < cl->f.lf.proto->szupvalues; i++) {
                    int uv_hash = lf_valuemap_compute_hash(cl->f.lf.upvalues[i]);
                    hash ^= uv_hash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                } */
                return hash;
            }
        }
        default:
            return 0; /* unhashable for now :( */
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
            return lf_string(lhs)->length == lf_string(rhs)->length && !strncmp(lf_string(rhs)->string, lf_string(lhs)->string, lf_string(lhs)->length);
        case LF_NULL:
            return true;
        case LF_CLOSURE:
            return lf_cl(lhs)->f.c.func == lf_cl(rhs)->f.c.func;
        default:
            return false;
    }
}

lfValueMap lf_valuemap_create(int size) {
    lfValueMap map = array_new(lfKeyValuePair);
    array_reserve(&map, size);
    for (int i = 0; i < size; i++)
        map[i].value.type = LF_TCOUNT;
    return map;
}

lfValueMap lf_valuemap_clone(const lfValueMap *map) {
    int map_size = size(map);
    lfValueMap new_map = lf_valuemap_create(map_size);

    for (int i = 0; i < map_size; ++i) {
        new_map[i] = (*map)[i];
    }

    return new_map;
}

bool lf_valuemap_lookup(const lfValueMap *map, const lfValue *key, lfValue *out) {
    unsigned int hash = lf_valuemap_compute_hash(key) % size(map);
    lfKeyValuePair b = (*map)[hash];
    if (b.value.type == LF_TCOUNT) return false;
    if (!lf_valuemap_compare_values(&b.key, key)) return false;
    *out = b.value;
    return true;
}

void lf_valuemap_insert(lfValueMap *map, const lfValue *key, const lfValue *value) {
    unsigned int hash = lf_valuemap_compute_hash(key) % size(map);
    bool collision = (*map)[hash].value.type != LF_TCOUNT && !lf_valuemap_compare_values(&(*map)[hash].key, key);
    do {
        if ((double)length(map) / (double)size(map) >= 0.7 || (*map)[hash].value.type != LF_TCOUNT) { /* load factor over 70%, expand map */
            lfValueMap expanded = lf_valuemap_create(size(map) * 2);
            for (int i = 0; i < size(map); i++)
                if ((*map)[i].value.type != LF_TCOUNT) {
                    lf_valuemap_insert(&expanded, &(*map)[i].key, &(*map)[i].value);
                }
            lf_valuemap_delete(map);
            *map = expanded;
        }
        hash = lf_valuemap_compute_hash(key) % size(map);
        collision = (*map)[hash].value.type != LF_TCOUNT && !lf_valuemap_compare_values(&(*map)[hash].key, key);
    } while (collision);
    lfKeyValuePair v = (lfKeyValuePair) {
        .key = *key,
        .value = *value
    };
    (*map)[hash] = v;
    length(map)++;
}
