/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_VALUEMAP_H
#define LEAF_VALUEMAP_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "lib/array.h"
#include "vm/value.h"

typedef struct lfValueBucket {
    struct lfValueBucket *next;
    struct lfValueBucket *previous;
    const char *key;
    lfValue value;
} lfValueBucket;

typedef lfArray(lfValueBucket *) lfValueMap;

lfValueMap lf_valuemap_create(size_t size);
lfValueMap lf_valuemap_clone(const lfValueMap *map);
bool lf_valuemap_lookup(const lfValueMap *map, const char *key, lfValue *out);
void lf_valuemap_insert(lfValueMap *map, const char *key, lfValue value); /* TODO: possibly resize the map when beneficial */

#endif /* LEAF_VALUEMAP_H */
