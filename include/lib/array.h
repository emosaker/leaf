/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_ARRAY_H
#define LEAF_ARRAY_H

#include <stdint.h>
#include <stdlib.h>

#define array_new_for(T) _array_new(sizeof(T))
#define array_new_deleter(T, D) _array_new_deleter(array_new_for(T), (void (*)(void *))D)
#define array_get_ctor(_1, _2, f, ...) f
#define array_ctor_for(...) array_get_ctor(__VA_ARGS__, array_new_deleter, array_new_for)
#define array_new(...) array_ctor_for(__VA_ARGS__)(__VA_ARGS__)

#define lfArray(T) T *
#define header(ARR) (((lfArrayHeader *)(*(ARR)))[-1])
#define length(ARR) header(ARR).length
#define size(ARR) header(ARR).size
#define typesize(ARR) header(ARR).typesize
#define deleter(ARR) header(ARR).deleter

typedef struct lfArrayHeader {
    int size;
    int length;
    size_t typesize;
    void (*deleter)(void *element);
    size_t pad;
} lfArrayHeader;

static inline void *_array_new(size_t typesize) {
    void *arr = malloc(sizeof(lfArrayHeader));
    *(lfArrayHeader *)arr = (lfArrayHeader) {
        .size = 0,
        .length = 0,
        .typesize = typesize,
        .deleter = NULL
    };
    return (uint8_t *)arr + sizeof(lfArrayHeader);
}

static inline void *_array_new_deleter(void *array, void (*new_deleter)(void *element)) {
    deleter(&array) = new_deleter;
    return array;
}

#define array_reserve(ARR, S) {                                                                                                   \
    int s = (S);                                                                                                                  \
    if (size(ARR) < s) {                                                                                                          \
        *(ARR) = (void *)((uint8_t *)realloc(&header(ARR), sizeof(lfArrayHeader) + (s * typesize(ARR))) + sizeof(lfArrayHeader)); \
        size(ARR) = s;                                                                                                            \
    }                                                                                                                             \
}

#define array_push(ARR, ELEMENT) {                             \
    if (length(ARR) == size(ARR)) {                            \
        array_reserve(ARR, size(ARR) > 0 ? size(ARR) * 2 : 4); \
    }                                                          \
    (*(ARR))[length(ARR)++] = ELEMENT;                         \
}

#define array_delete(ARR) {                     \
    if (deleter(ARR)) {                         \
        for (int i = 0; i < length(ARR); i++) { \
            deleter(ARR)((*ARR) + i);           \
        }                                       \
    }                                           \
    free(&header(ARR));                         \
}

#endif /* LEAF_ARRAY_H */
