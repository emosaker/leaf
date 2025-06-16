/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>

#include "vm/value.h"
#include "vm/builtins.h"

int lf_print(lfState *state) {
    int nargs = LF_STACKSIZE(state);
    for (int i = 0; i < nargs; i++) {
        lf_printvalue(state->base + i);
        if (i < nargs - 1)
            printf(", ");
    }
    printf("\n");
    return 0;
}
