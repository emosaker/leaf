/*
 * This file is part of the leaf programming language
 */

#include "vm/state.h"
#include "vm/value.h"

void lf_pushint(lfState *state, uint64_t value) {
    LF_CHECKTOP(state);
    *state->top++ = (lfValue) {
        .type = LF_INT,
        .v.integer = value
    };
}

uint64_t lf_popint(lfState *state) {
    return (*--state->top).v.integer;
}
