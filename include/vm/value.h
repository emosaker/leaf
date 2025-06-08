/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_VALUE_H
#define LEAF_VALUE_H

#include "compiler/bytecode.h"

typedef struct lfClosure {
    lfProto *proto;
} lfClosure;

typedef enum lfValueType {
    LF_INT,
    LF_CLOSURE
} lfValueType;

typedef struct lfValue {
    lfValueType type;
    union {
        uint64_t integer;
        lfClosure *cl;
    } v;
} lfValue;

#endif /* LEAF_VALUE_H */
