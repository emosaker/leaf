/*
 * This file is part of the leaf programming language
 */

#include <string.h>

#include "vm/vm.h"
#include "compiler/bytecode.h"
#include "lib/array.h"
#include "vm/error.h"
#include "vm/value.h"

void save_frame(lfState *state, lfClosure *cl) {
    lfCallFrame frame = (lfCallFrame) {
        .base = state->base - state->stack,
        .top = state->top - state->stack,
        .cl = cl,
        .ip = 0
    };
    state->base = state->top;
    array_push(&state->frame, frame);
}

void restore_frame(lfState *state) {
    lfCallFrame frame = state->frame[--length(&state->frame)];
    state->base = state->stack + frame.base;
    state->top = state->stack + frame.top;
}

void lf_call(lfState *state, int nargs, int nret) {
    lf_pusharray(state, nargs);
    lfValue argsv = lf_pop(state);
    lfArray(lfValue) args = lf_array(&argsv)->values;
    length(&args) = nargs;
    for (uint8_t i = 0; i < nargs; i++) {
        args[nargs - i - 1] = lf_pop(state);
    }
    lfValue func = lf_pop(state);
    if (func.type != LF_CLOSURE) {
        lf_errorf(state, "attempt to call object of type %s", lf_typeof(&func));
    }

    save_frame(state, lf_cl(&func));

    for (uint8_t i = 0; i < nargs; i++) {
        lf_push(state, args + i);
    }

    lfClosure *cl = lf_cl(&func);
    if (cl->is_c) {
        int returned = cl->f.c.func(state);
        if (returned == 0 && nret == 1) {
            lf_pushnull(state);
        }
    } else {
        lfValue **old_up = state->upvalues;
        state->upvalues = cl->f.lf.upvalues;
        int returned = lf_run(state);
        if (returned == 0 && nret == 1) {
            lf_pushnull(state);
        }
        state->upvalues = old_up;
    }

    lf_pusharray(state, nret);
    lfValue retv = lf_pop(state);
    lfArray(lfValue) ret = lf_array(&retv)->values;
    length(&ret) = nret;
    for (uint8_t i = 0; i < nret; i++) {
        ret[nret - i - 1] = lf_pop(state);
    }

    while (state->top > state->base) {
        lfValue v = lf_pop(state);
    }

    restore_frame(state);

    for (uint8_t i = 0; i < nret; i++) {
        lf_push(state, ret + i);
    }
}

int lf_run(lfState *state) {
    int captured = 0;
    lfProto *proto = state->frame[length(&state->frame) - 1].cl->f.lf.proto; /* proto is gotten here instead of passed to ensure that any error thrown by the VM has a lfCallInfo for diagnostics */
    for (int i = 0; i < proto->szcode; i++) {
        uint32_t ins = proto->code[i];
        switch (INS_OP(ins)) {
            case OP_PUSHSI:
                lf_pushint(state, INS_E(ins));
                break;
            case OP_PUSHLI:
                lf_pushint(state, proto->ints[INS_E(ins)]);
                break;
            case OP_PUSHF:
                lf_pushfloat(state, proto->floats[INS_E(ins)]);
                break;
            case OP_PUSHS:
                lf_pushstring(state, proto->strings[INS_E(ins)], proto->string_lengths[INS_E(ins)]);
                break;
            case OP_PUSHNULL:
                lf_pushnull(state);
                break;
            case OP_DUP:
                lf_push(state, state->base + INS_E(ins));
                break;
            case OP_POP:
                state->top -= INS_E(ins);
                break;

            case OP_GETGLOBAL:
                lf_getsglobal(state, proto->strings[INS_E(ins)], proto->string_lengths[INS_E(ins)]);
                break;
            case OP_SETGLOBAL:
                lf_setsglobal(state, proto->strings[INS_E(ins)], proto->string_lengths[INS_E(ins)]);
                break;
            case OP_GETUPVAL:
                lf_getupval(state, INS_E(ins));
                break;
            case OP_SETUPVAL:
                lf_setupval(state, INS_E(ins));
                break;
            case OP_INDEX: {
                lfValue index = lf_pop(state);
                lfValue object = lf_pop(state);
                if (object.type != LF_ARRAY) {
                    lf_errorf(state, "attempt to index object of type %s", lf_typeof(&object));
                }
                if (index.type != LF_INT) {
                    lf_errorf(state, "attempt to index array with %s", lf_typeof(&index));
                }
                if (index.v.integer < 0 || index.v.integer >= length(&lf_array(&object)->values)) {
                    lf_errorf(state, "index out of bounds");
                }
                lfValue v = lf_array(&object)->values[index.v.integer];
                lf_push(state, &v);
            } break;
            case OP_ASSIGN:
                state->base[INS_E(ins)] = lf_pop(state);
                break;

            case OP_NEWARR: {
                lfArray(lfValue) values = array_new(lfValue);
                array_reserve(&values, INS_E(ins));
                for (uint32_t i = 0; i < INS_E(ins); i++) {
                    values[INS_E(ins) - i - 1] = lf_pop(state);
                }
                length(&values) = INS_E(ins);
                lf_pusharray(state, INS_E(ins));
                lfValueArray *arr = lf_array(state->top - 1);
                memcpy(arr->values, values, INS_E(ins) * sizeof(lfValue));
                length(&arr->values) = INS_E(ins);
                array_delete(&values);
            } break;

            case OP_ADD:
                lf_add(state);
                break;
            case OP_SUB:
                lf_sub(state);
                break;
            case OP_MUL:
                lf_mul(state);
                break;
            case OP_DIV:
                lf_div(state);
                break;
            case OP_POW:
                lf_pow(state);
                break;

            case OP_EQ:
                lf_eq(state);
                break;
            case OP_NE:
                lf_ne(state);
                break;
            case OP_LT:
                lf_lt(state);
                break;
            case OP_GT:
                lf_gt(state);
                break;
            case OP_LE:
                lf_le(state);
                break;
            case OP_GE:
                lf_ge(state);
                break;

            case OP_BAND:
                lf_band(state);
                break;
            case OP_BOR:
                lf_bor(state);
                break;
            case OP_BXOR:
                lf_bxor(state);
                break;
            case OP_BLSH:
                lf_blsh(state);
                break;
            case OP_BRSH:
                lf_brsh(state);
                break;

            case OP_AND:
                lf_and(state);
                break;
            case OP_OR:
                lf_or(state);
                break;

            case OP_JMP:
                i += INS_E(ins) / 4;
                break;
            case OP_JMPBACK:
                i -= INS_E(ins) / 4 + 1;
                break;
            case OP_JMPIFNOT: {
                lfValue v = lf_pop(state);
                if (v.type == LF_NULL || (v.type == LF_BOOL && !v.v.boolean) || (v.type == LF_INT && v.v.integer == 0)) {
                    i += INS_E(ins) / 4;
                }
            } break;

            case OP_CALL: {
                state->frame[length(&state->frame) - 1].ip = i;
                lf_call(state, INS_A(ins), INS_B(ins));
                if (state->errored) {
                    return 0;
                }
            } break;

            case OP_CL: {
                lfProto *p = proto->protos[INS_E(ins)];
                lf_newlfcl(state, p);
                captured = 0;
            } break;
            case OP_CAPTURE: {
                lfValue *cl = state->top - 1;
                lfValue *capture;
                if (INS_A(ins) == UVT_IDX) {
                    capture = state->base + INS_D(ins);
                } else {
                    capture = state->upvalues[INS_D(ins)];
                }
                lf_cl(cl)->f.lf.upvalues[captured++] = capture;
            } break;
            case OP_RET: {
                /* move upvalues that would go out of scope onto the heap */
                if ((state->top - 1)->type == LF_CLOSURE) {
                    lfClosure *cl = lf_cl(state->top - 1);
                    if (!cl->is_c) {
                        lf_pusharray(state, 0);
                        lfValue arrv = lf_pop(state);
                        lfValueArray *arr = lf_array(&arrv);
                        for (int i = 0; i < cl->f.lf.proto->szupvalues; i++) {
                            if (cl->f.lf.upvalues[i] < state->base) continue;
                            array_push(&arr->values, *cl->f.lf.upvalues[i]);
                            cl->f.lf.upvalues[i] = arr->values + length(&arr->values) - 1;
                        }
                        if (length(&arr->values) > 0)
                            lf_valuemap_insert(&state->strays, state->top - 1, &arrv);
                    }
                }
                return INS_E(ins);
            } break;
        }
    }

    return 0;
}
