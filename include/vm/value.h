/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_VALUE_H
#define LEAF_VALUE_H

#include "compiler/bytecode.h"
#include "lib/array.h"

typedef struct lfClosure {
    lfProto *proto;
} lfClosure;

typedef enum lfValueType {
    LF_INT,
    LF_STRING,
    LF_CLOSURE
} lfValueType;

typedef struct lfValue {
    lfValueType type;
    union {
        uint64_t integer;
        lfArray(char) string;
        lfClosure *cl;
    } v;
} lfValue;

const char *lf_typeof(const lfValue *value);
void lf_printvalue(const lfValue *value);
void lf_deletevalue(const lfValue *value);

#endif /* LEAF_VALUE_H */
