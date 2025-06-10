/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "compiler/compile.h"
#include "lib/ansi.h"
#include "vm/state.h"
#include "vm/value.h"
#include "vm/vm.h"

#define FATAL FG_RED BOLD "fatal: " RESET

int main(int argc, const char **argv) {
    if (argc < 2) {
        fprintf(stderr, FATAL "no file provided\nsyntax: %s <file>\n", argv[0]);
        return 1;
    }

    const char *file = argv[1];

    FILE *f = fopen(file, "rb");
    if (f == NULL) {
        fprintf(stderr, FATAL "failed to open file %s\n", file);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    rewind(f);

    char *buffer = malloc(sz + 1);
    if (fread(buffer, 1, sz, f) != sz) {
        fprintf(stderr, FATAL "failed to read file %s\n", file);
        fclose(f);
        free(buffer);
        return 1;
    }
    buffer[sz] = 0;

    lfProto *chunk = lf_compile(buffer, file);
    if (chunk == NULL) {
        free(buffer);
        return 1;
    }

    lfState *state = lf_state_create();

    lf_run(state, chunk);
    if (!state->errored && LF_STACKSIZE(state) > 0) {
        lfValue top = lf_pop(state);
        lf_printvalue(&top);
        printf("\n");
        lf_deletevalue(&top);
    }

    lf_state_delete(state);
    lf_proto_deleter(&chunk);
    free(buffer);
    return 0;
}
