/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_COMPILE_H
#define LEAF_COMPILE_H

#include "lib/array.h"
#include "compiler/bytecode.h"
#include "compiler/variablemap.h"

typedef struct lfUpValue {
    enum {
        UVT_REF,
        UVT_IDX
    } by;
    uint32_t index;
} lfUpValue;

typedef struct lfStackFrame {
    lfVariableMap scope;
    lfArray(lfUpValue) upvalues;
} lfStackFrame;

typedef struct lfCompilerCtx {
    const char *file;
    const char *source;
    size_t top; /* stack top */
    bool discarded; /* whether the value of the currently compiled expression will be discarded */
    bool isclass; /* whether the compiler is compiling the body of a class */
    lfArray(lfProto) protos;
    lfArray(char *) strings;
    lfArray(uint64_t) ints;
    lfArray(uint8_t) current;
    lfVariableMap scope;
    lfArray(lfStackFrame *) fnstack;
} lfCompilerCtx;

lfChunk *lf_compile(const char *source, const char *file);
void lf_chunk_delete(lfChunk *chunk);

#endif /* LEAF_COMPILE_H */
