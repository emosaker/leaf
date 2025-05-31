/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_COMPILE_H
#define LEAF_COMPILE_H

#include "compiler/bytecode.h"

lfChunk *lf_compile(const char *source, const char *file);
void lf_chunk_delete(lfChunk *chunk);

#endif /* LEAF_COMPILE_H */
