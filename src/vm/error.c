/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdlib.h>

#include "vm/value.h"
#include "vm/error.h"

void error_jumpout(lfState *state) {
    state->errored = true;
    /* TODO: Cleanup etc */
    longjmp(state->error_buf, 1);
}

void lf_errorf(lfState *state, const char *fmt, ...) {
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    size_t length = vsnprintf(NULL, 0, fmt, args1) + 1;
    char *buf = malloc(length);
    va_end(args1);
    vsnprintf(buf, length, fmt, args2);
    va_end(args2);

    printf("runtime error: %s\n", buf);
    free(buf);

    error_jumpout(state);
}

void lf_error(lfState *state, const char *message) {
    printf("runtime error: %s\n", message);
    error_jumpout(state);
}
