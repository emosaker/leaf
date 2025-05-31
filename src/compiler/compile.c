/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "lib/array.h"
#include "compiler/bytecode.h"
#include "compiler/compile.h"
#include "parser/node.h"
#include "parser/parse.h"

typedef struct lfCompilerCtx {
    lfArray(lfProto) protos;
    lfArray(char *) strings;
    lfArray(int64_t) ints;
    lfArray(uint8_t) current;
} lfCompilerCtx;

void proto_deleter(lfProto *proto) {
    free(proto->code);
}

void string_deleter(char **string) {
    free(*string);
}

static inline void emit_op(lfCompilerCtx *ctx, lfOpCode op) {
    array_push(&ctx->current, op);
}

static inline void emit_i64(lfCompilerCtx *ctx, int64_t value) {
    array_push(&ctx->ints, value);
    size_t idx = length(&ctx->ints) - 1;
    array_push(&ctx->current, (idx >> 0)  & 0xFF);
    array_push(&ctx->current, (idx >> 8)  & 0xFF);
    array_push(&ctx->current, (idx >> 16) & 0xFF);
}

bool visit_int(lfCompilerCtx *ctx, lfLiteralNode *node) {
    uint64_t value = strtoll(node->value.value, NULL, 10);
    if (value >= 0xFFFFFF) { /* 2^24 */
        emit_op(ctx, OP_PUSHLI);
        emit_i64(ctx, value);
    } else {
        emit_op(ctx, OP_PUSHSI);
        array_push(&ctx->current, (value >> 0)  & 0xFF);
        array_push(&ctx->current, (value >> 8)  & 0xFF);
        array_push(&ctx->current, (value >> 16) & 0xFF);
    }
    return true;
}

bool visit(lfCompilerCtx *ctx, lfNode *node) {
    switch (node->type) {
        case NT_INT: return visit_int(ctx, (lfLiteralNode *)node);
        default:
            printf("unhandled: %d\n", node->type);
            return false;
    }
}

lfChunk *lf_compile(const char *source, const char *file) {
    lfNode *ast = lf_parse(source, file);
    if (ast == NULL) {
        return NULL;
    }

    lfCompilerCtx ctx = (lfCompilerCtx) {
        .protos = array_new(lfProto, proto_deleter),
        .strings = array_new(char *, string_deleter),
        .ints = array_new(int64_t),
        .current = array_new(uint8_t)
    };

    if (!visit(&ctx, ((lfCompoundNode *)ast)->statements[0])) {
        array_delete(&ctx.strings);
        array_delete(&ctx.protos);
        array_delete(&ctx.current);
        array_delete(&ctx.ints);
        lf_node_delete(ast);
        return NULL;
    }

    lfProto main = (lfProto) {
        .code = malloc(length(&ctx.current)),
        .szcode = length(&ctx.current) / 4,
        .name = 0
    };
    memcpy(main.code, ctx.current, length(&ctx.current));
    array_push(&ctx.protos, main);

    lfChunk *chunk = malloc(sizeof(lfChunk));
    chunk->protos = malloc(sizeof(lfProto) * length(&ctx.protos));
    chunk->szprotos = length(&ctx.protos);
    chunk->strings = malloc(sizeof(char *) * length(&ctx.strings));
    chunk->szstrings = length(&ctx.strings);
    chunk->ints = malloc(sizeof(int64_t) * length(&ctx.ints));
    chunk->szints = length(&ctx.ints);
    chunk->main = length(&ctx.protos) - 1;

    memcpy(chunk->protos, ctx.protos, sizeof(lfProto) * length(&ctx.protos));
    memcpy(chunk->strings, ctx.strings, sizeof(char *) * length(&ctx.strings));
    memcpy(chunk->ints, ctx.ints, sizeof(int64_t) * length(&ctx.ints));

    deleter(&ctx.strings) = NULL;
    deleter(&ctx.protos) = NULL;
    array_delete(&ctx.strings);
    array_delete(&ctx.protos);
    array_delete(&ctx.current);
    array_delete(&ctx.ints);
    lf_node_delete(ast);
    return chunk;
}

void lf_chunk_delete(lfChunk *chunk) {
    for (size_t i = 0; i < chunk->szprotos; i++)
        proto_deleter(chunk->protos + i);
    for (size_t i = 0; i < chunk->szstrings; i++)
        string_deleter(chunk->strings + i);
    free(chunk->protos);
    free(chunk->strings);
    free(chunk->ints);
    free(chunk);
}
