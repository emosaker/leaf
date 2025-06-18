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

bool getupvalue(lfCompilerCtx *ctx, char *key, uint32_t *out) {
    /* find the upvalue stack and index */
    int match = 0;
    lfVariable match_var;
    for (int i = length(&ctx->fnstack); i > 0; i--) {
        lfStackFrame *frame = ctx->fnstack[i - 1];
        if (lf_variablemap_lookup(&frame->scope, key, &match_var)) {
            match = i - 1;
            break;
        }
        if (i == 1) return false;
    }
    /* match is now the index of the stack frame, match_idx is the index of the upvalue */
    /* check if the upvalue is present already */
    bool found = false;
    int capture = 0;
    for (int i = 0; i < length(&ctx->fnstack[match + 1]->upvalues); i++) {
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
    for (int i = match + 2; i < length(&ctx->fnstack); i++) {
        /* check if a ref is already present */
        found = false;
        for (int j = 0; j < length(&ctx->fnstack[i]->upvalues); j++) {
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
    emit_ins_e(&ctx->bb, OP_PUSHS, new_string(&ctx->bb, node->value.value), node->lineno);
    ctx->top += 1;
    return true;
}

bool visit_int(lfCompilerCtx *ctx, lfLiteralNode *node) {
    if (ctx->discarded) return true;
    uint64_t value = strtoll(node->value.value, NULL, 10);
    if (value >= 0xFFFFFF) { /* 2^24 */
        emit_ins_e(&ctx->bb, OP_PUSHLI, new_u64(&ctx->bb, value), node->lineno);
    } else {
        emit_ins_e(&ctx->bb, OP_PUSHSI, value, node->lineno);
    }
    ctx->top += 1;
    return true;
}

bool visit_float(lfCompilerCtx *ctx, lfLiteralNode *node) {
    if (ctx->discarded) return true;
    double value = atof(node->value.value);
    emit_ins_e(&ctx->bb, OP_PUSHF, new_f64(&ctx->bb, value), node->lineno);
    return true;
}

bool visit_binop(lfCompilerCtx *ctx, lfBinaryOpNode *node) {
    if (!nodiscard(ctx, node->lhs)) return false;
    if (!nodiscard(ctx, node->rhs)) return false;
    switch (node->op.type) {
        case TT_ADD: emit_op(&ctx->bb, OP_ADD, node->lineno); break;
        case TT_SUB: emit_op(&ctx->bb, OP_SUB, node->lineno); break;
        case TT_MUL: emit_op(&ctx->bb, OP_MUL, node->lineno); break;
        case TT_DIV: emit_op(&ctx->bb, OP_DIV, node->lineno); break;
        case TT_EQ: emit_op(&ctx->bb, OP_EQ, node->lineno); break;
        case TT_NE: emit_op(&ctx->bb, OP_NE, node->lineno); break;
        case TT_LT: emit_op(&ctx->bb, OP_LT, node->lineno); break;
        case TT_GT: emit_op(&ctx->bb, OP_GT, node->lineno); break;
        case TT_LE: emit_op(&ctx->bb, OP_LE, node->lineno); break;
        case TT_BAND: emit_op(&ctx->bb, OP_BAND, node->lineno); break;
        case TT_BOR: emit_op(&ctx->bb, OP_BOR, node->lineno); break;
        case TT_BXOR: emit_op(&ctx->bb, OP_BXOR, node->lineno); break;
        case TT_LSHIFT: emit_op(&ctx->bb, OP_BLSH, node->lineno); break;
        case TT_RSHIFT: emit_op(&ctx->bb, OP_BRSH, node->lineno); break;
        case TT_AND: emit_op(&ctx->bb, OP_AND, node->lineno); break;
        case TT_OR: emit_op(&ctx->bb, OP_OR, node->lineno); break;
        default: /* unreachable for any AST produced by the parser */
            return false;
    }

    ctx->top -= 1;
    return true;
}

bool visit_unop(lfCompilerCtx *ctx, lfUnaryOpNode *node) {
    if (!nodiscard(ctx, node->value)) return false;
    switch (node->op.type) {
        case TT_SUB: emit_op(&ctx->bb, OP_NEG, node->lineno); break;
        case TT_NOT: emit_op(&ctx->bb, OP_NOT, node->lineno); break;
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
        emit_op(&ctx->bb, OP_PUSHNULL, node->lineno);
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
        emit_ins_e(&ctx->bb, OP_DUP, var.stack_offset, node->lineno);
    } else if (getupvalue(ctx, node->var.value, &uvindex)) {
        emit_ins_e(&ctx->bb, OP_GETUPVAL, uvindex, node->lineno);
    } else {
        emit_ins_e(&ctx->bb, OP_GETGLOBAL, new_cstring(&ctx->bb, node->var.value, strlen(node->var.value)), node->lineno);
    }
    ctx->top += 1;
    return true;
}

bool visit_array(lfCompilerCtx *ctx, lfArrayNode *node) {
    for (int i = 0; i < length(&node->values); i++)
        if (!nodiscard(ctx, node->values[i]))
            return false;
    emit_ins_e(&ctx->bb, OP_NEWARR, length(&node->values), node->lineno);
    ctx->top -= length(&node->values) - 1;
    return true;
}

bool visit_map(lfCompilerCtx *ctx, lfMapNode *node) {
    for (int i = 0; i < length(&node->values); i++) {
        if (!nodiscard(ctx, node->keys[i]))
            return false;
        if (!nodiscard(ctx, node->values[i]))
            return false;
    }
    emit_ins_e(&ctx->bb, OP_NEWMAP, length(&node->values), node->lineno);
    ctx->top -= length(&node->values) * 2 - 1;
    return true;
}

bool visit_subscribe(lfCompilerCtx *ctx, lfSubscriptionNode *node) {
    if (!nodiscard(ctx, node->object)) return false;
    if (!nodiscard(ctx, node->index)) return false;
    emit_op(&ctx->bb, OP_INDEX, node->lineno);
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
        emit_ins_e(&ctx->bb, OP_ASSIGN, index.stack_offset, node->lineno);
    } else if (getupvalue(ctx, node->var.value, &uvindex)) {
        emit_ins_e(&ctx->bb, OP_SETUPVAL, uvindex, node->lineno);
    } else {
        emit_ins_e(&ctx->bb, OP_SETGLOBAL, new_cstring(&ctx->bb, node->var.value, strlen(node->var.value)), node->lineno);
    }
    ctx->top -= 1;
    return true;
}

bool visit_objassign(lfCompilerCtx *ctx, lfObjectAssignNode *node) {
    if (!nodiscard(ctx, node->object)) return false;
    if (!nodiscard(ctx, node->key)) return false;
    if (!nodiscard(ctx, node->value)) return false;
    emit_op(&ctx->bb, OP_SET, node->lineno);
    ctx->top -= 3;
    return true;
}

bool visit_if(lfCompilerCtx *ctx, lfIfNode *node) {
    if (!nodiscard(ctx, node->condition)) return false;
    int idx = length(&ctx->bb.bytecode);
    emit_op(&ctx->bb, OP_NOP, node->lineno); /* replaced later */
    ctx->top -= 1; /* condition is popped */
    if (!visit(ctx, node->body)) return false;
    int curr = length(&ctx->bb.bytecode);
    emit_ins_e_at(&ctx->bb, OP_JMPIFNOT, (curr - idx) - 4 + (node->else_body != NULL ? 4 : 0), idx, node->lineno);
    if (node->else_body != NULL) {
        idx = length(&ctx->bb.bytecode);
        emit_op(&ctx->bb, OP_NOP, node->lineno); /* replaced later */
        if (!visit(ctx, node->else_body)) return false;
        curr = length(&ctx->bb.bytecode);
        emit_ins_e_at(&ctx->bb, OP_JMP, (curr - idx) - 4, idx, node->else_body->lineno);
    }
    return true;
}

bool visit_while(lfCompilerCtx *ctx, lfWhileNode *node) {
    int start = length(&ctx->bb.bytecode);
    if (!nodiscard(ctx, node->condition)) return false;
    int idx = length(&ctx->bb.bytecode);
    emit_op(&ctx->bb, OP_NOP, node->lineno); /* replaced later */
    ctx->top -= 1; /* condition is popped */
    if (!visit(ctx, node->body)) return false;
    emit_ins_e(&ctx->bb, OP_JMPBACK, length(&ctx->bb.bytecode) - start, node->lineno);
    emit_ins_e_at(&ctx->bb, OP_JMPIFNOT, length(&ctx->bb.bytecode) - idx - 4, idx, node->lineno);
    return true;
}

bool visit_call(lfCompilerCtx *ctx, lfCallNode *node) {
    if (!nodiscard(ctx, node->func)) return false;
    for (int i = 0; i < length(&node->args); i++) {
        if (!nodiscard(ctx, node->args[i])) return false;
    }
    emit_ins_abc(&ctx->bb, OP_CALL, length(&node->args), ctx->discarded ? 0 : 1, 0, node->lineno);
    ctx->top -= length(&node->args) + (ctx->discarded ? 1 : 0);
    return true;
}

bool visit_return(lfCompilerCtx *ctx, lfReturnNode *node) {
    if (node->value) {
        if (!nodiscard(ctx, node->value)) return false;
        ctx->top -= 1;
    }
    emit_ins_abc(&ctx->bb, OP_RET, node->value != 0 ? 1 : 0, 0, 0, node->lineno);
    return true;
}

bool visit_fn(lfCompilerCtx *ctx, lfFunctionNode *node) {
    int old_top = ctx->top;
    lfBytecodeBuilder old = bytecodebuilder_init(&ctx->bb);

    ctx->scope = lf_variablemap_create(256);
    ctx->top = length(&node->params); /* args passed on stack */

    lfStackFrame frame = (lfStackFrame) {
        .scope = ctx->scope,
        .upvalues = array_new(lfUpValue)
    };
    array_push(&ctx->fnstack, &frame);

    for (int i = 0; i < length(&node->params); i++) {
        lf_variablemap_insert(&ctx->scope, node->params[i]->name.value, (lfVariable) {
            .stack_offset = i,
            .is_const = node->params[i]->is_const,
            .is_ref = node->params[i]->is_ref,
        });
    }

    for (int i = 0; i < length(&node->body); i++)
        if (!visit(ctx, node->body[i])) {
            bytecodebuilder_restore(&ctx->bb, old, false);
            array_delete(&frame.upvalues);
            array_delete(&frame.scope);
            length(&ctx->fnstack) -= 1;
            /* caller cleans up */
            ctx->scope = ctx->fnstack[length(&ctx->fnstack) - 1]->scope;
            return false;
        }

    lfProto *func = bytecodebuilder_allocproto(&ctx->bb, node->name.value, length(&frame.upvalues), length(&node->params));

    bytecodebuilder_restore(&ctx->bb, old, true);
    array_delete(&ctx->fnstack[length(&ctx->fnstack) - 1]->scope);
    length(&ctx->fnstack) -= 1;

    ctx->scope = ctx->fnstack[length(&ctx->fnstack) - 1]->scope;
    ctx->top = old_top;

    array_push(&ctx->bb.protos, func);

    emit_ins_e(&ctx->bb, OP_CL, length(&ctx->bb.protos) - 1, node->lineno);
    for (int i = 0; i < length(&frame.upvalues); i++) {
        emit_ins_ad(&ctx->bb, OP_CAPTURE, frame.upvalues[i].by, frame.upvalues[i].index, node->lineno);
    }
    array_delete(&frame.upvalues);

    lfVariable i;
    if (lf_variablemap_lookup(&ctx->scope, node->name.value, &i)) {
        emit_ins_e(&ctx->bb, OP_ASSIGN, i.stack_offset, node->lineno);
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
    return true;
}

bool visit_compound(lfCompilerCtx *ctx, lfCompoundNode *node) {
    lfVariableMap old = lf_variablemap_clone(&ctx->scope);
    int old_top = ctx->top;
    for (int i = 0; i < length(&node->statements); i++)
        if (!visit(ctx, node->statements[i])) {
            array_delete(&old);
            return false;
        }
    array_delete(&ctx->scope);
    if (ctx->top - old_top > 0) emit_ins_e(&ctx->bb, OP_POP, ctx->top - old_top, node->lineno);
    ctx->scope = old;
    ctx->top = old_top;
    ctx->fnstack[length(&ctx->fnstack) - 1]->scope = old;
    return true;
}

bool visit(lfCompilerCtx *ctx, lfNode *node) {
    switch (node->type) {
        case NT_INT: return visit_int(ctx, (lfLiteralNode *)node);
        case NT_FLOAT: return visit_float(ctx, (lfLiteralNode *)node);
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
        .scope = lf_variablemap_create(256),
        .fnstack = array_new(lfStackFrame)
    };
    bytecodebuilder_init(&ctx.bb);

    lfStackFrame frame = (lfStackFrame) {
        .scope = ctx.scope,
        .upvalues = array_new(lfUpValue)
    };
    array_push(&ctx.fnstack, &frame);

    if (!visit(&ctx, ast)) {
        bytecodebuilder_restore(&ctx.bb, (lfBytecodeBuilder){0,0,0,0}, false);
        array_delete(&ctx.scope);
        array_delete(&frame.upvalues);
        array_delete(&ctx.fnstack);
        lf_node_deleter(&ast);
        return NULL;
    }

    lfProto *main = bytecodebuilder_allocproto(&ctx.bb, NULL, 0, 0);

    if (main->szcode > 0 && INS_OP(main->code[main->szcode - 1]) == OP_POP) {
        main->szcode -= 1;
    }

    bytecodebuilder_restore(&ctx.bb, (lfBytecodeBuilder){0,0,0,0}, true);
    array_delete(&ctx.scope);
    array_delete(&frame.upvalues);
    array_delete(&ctx.fnstack);
    lf_node_deleter(&ast);
    return main;
}
