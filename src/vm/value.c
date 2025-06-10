/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <string.h>

#include "lib/array.h"
#include "vm/error.h"
#include "vm/state.h"
#include "vm/value.h"

void lf_pushint(lfState *state, uint64_t value) {
    LF_CHECKTOP(state);
    *state->top++ = (lfValue) {
        .type = LF_INT,
        .v.integer = value
    };
}

void lf_pushstring(lfState *state, char *value, size_t length) {
    LF_CHECKTOP(state);
    lfArray(char) buf = array_new(char);
    array_reserve(&buf, length + 1);
    length(&buf) = length;
    memcpy(buf, value, length);
    buf[length] = 0;
    *state->top++ = (lfValue) {
        .type = LF_STRING,
        .v.string = buf
    };
}

void lf_pushlfstring(lfState *state, lfArray(char) value) {
    LF_CHECKTOP(state);
    *state->top++ = (lfValue) {
        .type = LF_STRING,
        .v.string = value
    };
}

lfValue lf_pop(lfState *state) {
    if (LF_STACKSIZE(state) < 1)
        lf_error(state, "attempt to pop empty stack");
    return *--state->top;
}

void lf_push(lfState *state, const lfValue *value) {
    *state->top++ = *value;
}

const char *lf_typeof(const lfValue *value) {
    switch (value->type) {
        case LF_INT: return "int";
        case LF_STRING: return "string";
        case LF_CLOSURE: return "closure";
    }
}

void lf_printvalue(const lfValue *value) {
    switch (value->type) {
        case LF_INT:
            printf("%ld", value->v.integer);
            break;
        case LF_STRING:
            printf("%s", value->v.string);
            break;
        case LF_CLOSURE:
            printf("closure %p", value->v.cl);
            break;
    }
}

void lf_deletevalue(const lfValue *value) {
    switch (value->type) {
        case LF_STRING:
            array_delete(&value->v.string);
            break;
        default:
            break;
    }
}
