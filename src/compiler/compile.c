/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "compiler/variablemap.h"
#include "lib/array.h"
#include "compiler/bytecode.h"
#include "compiler/compile.h"
#include "compiler/bytecodebuilder.h"
#include "lib/error.h"
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
    ctx->top += 1;
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

    ctx->top -= 1;
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

bool visit_vardecl(lfCompilerCtx *ctx, lfVarDeclNode *node) {
    if (variablemap_lookup(&ctx->scope, node->name.value, NULL)) {
        error_print(ctx->file, ctx->source, node->name.idx_start, node->name.idx_end, "variable name already in use");
        return false;
    }

    if (node->initializer) {
        if (!visit(ctx, node->initializer))
            return false;
    } else emit_op(ctx, OP_PUSHNULL);

    variablemap_insert(&ctx->scope, node->name.value, ctx->top - 1);

    return true;
}

bool visit_varaccess(lfCompilerCtx *ctx, lfVarAccessNode *node) {
    uint32_t index;
    if (variablemap_lookup(&ctx->scope, node->var.value, &index)) {
        emit_insn_e(ctx, OP_DUP, index);
    } else {
        emit_insn_e(ctx, OP_GETGLOBAL, new_string(ctx, node->var.value, strlen(node->var.value)));
    }
    ctx->top += 1;
    return true;
}

bool visit_array(lfCompilerCtx *ctx, lfArrayNode *node) {
    for (size_t i = 0; i < length(&node->values); i++)
        if (!visit(ctx, node->values[i]))
            return false;
    emit_insn_e(ctx, OP_NEWARR, length(&node->values));
    ctx->top -= length(&node->values) - 1;
    return true;
}

bool visit_map(lfCompilerCtx *ctx, lfMapNode *node) {
    for (size_t i = 0; i < length(&node->values); i++) {
        if (!visit(ctx, node->keys[i]))
            return false;
        if (!visit(ctx, node->values[i]))
            return false;
    }
    emit_insn_e(ctx, OP_NEWMAP, length(&node->values));
    ctx->top -= length(&node->values) * 2 - 1;
    return true;
}

bool visit_subscribe(lfCompilerCtx *ctx, lfSubscriptionNode *node) {
    if (!visit(ctx, node->object)) return false;
    if (!visit(ctx, node->index)) return false;
    emit_op(ctx, OP_INDEX);
    ctx->top -= 1;
    return true;
}

bool visit_assign(lfCompilerCtx *ctx, lfAssignNode *node) {
    if (!visit(ctx, node->value)) return false;
    uint32_t index;
    if (variablemap_lookup(&ctx->scope, node->variable.value, &index)) {
        emit_insn_e(ctx, OP_ASSIGN, index);
    } else {
        emit_insn_e(ctx, OP_SETGLOBAL, new_string(ctx, node->variable.value, strlen(node->variable.value)));
    }
    ctx->top -= 1;
    return true;
}

bool visit_objassign(lfCompilerCtx *ctx, lfObjectAssignNode *node) {
    if (!visit(ctx, node->object)) return false;
    if (!visit(ctx, node->key)) return false;
    if (!visit(ctx, node->value)) return false;
    emit_op(ctx, OP_SET);
    ctx->top -= 2;
    return true;
}

bool visit_if(lfCompilerCtx *ctx, lfIfNode *node) {
    if (!visit(ctx, node->condition)) return false;
    size_t idx = length(&ctx->current);
    emit_op(ctx, OP_NOP); /* replaced later */
    if (!visit(ctx, node->body)) return false;
    size_t curr = length(&ctx->current);
    emit_insn_e_at(ctx, OP_JMPIFNOT, (curr - idx) / 4 - 1 + (node->else_body != NULL ? 1 : 0), idx);
    if (node->else_body != NULL) {
        idx = length(&ctx->current);
        emit_op(ctx, OP_NOP); /* replaced later */
        if (!visit(ctx, node->else_body)) return false;
        curr = length(&ctx->current);
        emit_insn_e_at(ctx, OP_JMP, (curr - idx) / 4 - 1, idx);
    }
    return true;
}

bool visit_compound(lfCompilerCtx *ctx, lfCompoundNode *node) {
    for (size_t i = 0; i < length(&node->statements); i++)
        if (!visit(ctx, node->statements[i]))
            return false;
    return true;
}

bool visit(lfCompilerCtx *ctx, lfNode *node) {
    switch (node->type) {
        case NT_INT: return visit_int(ctx, (lfLiteralNode *)node);
        case NT_STRING: return visit_string(ctx, (lfLiteralNode *)node);
        case NT_BINARYOP: return visit_binop(ctx, (lfBinaryOpNode *)node);
        case NT_UNARYOP: return visit_unop(ctx, (lfUnaryOpNode *)node);
        case NT_COMPOUND: return visit_compound(ctx, (lfCompoundNode *)node);
        case NT_VARDECL: return visit_vardecl(ctx, (lfVarDeclNode *)node);
        case NT_VARACCESS: return visit_varaccess(ctx, (lfVarAccessNode *)node);
        case NT_ARRAY: return visit_array(ctx, (lfArrayNode *)node);
        case NT_MAP: return visit_map(ctx, (lfMapNode *)node);
        case NT_SUBSCRIBE: return visit_subscribe(ctx, (lfSubscriptionNode *)node);
        case NT_ASSIGN: return visit_assign(ctx, (lfAssignNode *)node);
        case NT_OBJASSIGN: return visit_objassign(ctx, (lfObjectAssignNode *)node);
        case NT_IF: return visit_if(ctx, (lfIfNode *)node);
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
        .file = file,
        .source = source,
        .top = 0,
        .protos = array_new(lfProto, proto_deleter),
        .strings = array_new(char *, string_deleter),
        .ints = array_new(uint64_t),
        .current = array_new(uint8_t),
        .scope = variablemap_create(256)
    };

    if (!visit(&ctx, ast)) {
        array_delete(&ctx.strings);
        array_delete(&ctx.protos);
        array_delete(&ctx.current);
        array_delete(&ctx.ints);
        array_delete(&ctx.scope);
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
    array_delete(&ctx.scope);
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
