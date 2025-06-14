/*
 * This file is part of the leaf programming language
 */

#include <stdlib.h>
#include "vm/value.h"
#include "vm/builtins.h"

lfState *lf_state_create(void) {
    lfState *state = malloc(sizeof(lfState));
    state->stack_size = 256;
    state->stack = malloc(state->stack_size * sizeof(lfValue));
    state->base = state->stack;
    state->top = state->stack;
    state->globals = lf_valuemap_create(128);
    state->strays = lf_valuemap_create(16); /* lfClosure -> lfValueArray map for upvalues that go out of scope */
    state->errored = false;
    state->upvalues = NULL;
    state->gc_objects = NULL;
    state->gray_objects = NULL;

    /* register builtins */
    lf_newccl(state, lf_print);
    lf_setsglobal(state, "print");

    return state;
}

void lf_state_delete(lfState *state) {
    lf_valuemap_delete(&state->globals);
    state->globals = NULL;
    lf_valuemap_delete(&state->strays);
    state->strays = NULL;
    while (state->gray_objects || state->gc_objects)
        lf_gc_step(state);
    free(state->stack);
    free(state);
}
