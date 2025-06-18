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

int lf_valuemap_compute_hash(const lfValue *value) {
    switch (value->type) {
        case LF_INT:
            return value->v.integer;
        case LF_BOOL:
            return value->v.boolean;
        case LF_STRING: {
            int hash = 0;
            for (int i = 0; i < lf_string(value)->length; i++) {
                hash = (31 * hash + lf_string(value)->string[i]);
            }
            return hash;
        }
        case LF_NULL:
            return 0;
        case LF_CLOSURE:
            return (int)(size_t)lf_cl(value)->f.c.func;
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
    int hash = lf_valuemap_compute_hash(key) % size(map);
    lfKeyValuePair b = (*map)[hash];
    if (b.value.type == LF_TCOUNT) return false;
    *out = b.value;
    return true;
}

void lf_valuemap_insert(lfValueMap *map, const lfValue *key, const lfValue *value) {
    int hash = lf_valuemap_compute_hash(key) % size(map);
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
