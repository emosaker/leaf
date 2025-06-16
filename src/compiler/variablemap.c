/*
 * This file is part of the leaf programming language
 */

#include <string.h>

#include "compiler/variablemap.h"
#include "lib/array.h"

void lf_variablebucket_deleter(lfVariableBucket **bucket) {
    lfVariableBucket *current = *bucket;
    if (current) {
        while (current->next != NULL) current = current->next;
        while (current->previous) {
            lfVariableBucket *tbf = current;
            current = current->previous;
            free(tbf);
        }
        free(current);
    }
}

int lf_variablemap_compute_hash(const char *s) {
    int p = 59;
    int m = 1e9 + 9;
    int hash_value = 0;
    int p_pow = 1;
    int len = strlen(s);
    for (int i = 0; i < len; i++) {
        char c = s[i];
        if (c >= 'a' && c <= 'z') hash_value = (hash_value + (s[i] - 'a' + 1) * p_pow) % m;
        else if (c >= 'A' && c <= 'Z') hash_value = (hash_value + (s[i] - 'A' + 27) * p_pow) % m;
        else hash_value = (hash_value + 53 * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }

    return hash_value;
}

lfVariableMap lf_variablemap_create(int size) {
    lfVariableMap map = array_new(lfVariableBucket *, lf_variablebucket_deleter);
    array_reserve(&map, size);
    for (int i = 0; i < size; i++)
        array_push(&map, NULL);
    return map;
}

lfVariableMap lf_variablemap_clone(const lfVariableMap *map) {
    int map_size = length(map);
    lfVariableMap new_map = lf_variablemap_create(map_size);

    for (int i = 0; i < map_size; ++i) {
        lfVariableBucket *current = (*map)[i];
        lfVariableBucket *prev_new_bucket = NULL;

        while (current) {
            lfVariableBucket *new_bucket = malloc(sizeof(lfVariableBucket));
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

bool lf_variablemap_lookup(const lfVariableMap *map, const char *key, lfVariable *out) {
    int hash = lf_variablemap_compute_hash(key) % length(map);
    lfVariableBucket *b = (*map)[hash];
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

void lf_variablemap_insert(lfVariableMap *map, const char *key, lfVariable value) {
    int hash = lf_variablemap_compute_hash(key) % length(map);
    lfVariableBucket *next = malloc(sizeof(lfVariableBucket));
    next->key = key;
    next->value = value;
    next->previous = NULL;
    next->next = NULL;
    lfVariableBucket *b = (*map)[hash];
    if (b != NULL) {
        while (b->next) b = b->next;
        next->previous = b;
        b->next = next;
    } else {
        (*map)[hash] = next;
    }
}
