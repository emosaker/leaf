/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_BYTECODEBUILDER_H
#define LEAF_BYTECODEBUILDER_H

#include "compiler/compile.h"

void emit_op(lfCompilerCtx *ctx, lfOpCode op, int lineno);
void emit_ins_abc(lfCompilerCtx *ctx, lfOpCode op, uint8_t a, uint8_t b, uint8_t c, int lineno);
void emit_ins_ad(lfCompilerCtx *ctx, lfOpCode op, uint8_t a, uint16_t d, int lineno);
void emit_ins_e(lfCompilerCtx *ctx, lfOpCode op, uint32_t e, int lineno);
void emit_ins_e_at(lfCompilerCtx *ctx, lfOpCode op, uint32_t e, int idx, int lineno);
void emit_u24(lfCompilerCtx *ctx, uint32_t value);

uint32_t new_u64(lfCompilerCtx *ctx, uint64_t value);
uint32_t new_string(lfCompilerCtx *ctx, char *value, int length);

#endif /* LEAF_BYTECODEBUILDER_H */
