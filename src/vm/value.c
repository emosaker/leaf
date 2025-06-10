/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <string.h>

#include "compiler/bytecode.h"
#include "lib/array.h"
#include "vm/error.h"
#include "vm/value.h"

void lf_setglobal(lfState *state, const char *key) {
    lfValue v = lf_pop(state);
    lf_valuemap_insert(&state->globals, key, v);
}

void lf_getglobal(lfState *state, const char *key) {
    lfValue v;
    if (lf_valuemap_lookup(&state->globals, key, &v)) {
        lf_push(state, &v);
    } else {
        lf_pushnull(state);
    }
}

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

void lf_pushbool(lfState *state, bool value) {
    LF_CHECKTOP(state);
    *state->top++ = (lfValue) {
        .type = LF_BOOL,
        .v.boolean = value
    };
}

void lf_pushnull(lfState *state) {
    LF_CHECKTOP(state);
    *state->top++ = (lfValue) {
        .type = LF_NULL
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

void lf_getupval(lfState *state, int index) {
    lf_push(state, state->upvalues[index]);
}

void lf_setupval(lfState *state, int index) {
    lfValue v = lf_pop(state);
    *state->upvalues[index] = v;
}

void lf_newccl(lfState *state, lfccl func) {
    LF_CHECKTOP(state);
    *state->top++ = (lfValue) {
        .type = LF_CLOSURE,
        .v.cl = (lfClosure) {
            .is_c = true,
            .f.c.func = func
        }
    };
}

void lf_newlfcl(lfState *state, lfProto *proto) {
    LF_CHECKTOP(state);
    *state->top++ = (lfValue) {
        .type = LF_CLOSURE,
        .v.cl = (lfClosure) {
            .is_c = false,
            .f.lf.proto = proto,
            .f.lf.upvalues = malloc(proto->nupvalues * sizeof(lfValue *))
        }
    };
}

const char *lf_typeof(const lfValue *value) {
    switch (value->type) {
        case LF_NULL: return "null";
        case LF_INT: return "int";
        case LF_STRING: return "string";
        case LF_BOOL: return "bool";
        case LF_CLOSURE: return "closure";
    }
}

void lf_printvalue(const lfValue *value) {
    switch (value->type) {
        case LF_NULL:
            printf("null");
            break;
        case LF_INT:
            printf("%ld", value->v.integer);
            break;
        case LF_STRING:
            printf("%s", value->v.string);
            break;
        case LF_BOOL:
            printf("%s", value->v.boolean ? "true" : "false");
            break;
        case LF_CLOSURE:
            printf("closure");
            break;
    }
}

void lf_value_deleter(const lfValue *value) {
    switch (value->type) {
        case LF_STRING:
            array_delete(&value->v.string);
            break;
        case LF_CLOSURE:
            if (!value->v.cl.is_c) {
                free(value->v.cl.f.lf.upvalues);
            }
            lf_proto_deleter((lfProto **)&value->v.cl.f.lf.proto);
            break;
        default:
            break;
    }
}
