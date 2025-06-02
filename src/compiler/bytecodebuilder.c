/*
 * This file is part of the leaf programming language
 */

#include <string.h>
#include <stdlib.h>
#include "compiler/compile.h"
#include "compiler/bytecodebuilder.h"

void emit_insn_abc(lfCompilerCtx *ctx, lfOpCode op, uint8_t a, uint8_t b, uint8_t c) {
    array_push(&ctx->current, op);
    array_push(&ctx->current, a);
    array_push(&ctx->current, b);
    array_push(&ctx->current, c);
}

void emit_insn_e(lfCompilerCtx *ctx, lfOpCode op, uint32_t e) {
    array_push(&ctx->current, op);
    emit_u24(ctx, e);
}

void emit_insn_e_at(lfCompilerCtx *ctx, lfOpCode op, uint32_t e, size_t idx) {
    size_t curr = length(&ctx->current);
    length(&ctx->current) = idx;
    emit_insn_e(ctx, op, e);
    length(&ctx->current) = curr;
}

void emit_op(lfCompilerCtx *ctx, lfOpCode op) {
    emit_insn_e(ctx, op, 0);
}

void emit_u24(lfCompilerCtx *ctx, uint32_t value) {
    array_push(&ctx->current, (value >> 0)  & 0xFF);
    array_push(&ctx->current, (value >> 8)  & 0xFF);
    array_push(&ctx->current, (value >> 16) & 0xFF);
}

uint32_t new_u64(lfCompilerCtx *ctx, uint64_t value) {
    array_push(&ctx->ints, value);
    return length(&ctx->ints) - 1;
}

uint32_t new_string(lfCompilerCtx *ctx, char *value, size_t length) {
    for (size_t i = 0; i < length(&ctx->strings); i++) { /* TODO: Replace with a map for O(1) insertion */
        if (!strncmp(ctx->strings[i], value, length))
            return i;
    }
    char *clone = malloc(length + 1);
    memcpy(clone, value, length);
    clone[length] = 0;
    array_push(&ctx->strings, clone);
    return length(&ctx->strings) - 1;
}
