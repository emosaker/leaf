/*
 * This file is part of the leaf programming language
 */

#include "compiler/compile.h" /* TODO: don't use compiler here, instead load serialized bytecode */
#include "compiler/bytecode.h"
#include "vm/value.h"
#include "vm/load.h"

bool lf_load(lfState *state, const char *source, const char *file) {
    lfProto *chunk = lf_compile(source, file);
    if (chunk == NULL) {
        return false;
    }
    lf_newlfcl(state, chunk);
    lf_proto_deleter(&chunk);
    return true;
}
