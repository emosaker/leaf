/*
 * This file is part of the leaf programming language
 */

#include <stdlib.h>
#include "lib/array.h"
#include "vm/value.h"
#include "vm/builtins.h"

lfState *lf_state_create(void) {
    lfState *state = malloc(sizeof(lfState));
    state->stack_size = 256;
    state->stack = malloc(state->stack_size * sizeof(lfValue));
    state->base = state->stack;
    state->top = state->stack;
    state->globals = lf_valuemap_create(512);
    state->errored = false;
    state->upvalues = NULL;

    /* register builtins */
    lf_newccl(state, lf_print);
    lf_setsglobal(state, "print");

    return state;
}

void lf_state_delete(lfState *state) {
    for (size_t i = 0; i < LF_STACKSIZE(state); i++) {
        lf_value_deleter(state->stack + i);
    }
    array_delete(&state->globals);
    free(state->stack);
    free(state);
}
