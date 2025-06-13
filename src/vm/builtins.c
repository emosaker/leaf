/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>

#include "vm/value.h"
#include "vm/builtins.h"
#include "lib/array.h"

int lf_print(lfState *state) {
    size_t nargs = LF_STACKSIZE(state);
    lfArray(lfValue) values = array_new(lfValue);
    array_reserve(&values, nargs);
    length(&values) = nargs;
    for (int i = 0; i < nargs; i++) {
        values[nargs - i - 1] = lf_pop(state);
    }
    for (int i = 0; i < nargs; i++) {
        lf_printvalue(values + i);
        if (i < nargs - 1)
            printf(", ");
    }
    printf("\n");
    array_delete(&values);
    return 0;
}
