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

void string_deleter(char **string) {
    free(*string);
}

bool getupvalue(lfCompilerCtx *ctx, char *key, uint32_t *out) {
    /* find the upvalue stack and index */
    size_t match = 0;
    lfVariable match_var;
    for (size_t i = length(&ctx->fnstack); i > 0; i--) {
        lfStackFrame *frame = ctx->fnstack[i - 1];
        if (lf_variablemap_lookup(&frame->scope, key, &match_var)) {
            match = i - 1;
            break;
        }
        if (i == 1) return false;
    }
    /* match is now the index of the stack frame, match_idx is the indef of the upvalue */
    /* check if the upvalue is present already */
    bool found = false;
    size_t capture = 0;
    for (size_t i = 0; i < length(&ctx->fnstack[match + 1]->upvalues); i++) {
        lfUpValue upval = ctx->fnstack[match + 1]->upvalues[i];
        if (upval.index == match_var.stack_offset) {
            found = true;
            capture = i;
            break;
        }
    }
    /* if not found, capture it */
    if (!found) {
        lfUpValue upval = (lfUpValue) {
            .by = UVT_IDX,
            .index = match_var.stack_offset
        };
        array_push(&ctx->fnstack[match + 1]->upvalues, upval);
        capture = length(&ctx->fnstack[match + 1]->upvalues) - 1;
    }
    /* pass the upvalue down by ref until we reach the current stack frame */
    for (size_t i = match + 2; i < length(&ctx->fnstack); i++) {
        /* check if a ref is already present */
        found = false;
        for (size_t j = 0; j < length(&ctx->fnstack[i]->upvalues); j++) {
            lfUpValue upval = ctx->fnstack[i]->upvalues[j];
            if (upval.index == capture) {
                found = true;
                capture = j;
                break;
            }
        }
        if (!found) {
            lfUpValue upval = (lfUpValue) {
                .by = UVT_REF,
                .index = capture
            };
            array_push(&ctx->fnstack[i]->upvalues, upval);
            capture = length(&ctx->fnstack[i]->upvalues) - 1;
        }
    }

    /* capture now holds the upvalue idx */
    *out = capture;
    return true;
}

bool visit(lfCompilerCtx *ctx, lfNode *node);

bool nodiscard(lfCompilerCtx *ctx, lfNode *node) {
    bool old = ctx->discarded;
    ctx->discarded = false;
    bool result = visit(ctx, node);
    ctx->discarded = old;
    return result;
}

bool visit_string(lfCompilerCtx *ctx, lfLiteralNode *node) {
    if (ctx->discarded) return true;
    emit_insn_e(ctx, OP_PUSHS, new_string(ctx, node->value.value, strlen(node->value.value)));
    ctx->top += 1;
    return true;
}

bool visit_int(lfCompilerCtx *ctx, lfLiteralNode *node) {
    if (ctx->discarded) return true;
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
    if (!nodiscard(ctx, node->lhs)) return false;
    if (!nodiscard(ctx, node->rhs)) return false;
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
    if (!nodiscard(ctx, node->value)) return false;
    switch (node->op.type) {
        case TT_SUB: emit_op(ctx, OP_NEG); break;
        case TT_NOT: emit_op(ctx, OP_NOT); break;
        default: /* unreachable for any AST produced by the parser */
            return false;
    }

    return true;
}

bool visit_vardecl(lfCompilerCtx *ctx, lfVarDeclNode *node) {
    if (lf_variablemap_lookup(&ctx->scope, node->name.value, NULL)) {
        lf_error_print(ctx->file, ctx->source, node->name.idx_start, node->name.idx_end, "variable redefinition");
        return false;
    }

    if (node->initializer) {
        if (!nodiscard(ctx, node->initializer))
            return false;
    } else {
        emit_op(ctx, OP_PUSHNULL);
        ctx->top += 1;
    }

    lf_variablemap_insert(&ctx->scope, node->name.value, (lfVariable) {
        .stack_offset = ctx->top - 1,
        .is_const = node->is_const,
        .is_ref = node->is_ref
    });

    return true;
}

bool visit_varaccess(lfCompilerCtx *ctx, lfVarAccessNode *node) {
    if (ctx->discarded) return true;
    lfVariable var;
    uint32_t uvindex;
    if (lf_variablemap_lookup(&ctx->scope, node->var.value, &var)) {
        emit_insn_e(ctx, OP_DUP, var.stack_offset);
    } else if (getupvalue(ctx, node->var.value, &uvindex)) {
        emit_insn_e(ctx, OP_GETUPVAL, uvindex);
    } else {
        emit_insn_e(ctx, OP_GETGLOBAL, new_string(ctx, node->var.value, strlen(node->var.value)));
    }
    ctx->top += 1;
    return true;
}

bool visit_array(lfCompilerCtx *ctx, lfArrayNode *node) {
    for (size_t i = 0; i < length(&node->values); i++)
        if (!nodiscard(ctx, node->values[i]))
            return false;
    emit_insn_e(ctx, OP_NEWARR, length(&node->values));
    ctx->top -= length(&node->values) - 1;
    return true;
}

bool visit_map(lfCompilerCtx *ctx, lfMapNode *node) {
    for (size_t i = 0; i < length(&node->values); i++) {
        if (!nodiscard(ctx, node->keys[i]))
            return false;
        if (!nodiscard(ctx, node->values[i]))
            return false;
    }
    emit_insn_e(ctx, OP_NEWMAP, length(&node->values));
    ctx->top -= length(&node->values) * 2 - 1;
    return true;
}

bool visit_subscribe(lfCompilerCtx *ctx, lfSubscriptionNode *node) {
    if (!nodiscard(ctx, node->object)) return false;
    if (!nodiscard(ctx, node->index)) return false;
    emit_op(ctx, OP_INDEX);
    ctx->top -= 1;
    return true;
}

bool visit_assign(lfCompilerCtx *ctx, lfAssignNode *node) {
    if (!nodiscard(ctx, node->value)) return false;
    lfVariable index;
    uint32_t uvindex;
    if (lf_variablemap_lookup(&ctx->scope, node->var.value, &index)) {
        if (index.is_const) {
            lf_error_print(ctx->file, ctx->source, node->var.idx_start, node->var.idx_end, "cannot assign to const");
            return false;
        }
        emit_insn_e(ctx, OP_ASSIGN, index.stack_offset);
    } else if (getupvalue(ctx, node->var.value, &uvindex)) {
        emit_insn_e(ctx, OP_SETUPVAL, uvindex);
    } else {
        emit_insn_e(ctx, OP_SETGLOBAL, new_string(ctx, node->var.value, strlen(node->var.value)));
    }
    ctx->top -= 1;
    return true;
}

bool visit_objassign(lfCompilerCtx *ctx, lfObjectAssignNode *node) {
    if (!nodiscard(ctx, node->object)) return false;
    if (!nodiscard(ctx, node->key)) return false;
    if (!nodiscard(ctx, node->value)) return false;
    emit_op(ctx, OP_SET);
    ctx->top -= 3;
    return true;
}

bool visit_if(lfCompilerCtx *ctx, lfIfNode *node) {
    if (!nodiscard(ctx, node->condition)) return false;
    size_t idx = length(&ctx->current);
    emit_op(ctx, OP_NOP); /* replaced later */
    ctx->top -= 1; /* condition is popped */
    if (!visit(ctx, node->body)) return false;
    size_t curr = length(&ctx->current);
    emit_insn_e_at(ctx, OP_JMPIFNOT, (curr - idx) - 4 + (node->else_body != NULL ? 4 : 0), idx);
    if (node->else_body != NULL) {
        idx = length(&ctx->current);
        emit_op(ctx, OP_NOP); /* replaced later */
        if (!visit(ctx, node->else_body)) return false;
        curr = length(&ctx->current);
        emit_insn_e_at(ctx, OP_JMP, (curr - idx) - 4, idx);
    }
    return true;
}

bool visit_while(lfCompilerCtx *ctx, lfWhileNode *node) {
    size_t start = length(&ctx->current);
    if (!nodiscard(ctx, node->condition)) return false;
    size_t idx = length(&ctx->current);
    emit_op(ctx, OP_NOP); /* replaced later */
    ctx->top -= 1; /* condition is popped */
    if (!visit(ctx, node->body)) return false;
    emit_insn_e(ctx, OP_JMPBACK, length(&ctx->current) - start);
    emit_insn_e_at(ctx, OP_JMPIFNOT, length(&ctx->current) - idx - 4, idx);
    return true;
}

bool visit_call(lfCompilerCtx *ctx, lfCallNode *node) {
    if (!nodiscard(ctx, node->func)) return false;
    for (size_t i = 0; i < length(&node->args); i++) {
        if (!nodiscard(ctx, node->args[i])) return false;
    }
    emit_insn_abc(ctx, OP_CALL, length(&node->args), ctx->discarded ? 0 : 1, 0);
    ctx->top -= length(&node->args) + (ctx->discarded ? 1 : 0);
    return true;
}

bool visit_return(lfCompilerCtx *ctx, lfReturnNode *node) {
    if (node->value) {
        if (!nodiscard(ctx, node->value)) return false;
        ctx->top -= 1;
    }
    emit_insn_abc(ctx, OP_RET, node->value != 0 ? 1 : 0, 0, 0);
    return true;
}

bool visit_fn(lfCompilerCtx *ctx, lfFunctionNode *node) {
    lfArray(uint8_t) old_body = ctx->current;
    size_t old_top = ctx->top;
    lfArray(uint64_t) old_ints = ctx->ints;
    lfArray(lfProto *) old_protos = ctx->protos;
    lfArray(char *) old_strings = ctx->strings;

    ctx->current = array_new(uint8_t);
    ctx->scope = lf_variablemap_create(256);
    ctx->top = length(&node->params); /* args passed on stack */
    ctx->protos = array_new(lfProto *, lf_proto_deleter);
    ctx->strings = array_new(char *, string_deleter);
    ctx->ints = array_new(uint64_t);

    lfStackFrame frame = (lfStackFrame) {
        .scope = ctx->scope,
        .upvalues = array_new(lfUpValue)
    };
    array_push(&ctx->fnstack, &frame);

    for (size_t i = 0; i < length(&node->params); i++) {
        lf_variablemap_insert(&ctx->scope, node->params[i]->name.value, (lfVariable) {
            .stack_offset = i,
            .is_const = node->params[i]->is_const,
            .is_ref = node->params[i]->is_ref,
        });
    }

    for (size_t i = 0; i < length(&node->body); i++)
        if (!visit(ctx, node->body[i])) {
            array_delete(&ctx->current);
            array_delete(&frame.upvalues);
            array_delete(&frame.scope);
            length(&ctx->fnstack) -= 1;
            /* caller cleans up */
            ctx->current = old_body;
            ctx->scope = ctx->fnstack[length(&ctx->fnstack) - 1]->scope;
            return false;
        }

    lfProto *func = malloc(sizeof(lfProto));
    func->code = malloc(length(&ctx->current));
    func->szcode = length(&ctx->current) / 4;
    func->name = new_string(ctx, node->name.value, strlen(node->name.value)) + 1;
    func->protos = malloc(sizeof(lfProto *) * length(&ctx->protos));
    func->szprotos = length(&ctx->protos);
    func->strings = malloc(sizeof(char *) * length(&ctx->strings));
    func->szstrings = length(&ctx->strings);
    func->ints = malloc(sizeof(uint64_t) * length(&ctx->ints));
    func->szints = length(&ctx->ints);
    func->nupvalues = length(&frame.upvalues);
    func->nargs = length(&node->params);

    memcpy(func->code, ctx->current, length(&ctx->current));
    memcpy(func->protos, ctx->protos, sizeof(lfProto *) * length(&ctx->protos));
    memcpy(func->strings, ctx->strings, sizeof(char *) * length(&ctx->strings));
    memcpy(func->ints, ctx->ints, sizeof(uint64_t) * length(&ctx->ints));

    deleter(&ctx->strings) = NULL;
    deleter(&ctx->protos) = NULL;
    array_delete(&ctx->strings);
    array_delete(&ctx->protos);
    array_delete(&ctx->ints);
    array_delete(&ctx->current);
    array_delete(&ctx->fnstack[length(&ctx->fnstack) - 1]->scope);
    length(&ctx->fnstack) -= 1;

    ctx->scope = ctx->fnstack[length(&ctx->fnstack) - 1]->scope;
    ctx->top = old_top;
    ctx->current = old_body;
    ctx->protos = old_protos;
    ctx->strings = old_strings;
    ctx->ints = old_ints;

    array_push(&ctx->protos, func);

    emit_insn_e(ctx, OP_CL, length(&ctx->protos) - 1);
    for (size_t i = 0; i < length(&frame.upvalues); i++) {
        emit_insn_ad(ctx, OP_CAPTURE, frame.upvalues[i].by, frame.upvalues[i].index);
    }
    array_delete(&frame.upvalues);

    lfVariable i;
    if (lf_variablemap_lookup(&ctx->scope, node->name.value, &i)) {
        emit_insn_e(ctx, OP_ASSIGN, i.stack_offset);
    } else {
        lf_variablemap_insert(&ctx->scope, node->name.value, (lfVariable) {
            .stack_offset = ctx->top,
            .is_const = false,
            .is_ref = false
        });
        ctx->top += 1;
    }
    return true;
}

bool visit_class(lfCompilerCtx *ctx, lfClassNode *node) {
    bool wasclass = ctx->isclass;
    ctx->isclass = true;
    for (size_t i = 0; i < length(&node->body); i++) {
        switch (node->body[i]->type) {
            case NT_VARDECL:
                emit_insn_e(
                    ctx,
                    OP_PUSHS,
                    new_string(
                        ctx,
                        ((lfVarDeclNode *)node->body[i])->name.value,
                        strlen(((lfVarDeclNode *)node->body[i])->name.value)
                    )
                );
                break;
            case NT_FUNC:
                emit_insn_e(
                    ctx,
                    OP_PUSHS,
                    new_string(
                        ctx,
                        ((lfFunctionNode *)node->body[i])->name.value,
                        strlen(((lfFunctionNode *)node->body[i])->name.value)
                    )
                );
                break;
            default: /* unreachable for any AST produced by the parser */
                return false;
        }
        visit(ctx, node->body[i]);
    }

    emit_insn_e(ctx, OP_PUSHS, new_string(ctx, node->name.value, strlen(node->name.value)));
    emit_insn_e(ctx, OP_CLS, length(&node->body));
    return true;
}

bool visit_compound(lfCompilerCtx *ctx, lfCompoundNode *node) {
    lfVariableMap old = lf_variablemap_clone(&ctx->scope);
    size_t old_top = ctx->top;
    for (size_t i = 0; i < length(&node->statements); i++)
        if (!visit(ctx, node->statements[i])) {
            array_delete(&old);
            return false;
        }
    array_delete(&ctx->scope);
    if (ctx->top - old_top > 0) emit_insn_e(ctx, OP_POP, ctx->top - old_top);
    ctx->scope = old;
    ctx->top = old_top;
    ctx->fnstack[length(&ctx->fnstack) - 1]->scope = old;
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
        case NT_WHILE: return visit_while(ctx, (lfWhileNode *)node);
        case NT_CALL: return visit_call(ctx, (lfCallNode *)node);
        case NT_RETURN: return visit_return(ctx, (lfReturnNode *)node);
        case NT_FUNC: return visit_fn(ctx, (lfFunctionNode *)node);
        case NT_CLASS: return visit_class(ctx, (lfClassNode *)node);
        default:
            printf("unhandled: %d\n", node->type);
            return false;
    }
}

lfProto *lf_compile(const char *source, const char *file) {
    lfNode *ast = lf_parse(source, file);
    if (ast == NULL) {
        return NULL;
    }

    lfCompilerCtx ctx = (lfCompilerCtx) {
        .file = file,
        .source = source,
        .top = 0,
        .discarded = true,
        .protos = array_new(lfProto *, lf_proto_deleter),
        .strings = array_new(char *, string_deleter),
        .ints = array_new(uint64_t),
        .current = array_new(uint8_t),
        .scope = lf_variablemap_create(256),
        .fnstack = array_new(lfStackFrame)
    };

    lfStackFrame frame = (lfStackFrame) {
        .scope = ctx.scope,
        .upvalues = array_new(lfUpValue)
    };
    array_push(&ctx.fnstack, &frame);

    if (!visit(&ctx, ast)) {
        array_delete(&ctx.strings);
        array_delete(&ctx.protos);
        array_delete(&ctx.current);
        array_delete(&ctx.ints);
        array_delete(&ctx.scope);
        array_delete(&frame.upvalues);
        array_delete(&ctx.fnstack);
        lf_node_deleter(&ast);
        return NULL;
    }

    lfProto *main = malloc(sizeof(lfProto));
    main->code = malloc(length(&ctx.current));
    main->szcode = length(&ctx.current) / 4;
    main->name = 0;
    main->protos = malloc(sizeof(lfProto *) * length(&ctx.protos));
    main->szprotos = length(&ctx.protos);
    main->strings = malloc(sizeof(char *) * length(&ctx.strings));
    main->szstrings = length(&ctx.strings);
    main->ints = malloc(sizeof(uint64_t) * length(&ctx.ints));
    main->szints = length(&ctx.ints);
    main->nupvalues = 0;
    main->nargs = 0;

    memcpy(main->code, ctx.current, length(&ctx.current));
    memcpy(main->protos, ctx.protos, sizeof(lfProto *) * length(&ctx.protos));
    memcpy(main->strings, ctx.strings, sizeof(char *) * length(&ctx.strings));
    memcpy(main->ints, ctx.ints, sizeof(uint64_t) * length(&ctx.ints));

    if (main->szcode > 0 && INS_OP(main->code[main->szcode - 1]) == OP_POP) {
        main->szcode -= 1;
    }

    deleter(&ctx.strings) = NULL;
    deleter(&ctx.protos) = NULL;
    array_delete(&ctx.strings);
    array_delete(&ctx.current);
    array_delete(&ctx.protos);
    array_delete(&ctx.ints);
    array_delete(&ctx.scope);
    array_delete(&frame.upvalues);
    array_delete(&ctx.fnstack);
    lf_node_deleter(&ast);
    return main;
}

lfProto *lf_proto_clone(lfProto *proto) {
    lfProto *new = malloc(sizeof(lfProto));
    new->strings = malloc(sizeof(char *) * proto->szstrings);
    new->szstrings = proto->szstrings;
    new->ints = malloc(sizeof(uint64_t) * proto->szints);
    new->szints = proto->szints;
    new->protos = malloc(sizeof(lfProto *) * proto->szprotos);
    new->szprotos = proto->szprotos;
    new->code = malloc(sizeof(uint32_t) * proto->szcode);
    new->szcode = proto->szcode;
    new->name = proto->name;
    new->nargs = proto->nargs;
    new->nupvalues = proto->nupvalues;

    for (size_t i = 0; i < proto->szstrings; i++) {
        char *clone = malloc(strlen(proto->strings[i]) + 1);
        memcpy(clone, proto->strings[i], strlen(proto->strings[i]) + 1);
        new->strings[i] = clone;
    }

    for (size_t i = 0; i < proto->szprotos; i++) {
        new->protos[i] = lf_proto_clone(proto->protos[i]);
    }

    memcpy(new->ints, proto->ints, proto->szints * sizeof(uint64_t));
    memcpy(new->code, proto->code, proto->szcode * sizeof(uint32_t));

    return new;
}

void lf_proto_deleter(lfProto **proto) {
    for (size_t i = 0; i < (*proto)->szprotos; i++)
        lf_proto_deleter((*proto)->protos + i);
    for (size_t i = 0; i < (*proto)->szstrings; i++)
        string_deleter((*proto)->strings + i);
    free((*proto)->protos);
    free((*proto)->strings);
    free((*proto)->ints);
    free((*proto)->code);
    free((*proto));
}
