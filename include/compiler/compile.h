/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_COMPILE_H
#define LEAF_COMPILE_H

#include "compiler/bytecodebuilder.h"
#include "lib/array.h"
#include "compiler/bytecode.h"
#include "compiler/variablemap.h"

typedef struct lfStackFrame {
    lfVariableMap scope;
    lfArray(lfUpValue) upvalues;
} lfStackFrame;

typedef struct lfCompilerCtx {
    const char *file;
    const char *source;
    int top; /* stack top */
    bool discarded; /* whether the value of the currently compiled expression will be discarded */
    bool isclass; /* whether the compiler is compiling the body of a class */
    lfVariableMap scope;
    lfArray(lfStackFrame *) fnstack;
    lfBytecodeBuilder bb;
} lfCompilerCtx;

lfProto *lf_compile(const char *source, const char *file);

#endif /* LEAF_COMPILE_H */
