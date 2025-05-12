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

void lf_node_deleter(lfNode **node) { /* for use with the lfArray type */
    lf_node_delete(*node);
}

lfNode *parse_expr(lfParseCtx *ctx);

lfNode *parse_literal(lfParseCtx *ctx) {
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
            error_print(ctx->file, ctx->source, lparen.idx_start, lparen.idx_end, "... to close");
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
    } else if (ctx->current.type == TT_IDENTIFIER) {
        lfVarAccessNode *access = alloc(lfVarAccessNode);
        access->type = NT_VARACCESS;
        access->var = ctx->current;
        advance(ctx);
        return (lfNode *)access;
    }

    ctx->errored = true;
    ctx->described = false;
    return NULL;
}

lfNode *parse_subscriptive(lfParseCtx *ctx) {
    lfNode *object = parse_literal(ctx);
    if (ctx->errored) return NULL;
    while (ctx->current.type == TT_LBRACKET || ctx->current.type == TT_DOT || ctx->current.type == TT_LPAREN) {
        if (ctx->current.type == TT_LBRACKET) {
            lfToken lbracket = ctx->current;
            advance(ctx);
            lfNode *index = parse_expr(ctx);
            if (ctx->errored) {
                lf_node_delete(object);
                return NULL;
            }
            if (ctx->current.type != TT_RBRACKET) {
                lf_node_delete(object);
                error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected ']'");
                error_print(ctx->file, ctx->source, lbracket.idx_start, lbracket.idx_end, "... to close");
                ctx->errored = true;
                ctx->described = true;
                return NULL;
            }
            advance(ctx);
            lfSubscriptionNode *sub = alloc(lfSubscriptionNode);
            sub->type = NT_SUBSCRIBE;
            sub->object = object;
            sub->index = index;
            object = (lfNode *)sub;
        } else if (ctx->current.type == TT_DOT) {
            advance(ctx);
            if (ctx->current.type != TT_IDENTIFIER) {
                lf_node_delete(object);
                error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected identifier");
                ctx->errored = true;
                ctx->described = true;
                return NULL;
            }
            lfLiteralNode *index = alloc(lfLiteralNode);
            index->type = NT_STRING;
            index->value = ctx->current;
            advance(ctx);
            lfSubscriptionNode *sub = alloc(lfSubscriptionNode);
            sub->type = NT_SUBSCRIBE;
            sub->object = object;
            sub->index = (lfNode *)index;
            object = (lfNode *)sub;
        } else {
            lfToken lparen = ctx->current;
            lfArray(lfNode *) args = array_new(lfNode *, lf_node_deleter);
            advance(ctx);
            if (ctx->current.type != TT_RPAREN) {
                do {
                    lfNode *arg = parse_expr(ctx);
                    if (ctx->errored) {
                        lf_node_delete(object);
                        return NULL;
                    }
                    array_push(&args, arg);
                } while (ctx->current.type == TT_COMMA);
            }
            if (ctx->current.type != TT_RPAREN) {
                lf_node_delete(object);
                error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected ')'");
                error_print(ctx->file, ctx->source, lparen.idx_start, lparen.idx_end, "... to close");
                ctx->errored = true;
                ctx->described = true;
                return NULL;
            }
            advance(ctx);
            lfCallNode *call = alloc(lfCallNode);
            call->type = NT_CALL;
            call->func = object;
            call->args = args;
            object = (lfNode *)call;
        }
    }

    return object;
}

lfNode *parse_bitwise(lfParseCtx *ctx) {
    lfNode *lhs = parse_subscriptive(ctx);
    if (ctx->errored) return NULL;
    while (ctx->current.type == TT_LSHIFT || ctx->current.type == TT_RSHIFT) {
        lfToken op = ctx->current;
        advance(ctx);
        lfNode *rhs = parse_subscriptive(ctx);
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

lfNode *parse_multiplicative(lfParseCtx *ctx) {
    lfNode *lhs = parse_bitwise(ctx);
    if (ctx->errored) return NULL;
    while (ctx->current.type == TT_MUL || ctx->current.type == TT_DIV || ctx->current.type == TT_POW) {
        lfToken op = ctx->current;
        advance(ctx);
        lfNode *rhs = parse_bitwise(ctx);
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
    lfNode *lhs = parse_multiplicative(ctx);
    if (ctx->errored) return NULL;
    while (ctx->current.type == TT_ADD || ctx->current.type == TT_SUB) {
        lfToken op = ctx->current;
        advance(ctx);
        lfNode *rhs = parse_multiplicative(ctx);
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

lfNode *parse_comparative(lfParseCtx *ctx) {
    lfNode *lhs = parse_additive(ctx);
    if (ctx->errored) return NULL;
    while (
        ctx->current.type == TT_EQ || ctx->current.type == TT_NE ||
        ctx->current.type == TT_LT || ctx->current.type == TT_GT ||
        ctx->current.type == TT_LE || ctx->current.type == TT_GE
    ) {
        lfToken op = ctx->current;
        advance(ctx);
        lfNode *rhs = parse_additive(ctx);
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
    lfNode *expr = parse_comparative(ctx);
    if (ctx->errored && !ctx->described) {
        error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected expression");
        ctx->described = true;
    }
    return expr;
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
        case TT_EQ: return "==";
        case TT_NE: return "!=";
        case TT_LT: return "<";
        case TT_GT: return ">";
        case TT_LE: return "<=";
        case TT_GE: return ">=";
        case TT_LSHIFT: return "<<";
        case TT_RSHIFT: return ">>";
        default: return "?";
    }
}

void lf_node_print(lfNode *node) {
    switch (node->type) {
        case NT_UNARYOP: {
            lfUnaryOpNode *unop = (lfUnaryOpNode *)node;
            printf("%s(", unop->op.value);
            lf_node_print(unop->value);
            printf(")");
        } break;
        case NT_BINARYOP: {
            lfBinaryOpNode *binop = (lfBinaryOpNode *)node;
            printf("(");
            lf_node_print(binop->lhs);
            printf(" %s ", op_to_string(binop->op.type));
            lf_node_print(binop->rhs);
            printf(")");
        } break;
        case NT_VARDECL: {
            lfVarDeclNode *decl = (lfVarDeclNode *)node;
            printf("%s decl(%s", decl->is_const ? "const" : decl->is_ref ? "ref" : "var", decl->name.value);
            if (decl->initializer) {
                printf(" = ");
                lf_node_print(decl->initializer);
            }
            printf(")");
        } break;
        case NT_VARACCESS: {
            lfVarAccessNode *access = (lfVarAccessNode *)node;
            printf("var(%s)", access->var.value);
        } break;
        case NT_SUBSCRIBE: {
            lfSubscriptionNode *sub = (lfSubscriptionNode *)node;
            lf_node_print(sub->object);
            printf("[");
            lf_node_print(sub->index);
            printf("]");
        } break;
        case NT_CALL: {
            lfCallNode *call = (lfCallNode *)node;
            lf_node_print(call->func);
            printf("(");
            for (size_t i = 0; i < length(&call->args); i++) {
                lf_node_print(call->args[i]);
                if (i < length(&call->args) - 1) {
                    printf(", ");
                }
            }
            printf(")");
        } break;
        case NT_INT:
        case NT_FLOAT:
        case NT_STRING: {
            lfLiteralNode *literal = (lfLiteralNode *)node;
            printf("literal(%s)", literal->value.value);
        } break;
    }
}

void lf_node_delete(lfNode *node) {
    switch (node->type) {
        case NT_UNARYOP: {
            lfUnaryOpNode *unop = (lfUnaryOpNode *)node;
            lf_node_delete(unop->value);
            free(node);
        } break;
        case NT_BINARYOP: {
            lfBinaryOpNode *binop = (lfBinaryOpNode *)node;
            lf_node_delete(binop->lhs);
            lf_node_delete(binop->rhs);
            free(node);
        } break;
        case NT_VARDECL: {
            lfVarDeclNode *decl = (lfVarDeclNode *)node;
            if (decl->initializer)
                lf_node_delete(decl->initializer);
            free(node);
        } break;
        case NT_CALL: {
            lfCallNode *call = (lfCallNode *)node;
            lf_node_delete(call->func);
            array_delete(&call->args);
            free(node);
        } break;
        case NT_SUBSCRIBE: {
            lfSubscriptionNode *sub = (lfSubscriptionNode *)node;
            lf_node_delete(sub->object);
            lf_node_delete(sub->index);
            free(node);
        } break;
        case NT_INT:
        case NT_FLOAT:
        case NT_VARACCESS:
        case NT_STRING:
            free(node);
            break;
    }
}
