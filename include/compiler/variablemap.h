/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_VARIABLEMAP_H
#define LEAF_VARIABLEMAP_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "lib/array.h"

typedef struct lfVariableBucket {
    struct lfVariableBucket *next;
    struct lfVariableBucket *previous;
    const char *key;
    uint32_t value;
} lfVariableBucket;

typedef lfArray(lfVariableBucket *) lfVariableMap;

lfVariableMap variablemap_create(size_t size);
lfVariableMap variablemap_clone(const lfVariableMap *map);
bool variablemap_lookup(const lfVariableMap *map, const char *key, uint32_t *out);
void variablemap_insert(lfVariableMap *map, const char *key, uint32_t value); /* TODO: possibly resize the map when beneficial */

#endif /* LEAF_VARIABLEMAP_H */
