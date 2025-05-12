/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <stdbool.h>

#include "parser/node.h"
#include "parser/parse.h"
#include "parser/token.h"
#include "lib/array.h"
#include "lib/error.h"

#define alloc(TYPE) (TYPE *)malloc(sizeof(TYPE))

typedef struct lfParseCtx {
    size_t current_idx;
    lfToken current;
    const lfArray(lfToken) tokens;
    bool errored; /* whether the current context ran into a syntax error */
    bool described; /* whether an error has been printed */
    /* strictly for error messages */
    const char *file;
    const char *source;
} lfParseCtx;

void advance(lfParseCtx *ctx) {
    ctx->current_idx += 1;
    ctx->current = ctx->tokens[ctx->current_idx];
}

lfNode *parse_expr(lfParseCtx *ctx);

lfNode *lf_literal(lfParseCtx *ctx) {
    if (ctx->current.type == TT_INT || ctx->current.type == TT_FLOAT || ctx->current.type == TT_STRING) {
        lfLiteralNode *literal = alloc(lfLiteralNode);
        literal->type = ctx->current.type == TT_INT ? NT_INT : ctx->current.type == TT_FLOAT ? NT_FLOAT : NT_STRING;
        literal->value = ctx->current;
        advance(ctx);
        return (lfNode *)literal;
    } else if (ctx->current.type == TT_LPAREN) {
        lfToken lparen = ctx->current;
        advance(ctx);
        lfNode *expr = parse_expr(ctx);
        if (ctx->errored) return NULL;
        if (ctx->current.type != TT_RPAREN) {
            lf_node_delete(expr);
            error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected ')'");
            error_print(ctx->file, ctx->source, lparen.idx_start, lparen.idx_end, "to match");
            ctx->errored = true;
            ctx->described = true;
            return NULL;
        }
        advance(ctx);
        return expr;
    } else if (ctx->current.type == TT_SUB) {
        lfToken op = ctx->current;
        advance(ctx);
        lfNode *expr = parse_expr(ctx);
        if (ctx->errored) {
            return NULL;
        }
        lfUnaryOpNode *unop = alloc(lfUnaryOpNode);
        unop->type = NT_UNARYOP;
        unop->op = op;
        unop->value = expr;
        return (lfNode *)unop;
    }

    ctx->errored = true;
    ctx->described = false;
    return NULL;
}

lfNode *lf_multiplicative(lfParseCtx *ctx) {
    lfNode *lhs = lf_literal(ctx);
    if (ctx->errored) return NULL;
    while (ctx->current.type == TT_MUL || ctx->current.type == TT_DIV || ctx->current.type == TT_POW) {
        lfToken op = ctx->current;
        advance(ctx);
        lfNode *rhs = lf_literal(ctx);
        if (ctx->errored) {
            lf_node_delete(lhs);
            return NULL;
        }
        lfBinaryOpNode *binop = alloc(lfBinaryOpNode);
        binop->type = NT_BINARYOP;
        binop->lhs = lhs;
        binop->rhs = rhs;
        binop->op = op;
        lhs = (lfNode *)binop;
    }
    return lhs;
}

lfNode *parse_additive(lfParseCtx *ctx) {
    lfNode *lhs = lf_multiplicative(ctx);
    if (ctx->errored) return NULL;
    while (ctx->current.type == TT_ADD || ctx->current.type == TT_SUB) {
        lfToken op = ctx->current;
        advance(ctx);
        lfNode *rhs = lf_multiplicative(ctx);
        if (ctx->errored) {
            lf_node_delete(lhs);
            return NULL;
        }
        lfBinaryOpNode *binop = alloc(lfBinaryOpNode);
        binop->type = NT_BINARYOP;
        binop->lhs = lhs;
        binop->rhs = rhs;
        binop->op = op;
        lhs = (lfNode *)binop;
    }
    return lhs;
}

lfNode *parse_expr(lfParseCtx *ctx) {
    return parse_additive(ctx);
}

lfNode *lf_parse(const lfArray(lfToken) tokens, const char *source, const char *file) {
    if (length(&tokens) == 0) {
        return NULL;
    }
    lfParseCtx ctx = (lfParseCtx) {
        .tokens = tokens,
        .current_idx = 0,
        .current = tokens[0],
        .file = file,
        .source = source,
        .errored = false,
        .described = false
    };

    lfNode *expr = parse_expr(&ctx);
    if (ctx.errored && !ctx.described) {
        error_print(file, source, ctx.current.idx_start, ctx.current.idx_end, "expected expression");
        return NULL;
    }

    return expr;
}

const char *op_to_string(lfTokenType type) {
    switch (type) {
        case TT_ADD: return "+";
        case TT_SUB: return "-";
        case TT_MUL: return "*";
        case TT_DIV: return "/";
        default: return "?";
    }
}

void lf_node_print(lfNode *node) {
    switch (node->type) {
        case NT_UNARYOP:
            printf("%s(", ((lfUnaryOpNode *)node)->op.value);
            lf_node_print(((lfUnaryOpNode *)node)->value);
            printf(")");
            break;
        case NT_BINARYOP:
            printf("(");
            lf_node_print(((lfBinaryOpNode *)node)->lhs);
            printf(" %s ", op_to_string(((lfUnaryOpNode *)node)->op.type));
            lf_node_print(((lfBinaryOpNode *)node)->rhs);
            printf(")");
            break;
        case NT_INT:
        case NT_FLOAT:
        case NT_STRING:
            printf("%s", ((lfLiteralNode *)node)->value.value);
            break;
    }
}

void lf_node_delete(lfNode *node) {
    switch (node->type) {
        case NT_UNARYOP:
            lf_node_delete(((lfUnaryOpNode *)node)->value);
            free(node);
            break;
        case NT_BINARYOP:
            lf_node_delete(((lfBinaryOpNode *)node)->lhs);
            lf_node_delete(((lfBinaryOpNode *)node)->rhs);
            free(node);
            break;
        case NT_INT:
        case NT_FLOAT:
        case NT_STRING:
            free(node);
            break;
    }
}
