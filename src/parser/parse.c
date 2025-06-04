/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "parser/node.h"
#include "parser/parse.h"
#include "parser/token.h"
#include "lib/array.h"
#include "lib/error.h"
#include "parser/tokenize.h"

/* TODO: add error checking everywhere this is called */
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

typedef struct lfParseCtxState {
    size_t old_idx;
    lfToken old;
} lfParseCtxState;

void advance(lfParseCtx *ctx) {
    ctx->current_idx += 1;
    ctx->current = ctx->tokens[ctx->current_idx];
}

lfParseCtxState save(lfParseCtx *ctx) {
    return (lfParseCtxState) {
        .old_idx = ctx->current_idx,
        .old = ctx->current
    };
}

void restore(lfParseCtx *ctx, const lfParseCtxState *state) {
    ctx->current = state->old;
    ctx->current_idx = state->old_idx;
    ctx->errored = false;
    ctx->described = false;
}

lfNode *parse_expr(lfParseCtx *ctx);
lfType *parse_type(lfParseCtx *ctx);
lfNode *parse_statement(lfParseCtx *ctx);

lfType *parse_typename(lfParseCtx *ctx) {
    if (ctx->current.type != TT_IDENTIFIER) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected type");
        ctx->errored = true;
        ctx->described = true;
        return NULL;
    }
    lfTypeName *typename = alloc(lfTypeName);
    typename->type = VT_TYPENAME;
    typename->typename = ctx->current;
    advance(ctx);
    return (lfType *)typename;
}

lfType *parse_nontrivial_type(lfParseCtx *ctx) {
    if (ctx->current.type == TT_LBRACE) {
        lfArray(lfType *) keys = array_new(lfType *, lf_type_deleter);
        lfArray(lfType *) values = array_new(lfType *, lf_type_deleter);
        lfToken lbrace = ctx->current;
        advance(ctx);
        bool is_map = false;
        bool is_array = false;
        if (ctx->current.type != TT_RBRACE && ctx->current.type != TT_COMMA) {
            do {
                if (ctx->current.type == TT_COMMA) {
                    advance(ctx);
                }
                lfType *t = parse_type(ctx);
                if (ctx->errored) {
                    array_delete(&keys);
                    array_delete(&values);
                    return NULL;
                }
                if (ctx->current.type == TT_COLON) {
                    is_map = true;
                    array_push(&keys, t);
                    if (is_array) {
                        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "invalid ':' in array type");
                        ctx->errored = true;
                        ctx->described = true;
                        array_delete(&keys);
                        array_delete(&values);
                        return NULL;
                    }
                    advance(ctx);
                    lfType *v = parse_type(ctx);
                    if (ctx->errored) {
                        array_delete(&keys);
                        array_delete(&values);
                        return NULL;
                    }
                    array_push(&values, v);
                } else {
                    is_array = true;
                    array_push(&values, t);
                    if (is_map) {
                        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected ':' in map type");
                        ctx->errored = true;
                        ctx->described = true;
                        array_delete(&keys);
                        array_delete(&values);
                        return NULL;
                    }
                }
            } while (ctx->current.type == TT_COMMA);
        }
        if (ctx->current.type != TT_RBRACE) {
            lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected '}'");
            lf_error_print(ctx->file, ctx->source, lbrace.idx_start, lbrace.idx_end, "... to close");
            ctx->errored = true;
            ctx->described = true;
            array_delete(&keys);
            array_delete(&values);
            return NULL;
        }
        advance(ctx);
        if (is_map) {
            lfMapType *t = alloc(lfMapType);
            t->type = VT_MAP;
            t->keys = keys;
            t->values = values;
            return (lfType *)t;
        } else {
            array_delete(&keys);
            lfArrayType *t = alloc(lfArrayType);
            t->type = VT_ARRAY;
            t->values = values;
            return (lfType *)t;
        }
    } else if (ctx->current.type == TT_LPAREN) {
        lfToken lparen = ctx->current;
        advance(ctx);
        lfArray(lfType *) params = NULL;
        lfType *t = NULL;
        if (ctx->current.type != TT_RPAREN) {
            t = parse_type(ctx);
            if (ctx->errored) {
                return NULL;
            }
        }
        if (ctx->current.type == TT_COMMA) {
            params = array_new(lfType *, lf_type_deleter);
            array_push(&params, t);
            do {
                advance(ctx);
                t = parse_type(ctx);
                if (ctx->errored) {
                    array_delete(&params);
                    return NULL;
                }
                array_push(&params, t);
            } while (ctx->current.type == TT_COMMA);
        }
        if (ctx->current.type != TT_RPAREN) {
            lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected ')'");
            lf_error_print(ctx->file, ctx->source, lparen.idx_start, lparen.idx_end, "... to close");
            ctx->errored = true;
            ctx->described = true;
            if (params != NULL) {
                array_delete(params);
            } else {
                lf_type_deleter(&t);
            }
            return NULL;
        }
        advance(ctx);
        if (ctx->current.type == TT_ARROW) {
            advance(ctx);
            if (params == NULL) { /* turn our single type into a parameter list with one type */
                params = array_new(lfType *, lf_type_deleter);
                if (t) {
                    array_push(&params, t);
                }
            }
            lfType *ret = parse_type(ctx);
            if (ctx->errored) {
                array_delete(params);
                return NULL;
            }
            lfFuncType *f = alloc(lfFuncType);
            f->type = VT_FUNC;
            f->params = params;
            f->ret = ret;
            return (lfType *)f;
        } else if (params == NULL && t) {
            return t;
        } else {
            lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected '->'");
            ctx->errored = true;
            ctx->described = true;
            if (params != NULL) {
                array_delete(params);
            } else if (t) {
                lf_type_deleter(&t);
            }
            return NULL;
        }
    }

    return parse_typename(ctx);
}

lfType *parse_type(lfParseCtx *ctx) {
    lfType *t = parse_nontrivial_type(ctx);
    if (ctx->errored) {
        return NULL;
    }
    while (ctx->current.type == TT_BAND || ctx->current.type == TT_BOR) {
        lfTypeType op = ctx->current.type == TT_BAND ? VT_INTERSECTION : VT_UNION;
        advance(ctx);
        lfType *rhs = parse_nontrivial_type(ctx);
        if (ctx->errored) {
            lf_type_deleter(&t);
            return NULL;
        }
        lfTypeOp *o = alloc(lfTypeOp);
        o->type = op;
        o->lhs = t;
        o->rhs = rhs;
        t = (lfType *)o;
    }
    return t;
}

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
            lf_node_deleter(&expr);
            lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected ')'");
            lf_error_print(ctx->file, ctx->source, lparen.idx_start, lparen.idx_end, "... to close");
            ctx->errored = true;
            ctx->described = true;
            return NULL;
        }
        advance(ctx);
        return expr;
    } else if (ctx->current.type == TT_SUB || ctx->current.type == TT_NOT) {
        lfToken op = ctx->current;
        advance(ctx);
        lfNode *expr = parse_literal(ctx);
        if (ctx->errored) {
            return NULL;
        }
        lfUnaryOpNode *unop = alloc(lfUnaryOpNode);
        unop->type = NT_UNARYOP;
        unop->op = op;
        unop->value = expr;
        return (lfNode *)unop;
    } else if (ctx->current.type == TT_IDENTIFIER) {
        lfToken var = ctx->current;
        advance(ctx);
        if (ctx->current.type == TT_ASSIGN) {
            advance(ctx);
            lfNode *value = parse_expr(ctx);
            lfAssignNode *assign = alloc(lfAssignNode);
            assign->type = NT_ASSIGN;
            assign->var = var;
            assign->value = value;
            return (lfNode *)assign;
        } else {
            lfVarAccessNode *access = alloc(lfVarAccessNode);
            access->type = NT_VARACCESS;
            access->var = var;
            return (lfNode *)access;
        }
    } else if (ctx->current.type == TT_LBRACE) {
        lfToken lbrace = ctx->current;
        bool is_arr = false;
        bool is_map = false;
        lfArray(lfNode *) keys = array_new(lfNode *, lf_node_deleter);
        lfArray(lfNode *) values = array_new(lfNode *, lf_node_deleter);
        advance(ctx);
        if (ctx->current.type != TT_RBRACE && ctx->current.type != TT_COMMA) {
            do {
                if (ctx->current.type == TT_COMMA) {
                    advance(ctx);
                }
                lfNode *expr = parse_expr(ctx);
                if (ctx->errored) {
                    array_delete(&keys);
                    array_delete(&values);
                    return NULL;
                }
                if (ctx->current.type == TT_COLON) {
                    is_map = true;
                    array_push(&keys, expr);
                    if (is_arr) {
                        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "invalid ':' in array");
                        array_delete(&keys);
                        array_delete(&values);
                        ctx->errored = true;
                        ctx->described = true;
                        return NULL;
                    }
                    advance(ctx);
                    lfNode *value = parse_expr(ctx);
                    if (ctx->errored) {
                        array_delete(&keys);
                        array_delete(&values);
                        return NULL;
                    }
                    array_push(&values, value);
                } else {
                    is_arr = true;
                    array_push(&values, expr);
                    if (is_map) {
                        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected ':' in map");
                        array_delete(&keys);
                        array_delete(&values);
                        ctx->errored = true;
                        ctx->described = true;
                        return NULL;
                    }
                }
            } while (ctx->current.type == TT_COMMA);
        }
        if (ctx->current.type != TT_RBRACE) {
            array_delete(&keys);
            array_delete(&values);
            lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected '}'");
            lf_error_print(ctx->file, ctx->source, lbrace.idx_start, lbrace.idx_end, "... to close");
            ctx->errored = true;
            ctx->described = true;
            return NULL;
        }
        advance(ctx);
        if (is_map) {
            lfMapNode *map = alloc(lfMapNode);
            map->type = NT_MAP;
            map->keys = keys;
            map->values = values;
            return (lfNode *)map;
        } else { /* empty declerations are assumed to be arrays */
            array_delete(&keys);
            lfArrayNode *array = alloc(lfArrayNode);
            array->type = NT_ARRAY;
            array->values = values;
            return (lfNode *)array;
        }
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
                lf_node_deleter(&object);
                return NULL;
            }
            if (ctx->current.type != TT_RBRACKET) {
                lf_node_deleter(&object);
                lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected ']'");
                lf_error_print(ctx->file, ctx->source, lbracket.idx_start, lbracket.idx_end, "... to close");
                ctx->errored = true;
                ctx->described = true;
                return NULL;
            }
            advance(ctx);
            if (ctx->current.type != TT_ASSIGN) {
                lfSubscriptionNode *sub = alloc(lfSubscriptionNode);
                sub->type = NT_SUBSCRIBE;
                sub->object = object;
                sub->index = index;
                object = (lfNode *)sub;
            } else {
                advance(ctx);
                lfNode *value = parse_expr(ctx);
                if (ctx->errored) {
                    lf_node_deleter(&object);
                    return NULL;
                }
                lfObjectAssignNode *assign = alloc(lfObjectAssignNode);
                assign->type = NT_OBJASSIGN;
                assign->object = object;
                assign->key = index;
                assign->value = value;
                return (lfNode *)assign;
            }
        } else if (ctx->current.type == TT_DOT) {
            advance(ctx);
            if (ctx->current.type != TT_IDENTIFIER) {
                lf_node_deleter(&object);
                lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected identifier");
                ctx->errored = true;
                ctx->described = true;
                return NULL;
            }
            lfLiteralNode *index = alloc(lfLiteralNode);
            index->type = NT_STRING;
            index->value = ctx->current;
            advance(ctx);
            if (ctx->current.type != TT_ASSIGN) {
                lfSubscriptionNode *sub = alloc(lfSubscriptionNode);
                sub->type = NT_SUBSCRIBE;
                sub->object = object;
                sub->index = (lfNode *)index;
                object = (lfNode *)sub;
            } else {
                advance(ctx);
                lfNode *value = parse_expr(ctx);
                if (ctx->errored) {
                    lf_node_deleter(&object);
                    lf_node_deleter((lfNode **)&index);
                    return NULL;
                }
                lfObjectAssignNode *assign = alloc(lfObjectAssignNode);
                assign->type = NT_OBJASSIGN;
                assign->object = object;
                assign->key = (lfNode *)index;
                assign->value = value;
                return (lfNode *)assign;
            }
        } else {
            lfToken lparen = ctx->current;
            lfArray(lfNode *) args = array_new(lfNode *, lf_node_deleter);
            advance(ctx);
            if (ctx->current.type != TT_RPAREN && ctx->current.type != TT_COMMA) {
                do {
                    if (ctx->current.type == TT_COMMA) {
                        advance(ctx);
                    }
                    lfNode *arg = parse_expr(ctx);
                    if (ctx->errored) {
                        lf_node_deleter(&object);
                        array_delete(&args);
                        return NULL;
                    }
                    array_push(&args, arg);
                } while (ctx->current.type == TT_COMMA);
            }
            if (ctx->current.type != TT_RPAREN) {
                lf_node_deleter(&object);
                array_delete(&args);
                lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected ')'");
                lf_error_print(ctx->file, ctx->source, lparen.idx_start, lparen.idx_end, "... to close");
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
            lf_node_deleter(&lhs);
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
            lf_node_deleter(&lhs);
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
            lf_node_deleter(&lhs);
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
            lf_node_deleter(&lhs);
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
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected expression");
        ctx->described = true;
    }
    return expr;
}

lfNode *parse_vardecl(lfParseCtx *ctx, bool allow_ref) {
    if (ctx->current.type != TT_KEYWORD) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected 'var', 'const', or 'ref'");
        ctx->errored = true;
        ctx->described = true;
        return NULL;
    }
    bool is_const = !strcmp(ctx->current.value, "const");
    bool is_ref = !strcmp(ctx->current.value, "ref");
    if (!allow_ref && is_ref) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "unexpected 'ref'");
        ctx->errored = true;
        ctx->described = true;
        return NULL;
    }
    if (!strcmp(ctx->current.value, "var") || is_const || is_ref) {
        advance(ctx);
        if (ctx->current.type != TT_IDENTIFIER) {
            lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected variable name");
            ctx->errored = true;
            ctx->described = true;
            return NULL;
        }
        lfToken name = ctx->current;
        advance(ctx);
        lfType *type = NULL;
        if (ctx->current.type == TT_COLON) {
            advance(ctx);
            type = parse_type(ctx);
            if (ctx->errored) {
                lf_token_deleter(&name);
                return NULL;
            }
        }
        lfNode *initializer = NULL;
        if (ctx->current.type == TT_ASSIGN) {
            advance(ctx);
            initializer = parse_expr(ctx);
            if (ctx->errored) {
                lf_token_deleter(&name);
                if (type) {
                    lf_type_deleter(&type);
                }
                return NULL;
            }
        }
        lfVarDeclNode *decl = alloc(lfVarDeclNode);
        decl->type = NT_VARDECL;
        decl->is_const = is_const;
        decl->name = name;
        decl->initializer = initializer;
        decl->is_ref = is_ref;
        decl->vartype = type;
        return (lfNode *)decl;
    }

    ctx->errored = true;
    ctx->described = false;
    return NULL;
}

bool parse_generics(lfParseCtx *ctx, lfArray(lfToken) *type_names, lfArray(lfType *) *types) {
    if (ctx->current.type == TT_LT) {
        lfToken lt = ctx->current;
        advance(ctx);
        if (ctx->current.type == TT_COMMA) {
            lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "unexpected ','");
            ctx->errored = true;
            ctx->described = true;
            array_delete(type_names);
            array_delete(types);
            return false;
        }
        do {
            if (ctx->current.type == TT_COMMA) {
                advance(ctx);
            }
            if (ctx->current.type != TT_IDENTIFIER) {
                lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected name");
                ctx->errored = true;
                ctx->described = true;
                array_delete(type_names);
                array_delete(types);
                return false;
            }
            array_push(type_names, ctx->current);
            advance(ctx);
            if (ctx->current.type == TT_COLON) {
                advance(ctx);
                lfType *t = parse_type(ctx);
                if (ctx->errored) {
                    array_delete(type_names);
                    array_delete(types);
                    return false;
                }
                array_push(types, t);
            } else {
                lfType *t = alloc(lfType);
                t->type = VT_ANY;
                array_push(types, t);
            }
        } while (ctx->current.type == TT_COMMA);
        if (ctx->current.type != TT_GT) {
            lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected '>'");
            lf_error_print(ctx->file, ctx->source, lt.idx_start, lt.idx_end, "... to close");
            ctx->errored = true;
            ctx->described = true;
            array_delete(type_names);
            array_delete(types);
            return false;
        }
        advance(ctx);
    }

    return true;
}

lfNode *parse_fn(lfParseCtx *ctx) {
    if (ctx->current.type != TT_KEYWORD || strcmp(ctx->current.value, "fn")) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected 'fn'");
        ctx->errored = true;
        ctx->described = true;
        return NULL;
    }
    advance(ctx);
    lfArray(lfToken) type_names = array_new(lfToken, lf_token_deleter);
    lfArray(lfType *) types = array_new(lfType *, lf_type_deleter);
    if (!parse_generics(ctx, &type_names, &types)) {
        return NULL;
    }
    if (ctx->current.type != TT_IDENTIFIER) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected function name");
        ctx->errored = true;
        ctx->described = true;
        array_delete(&type_names);
        array_delete(&types);
        return NULL;
    }
    lfToken name = ctx->current;
    advance(ctx);
    lfToken lparen = ctx->current;
    if (ctx->current.type != TT_LPAREN) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected '('");
        ctx->errored = true;
        ctx->described = true;
        array_delete(&type_names);
        array_delete(&types);
        return NULL;
    }
    advance(ctx);
    lfArray(lfVarDeclNode *) params = array_new(lfVarDeclNode *, lf_node_deleter);
    if (ctx->current.type != TT_RPAREN && ctx->current.type != TT_COMMA) {
        do {
            if (ctx->current.type == TT_COMMA) {
                advance(ctx);
            }
            lfVarDeclNode *param = (lfVarDeclNode *)parse_vardecl(ctx, true);
            if (ctx->errored) {
                if (!ctx->described) {
                    lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected parameter");
                    ctx->described = true;
                }
                lf_token_deleter(&name);
                array_delete(&params);
                array_delete(&type_names);
                array_delete(&types);
                return NULL;
            }
            array_push(&params, param);
        } while (ctx->current.type == TT_COMMA);
    }
    if (ctx->current.type != TT_RPAREN) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected ')'");
        lf_error_print(ctx->file, ctx->source, lparen.idx_start, lparen.idx_end, "... to close");
        ctx->described = true;
        ctx->errored = true;
        lf_token_deleter(&name);
        array_delete(&params);
        array_delete(&type_names);
        array_delete(&types);
        return NULL;
    }
    advance(ctx);
    lfType *type = NULL;
    if (ctx->current.type == TT_ARROW) {
        advance(ctx);
        type = parse_type(ctx);
        if (ctx->errored) {
            lf_token_deleter(&name);
            array_delete(&params);
            array_delete(&type_names);
            array_delete(&types);
            return NULL;
        }
    }
    lfToken lbrace = ctx->current;
    if (ctx->current.type != TT_LBRACE) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected '{'");
        ctx->described = true;
        ctx->errored = true;
        lf_token_deleter(&name);
        array_delete(&params);
        if (type) {
            lf_type_deleter(&type);
        }
        array_delete(&type_names);
        array_delete(&types);
        return NULL;
    }
    advance(ctx);
    lfArray(lfNode *) body = array_new(lfNode *, lf_node_deleter);
    while (ctx->current.type != TT_RBRACE) {
        lfNode *statement = parse_statement(ctx);
        if (ctx->errored) {
            lf_token_deleter(&name);
            array_delete(&params);
            array_delete(&body);
            if (type) {
                lf_type_deleter(&type);
            }
            array_delete(&type_names);
            array_delete(&types);
            return NULL;
        }
        array_push(&body, statement);
    }
    if (ctx->current.type != TT_RBRACE) { /* eof */
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected '}'");
        lf_error_print(ctx->file, ctx->source, lbrace.idx_start, lbrace.idx_end, "... to close");
        ctx->described = true;
        ctx->errored = true;
        lf_token_deleter(&name);
        array_delete(params);
        array_delete(body);
        if (type) {
            lf_type_deleter(&type);
        }
        array_delete(&type_names);
        array_delete(&types);
        return NULL;
    }
    advance(ctx);
    lfFunctionNode *f = alloc(lfFunctionNode);
    f->type = NT_FUNC;
    f->name = name;
    f->body = body;
    f->params = params;
    f->return_type = type;
    f->type_names = type_names;
    f->types = types;
    return (lfNode *)f;
}

lfNode *parse_compound(lfParseCtx *ctx) {
    if (ctx->current.type != TT_LBRACE) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected '{'");
        ctx->errored = true;
        ctx->described = true;
        return NULL;
    }
    lfToken lbrace = ctx->current;
    advance(ctx);
    lfCompoundNode *compound = alloc(lfCompoundNode);
    compound->type = NT_COMPOUND;
    compound->statements = array_new(lfNode *, lf_node_deleter);
    while (ctx->current.type != TT_RBRACE) {
        lfNode *statement = parse_statement(ctx);
        if (ctx->errored) {
            lf_node_deleter((lfNode **)&compound);
            return NULL;
        }
        array_push(&compound->statements, statement);
    }
    if (ctx->current.type != TT_RBRACE) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected '}'");
        lf_error_print(ctx->file, ctx->source, lbrace.idx_start, lbrace.idx_end, "... to close");
        ctx->errored = true;
        ctx->described = true;
        return NULL;
    }
    advance(ctx);

    return (lfNode *)compound;
}

lfNode *parse_statement(lfParseCtx *ctx) {
    if (ctx->current.type == TT_KEYWORD) {
        if (!strcmp(ctx->current.value, "var") || !strcmp(ctx->current.value, "const")) {
            return parse_vardecl(ctx, false);
        } else if (!strcmp(ctx->current.value, "if")) {
            advance(ctx);
            lfNode *condition = parse_expr(ctx);
            if (ctx->errored) {
                return NULL;
            }
            lfNode *body = parse_compound(ctx);
            if (ctx->errored) {
                lf_node_deleter(&condition);
                return NULL;
            }
            lfNode *else_body = NULL;
            if (ctx->current.type == TT_KEYWORD && !strcmp(ctx->current.value, "else")) {
                advance(ctx);
                else_body = parse_compound(ctx);
                if (ctx->errored) {
                    lf_node_deleter(&condition);
                    lf_node_deleter(&body);
                    return NULL;
                }
            }
            lfIfNode *ifnode = alloc(lfIfNode);
            ifnode->type = NT_IF;
            ifnode->body = body;
            ifnode->else_body = else_body;
            ifnode->condition = condition;
            return (lfNode *)ifnode;
        } else if (!strcmp(ctx->current.value, "while")) {
            advance(ctx);
            lfNode *condition = parse_expr(ctx);
            if (ctx->errored) {
                return NULL;
            }
            lfNode *body = parse_compound(ctx);
            if (ctx->errored) {
                lf_node_deleter(&condition);
                return NULL;
            }
            lfWhileNode *whilenode = alloc(lfWhileNode);
            whilenode->type = NT_WHILE;
            whilenode->body = body;
            whilenode->condition = condition;
            return (lfNode *)whilenode;
        } else if (!strcmp(ctx->current.value, "fn")) {
            return parse_fn(ctx);
        } else if (!strcmp(ctx->current.value, "return")) {
            advance(ctx);
            lfParseCtxState old = save(ctx);
            lfNode *expr = parse_comparative(ctx);
            if (ctx->errored) {
                restore(ctx, &old);
            }
            lfReturnNode *ret = alloc(lfReturnNode);
            ret->type = NT_RETURN;
            ret->value = expr;
            return (lfNode *)ret;
        } else if (!strcmp(ctx->current.value, "class")) {
            advance(ctx);
            if (ctx->current.type != TT_IDENTIFIER) {
                lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected class name");
                ctx->errored = true;
                ctx->described = true;
                return NULL;
            }
            lfToken name = ctx->current;
            advance(ctx);
            lfToken lbrace = ctx->current;
            if (ctx->current.type != TT_LBRACE) {
                lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected '{'");
                ctx->errored = true;
                ctx->described = true;
                lf_token_deleter(&name);
                return NULL;
            }
            advance(ctx);
            lfArray(lfNode *) body = array_new(lfNode *, lf_node_deleter);
            while (ctx->current.type != TT_RBRACE) {
                lfNode *statement = NULL;
                if (ctx->current.type == TT_KEYWORD) {
                    if (!strcmp(ctx->current.value, "var") || !strcmp(ctx->current.value, "const")) {
                        statement = parse_vardecl(ctx, false);
                    } else if (!strcmp(ctx->current.value, "fn")) {
                        statement = parse_fn(ctx);
                    }
                }
                if (ctx->errored || statement == NULL) {
                    if (statement == NULL) {
                        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected 'var', 'const', 'fn', or '}'");
                    }
                    array_delete(&body);
                    lf_token_deleter(&name);
                    ctx->errored = true;
                    ctx->described = true;
                    return NULL;
                }
                array_push(&body, statement);
            }
            advance(ctx);
            lfClassNode *cls = alloc(lfClassNode);
            cls->type = NT_CLASS;
            cls->name = name;
            cls->body = body;
            return (lfNode *)cls;
        } else if (!strcmp(ctx->current.value, "include")) {
            advance(ctx);
            lfArray(lfToken) path = array_new(lfToken, lf_token_deleter);
            if (ctx->current.type != TT_IDENTIFIER) {
                lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected include path");
                ctx->errored = true;
                ctx->described = true;
                array_delete(&path);
                return NULL;
            }
            array_push(&path, ctx->current);
            advance(ctx);
            while (ctx->current.type == TT_DOT) {
                advance(ctx);
                if (ctx->current.type != TT_IDENTIFIER) {
                    lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected include path");
                    ctx->errored = true;
                    ctx->described = true;
                    array_delete(&path);
                    return NULL;
                }
                array_push(&path, ctx->current);
                advance(ctx);
            }
            lfImportNode *import = alloc(lfImportNode);
            import->type = NT_IMPORT;
            import->path = path;
            return (lfNode *)import;
        }
    } else if (ctx->current.type == TT_LBRACKET) {
        return parse_compound(ctx);
    }

    lfNode *expr = parse_comparative(ctx); /* avoid the expr error printer */
    if (ctx->errored && !ctx->described) {
        lf_error_print(ctx->file, ctx->source, ctx->current.idx_start, ctx->current.idx_end, "expected statement or expression");
        ctx->described = true;
    }

    return expr;
}

lfNode *lf_parse(const char *source, const char *file) {
    lfArray(lfToken) tokens = lf_tokenize(source, file);
    if (tokens == NULL) {
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

    lfCompoundNode *chunk = alloc(lfCompoundNode);
    chunk->type = NT_COMPOUND;
    chunk->statements = array_new(lfNode *, lf_node_deleter);
    while (ctx.current.type != TT_EOF) {
        lfNode *statement = parse_statement(&ctx);
        if (ctx.errored) {
            lf_node_deleter((lfNode **)&chunk);
            chunk = NULL;
            break;
        }
        array_push(&chunk->statements, statement);
    }

    for (size_t i = 0; i < length(&tokens); i++) {
        if (tokens[i].type == TT_KEYWORD || (ctx.errored && i >= ctx.current_idx)) {
            lf_token_deleter(tokens + i);
        }
    }
    array_delete(&tokens);

    return (lfNode *)chunk;
}
