/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_BYTECODEBUILDER_H
#define LEAF_BYTECODEBUILDER_H

#include "compiler/compile.h"

void emit_op(lfCompilerCtx *ctx, lfOpCode op);
void emit_insn_abc(lfCompilerCtx *ctx, lfOpCode op, uint8_t a, uint8_t b, uint8_t c);
void emit_insn_e(lfCompilerCtx *ctx, lfOpCode op, uint32_t e);
void emit_u24(lfCompilerCtx *ctx, uint32_t value);

uint32_t new_u64(lfCompilerCtx *ctx, uint64_t value);
uint32_t new_string(lfCompilerCtx *ctx, char *value, size_t length);

#endif /* LEAF_BYTECODEBUILDER_H */
