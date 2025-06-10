/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_STATE_H
#define LEAF_STATE_H

#include <stdlib.h>
#include <setjmp.h>
#include <stdbool.h>

#include "vm/valuemap.h"
#include "vm/value.h"

#define LF_CHECKTOP(STATE) { \
    if (((STATE)->top - (STATE)->stack) / 8 >= (STATE)->stack_size) { \
        (STATE)->stack_size *= 2; \
        (STATE)->stack = realloc((STATE)->stack, (STATE)->stack_size * sizeof(lfValue)); \
    } \
}

#define LF_STACKSIZE(STATE) ((STATE)->top - (STATE)->base)

typedef struct lfState {
    size_t stack_size;
    lfValue *stack;

    lfValue *base;
    lfValue *top;

    lfValueMap globals;

    bool errored;
    jmp_buf error_buf;
} lfState;

lfState *lf_state_create(void);
void lf_state_delete(lfState *state);

void lf_setglobal(lfState *state, const char *key);
void lf_getglobal(lfState *state, const char *key);

void lf_pushint(lfState *state, uint64_t value);
void lf_pushstring(lfState *state, char *value, size_t length);
void lf_pushbool(lfState *state, bool value);
void lf_pushnull(lfState *state);
void lf_pushlfstring(lfState *state, lfArray(char) value); /* no-clone version, requires an lfArray */
lfValue lf_pop(lfState *state);
void lf_push(lfState *state, const lfValue *value);

#endif /* LEAF_STATE_H */
