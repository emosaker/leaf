/*
 * This file is part of the leaf programming language
 */

#include <stdlib.h>
#include "vm/state.h"
#include "vm/value.h"

lfState *lf_state_create(void) {
    lfState *state = malloc(sizeof(lfState));
    state->stack_size = 256;
    state->stack = malloc(state->stack_size * sizeof(lfValue));
    state->base = state->stack;
    state->top = state->stack;
    state->errored = false;
    return state;
}

void lf_state_delete(lfState *state) {
    for (size_t i = 0; i < LF_STACKSIZE(state); i++) {
        lf_deletevalue(state->stack + i);
    }
    free(state->stack);
    free(state);
}
