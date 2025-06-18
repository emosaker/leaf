/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>

#include "lib/array.h"
#include "vm/value.h"
#include "vm/vm.h"
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

int lf_input(lfState *state) {
    if (LF_STACKSIZE(state) > 0) {
        lf_printvalue(state->base);
    }
    lfArray(char) buf = array_new(char);
    char next;
    while ((next = getchar()) != '\n') {
        array_push(&buf, next);
    }
    array_push(&buf, 0);
    lf_pushstring(state, buf, length(&buf));
    array_delete(&buf);
    return 1;
}
