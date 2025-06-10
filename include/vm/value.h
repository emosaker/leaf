/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_VALUE_H
#define LEAF_VALUE_H

#include <stdlib.h>
#include <setjmp.h>
#include <stdbool.h>

#include "compiler/bytecode.h"
#include "lib/array.h"

#define LF_CHECKTOP(STATE) { \
    if (((STATE)->top - (STATE)->stack) / 8 >= (STATE)->stack_size) { \
        (STATE)->stack_size *= 2; \
        (STATE)->stack = realloc((STATE)->stack, (STATE)->stack_size * sizeof(lfValue)); \
    } \
}

#define LF_STACKSIZE(STATE) ((STATE)->top - (STATE)->base)

typedef lfArray(struct lfValueBucket *) lfValueMap;

typedef struct lfState {
    size_t stack_size;
    struct lfValue *stack;

    struct lfValue *base;
    struct lfValue *top;

    lfValueMap globals;

    struct lfValue **upvalues;

    bool errored;
    jmp_buf error_buf;
} lfState;

typedef int(*lfccl)(lfState *state);

typedef struct lfClosure {
    bool is_c;
    union {
        struct {
            lfProto *proto;
            struct lfValue **upvalues;
        } lf;
        struct {
            lfccl func;
        } c;
    } f;
} lfClosure;

typedef enum lfValueType {
    LF_NULL,
    LF_INT,
    LF_STRING,
    LF_BOOL,
    LF_CLOSURE
} lfValueType;

typedef struct lfValue {
    lfValueType type;
    union {
        uint64_t integer;
        lfArray(char) string;
        bool boolean;
        lfClosure cl;
    } v;
} lfValue;

typedef struct lfValueBucket {
    struct lfValueBucket *next;
    struct lfValueBucket *previous;
    const char *key;
    lfValue value;
} lfValueBucket;

/* value map */
lfValueMap lf_valuemap_create(size_t size);
lfValueMap lf_valuemap_clone(const lfValueMap *map);
bool lf_valuemap_lookup(const lfValueMap *map, const char *key, lfValue *out);
void lf_valuemap_insert(lfValueMap *map, const char *key, lfValue value); /* TODO: possibly resize the map when beneficial */

/* state */
lfState *lf_state_create(void);
void lf_state_delete(lfState *state);

/* globals */
void lf_setglobal(lfState *state, const char *key);
void lf_getglobal(lfState *state, const char *key);

/* stack */
void lf_pushint(lfState *state, uint64_t value);
void lf_pushstring(lfState *state, char *value, size_t length);
void lf_pushbool(lfState *state, bool value);
void lf_pushnull(lfState *state);
void lf_pushlfstring(lfState *state, lfArray(char) value); /* no-clone version, requires an lfArray */
lfValue lf_pop(lfState *state);
void lf_push(lfState *state, const lfValue *value);
void lf_getupval(lfState *state, int index);
void lf_setupval(lfState *state, int index);

/* value */
void lf_newccl(lfState *state, lfccl func);
void lf_newlfcl(lfState *state, lfProto *proto);

const char *lf_typeof(const lfValue *value);
void lf_printvalue(const lfValue *value);
void lf_value_deleter(const lfValue *value);

#endif /* LEAF_VALUE_H */
