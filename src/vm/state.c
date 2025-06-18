/*
 * This file is part of the leaf programming language
 */

#include <stdlib.h>
#include "lib/array.h"
#include "vm/value.h"
#include "vm/builtins.h"

void gco_deleter(lfGCObject **gco) {
    switch ((*gco)->type) {
        case LF_CLOSURE:
            if (!((lfClosure *)*gco)->is_c)
                lf_proto_deleter(&((lfClosure *)*gco)->f.lf.proto);
            break;
        case LF_ARRAY:
            array_delete(&((lfValueArray *)*gco)->values);
            break;
        default:
            break;
    }
    free(*gco);
}

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
    state->gc_objects = array_new(lfGCObject *, gco_deleter);
    state->frame = array_new(lfCallFrame);

    /* register builtins */
    lf_newccl(state, lf_print, "print");
    lf_setcsglobal(state, "print");

    return state;
}

void lf_state_delete(lfState *state) {
    lf_valuemap_delete(&state->globals);
    state->globals = NULL;
    lf_valuemap_delete(&state->strays);
    state->strays = NULL;
    array_delete(&state->gc_objects);
    array_delete(&state->frame);
    free(state->stack);
    free(state);
}
