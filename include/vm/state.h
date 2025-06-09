/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_STATE_H
#define LEAF_STATE_H

#include <stdlib.h>
#include <setjmp.h>
#include <stdbool.h>

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

    bool errored;
    jmp_buf error_buf;
} lfState;

lfState *lf_state_create(void);
void lf_state_delete(lfState *state);

void lf_pushint(lfState *state, uint64_t value);
uint64_t lf_popint(lfState *state);

#endif /* LEAF_STATE_H */
