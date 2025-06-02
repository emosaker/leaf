/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_COMPILE_H
#define LEAF_COMPILE_H

#include "lib/array.h"
#include "compiler/bytecode.h"
#include "compiler/variablemap.h"

typedef struct lfCompilerCtx {
    const char *file;
    const char *source;
    size_t top; /* stack top */
    lfArray(lfProto) protos;
    lfArray(char *) strings;
    lfArray(uint64_t) ints;
    lfArray(uint8_t) current;
    lfVariableMap scope;
} lfCompilerCtx;

lfChunk *lf_compile(const char *source, const char *file);
void lf_chunk_delete(lfChunk *chunk);

#endif /* LEAF_COMPILE_H */
