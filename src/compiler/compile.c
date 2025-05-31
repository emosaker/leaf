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
#include "compiler/util.h"
#include "parser/node.h"
#include "parser/parse.h"
#include "parser/token.h"

void proto_deleter(lfProto *proto) {
    free(proto->code);
}

void string_deleter(char **string) {
    free(*string);
}

bool visit(lfCompilerCtx *ctx, lfNode *node);

bool visit_string(lfCompilerCtx *ctx, lfLiteralNode *node) {
    emit_insn_e(ctx, OP_PUSHS, new_string(ctx, node->value.value, strlen(node->value.value)));
    return true;
}

bool visit_int(lfCompilerCtx *ctx, lfLiteralNode *node) {
    uint64_t value = strtoll(node->value.value, NULL, 10);
    if (value >= 0xFFFFFF) { /* 2^24 */
        emit_insn_e(ctx, OP_PUSHLI, new_u64(ctx, value));
    } else {
        emit_insn_e(ctx, OP_PUSHSI, value);
    }
    return true;
}

bool visit_binop(lfCompilerCtx *ctx, lfBinaryOpNode *node) {
    if (!visit(ctx, node->lhs)) return false;
    if (!visit(ctx, node->rhs)) return false;
    switch (node->op.type) {
        case TT_ADD: emit_op(ctx, OP_ADD); break;
        case TT_SUB: emit_op(ctx, OP_SUB); break;
        case TT_MUL: emit_op(ctx, OP_MUL); break;
        case TT_DIV: emit_op(ctx, OP_DIV); break;
        case TT_EQ: emit_op(ctx, OP_EQ); break;
        case TT_NE: emit_op(ctx, OP_NE); break;
        case TT_LT: emit_op(ctx, OP_LT); break;
        case TT_GT: emit_op(ctx, OP_GT); break;
        case TT_LE: emit_op(ctx, OP_LE); break;
        case TT_BAND: emit_op(ctx, OP_BAND); break;
        case TT_BOR: emit_op(ctx, OP_BOR); break;
        case TT_BXOR: emit_op(ctx, OP_BXOR); break;
        case TT_LSHIFT: emit_op(ctx, OP_BLSH); break;
        case TT_RSHIFT: emit_op(ctx, OP_BRSH); break;
        case TT_AND: emit_op(ctx, OP_AND); break;
        case TT_OR: emit_op(ctx, OP_OR); break;
        default: /* unreachable for any AST produced by the parser */
            return false;
    }

    return true;
}

bool visit_unop(lfCompilerCtx *ctx, lfUnaryOpNode *node) {
    if (!visit(ctx, node->value)) return false;
    switch (node->op.type) {
        case TT_SUB: emit_op(ctx, OP_NEG); break;
        case TT_NOT: emit_op(ctx, OP_NOT); break;
        default: /* unreachable for any AST produced by the parser */
            return false;
    }

    return true;
}

bool visit(lfCompilerCtx *ctx, lfNode *node) {
    switch (node->type) {
        case NT_INT: return visit_int(ctx, (lfLiteralNode *)node);
        case NT_STRING: return visit_string(ctx, (lfLiteralNode *)node);
        case NT_BINARYOP: return visit_binop(ctx, (lfBinaryOpNode *)node);
        case NT_UNARYOP: return visit_unop(ctx, (lfUnaryOpNode *)node);
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
        .ints = array_new(uint64_t),
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
    chunk->ints = malloc(sizeof(uint64_t) * length(&ctx.ints));
    chunk->szints = length(&ctx.ints);
    chunk->main = length(&ctx.protos) - 1;

    memcpy(chunk->protos, ctx.protos, sizeof(lfProto) * length(&ctx.protos));
    memcpy(chunk->strings, ctx.strings, sizeof(char *) * length(&ctx.strings));
    memcpy(chunk->ints, ctx.ints, sizeof(uint64_t) * length(&ctx.ints));

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
