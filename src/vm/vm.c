/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <setjmp.h>
#include <string.h>

#include "vm/vm.h"
#include "compiler/bytecode.h"
#include "lib/array.h"
#include "vm/error.h"
#include "vm/value.h"

void lf_call(lfState *state, int nargs, int nret) {
    lfArray(lfValue) args = array_new(lfValue);
    array_reserve(&args, nargs);
    length(&args) = nargs;
    for (uint8_t i = 0; i < nargs; i++) {
        args[nargs - i - 1] = lf_pop(state);
    }
    lfValue func = lf_pop(state);
    if (func.type != LF_CLOSURE) {
        array_delete(&args);
        lf_errorf(state, "attempt to call object of type %s", lf_typeof(&func));
    }

    lfValue *old_base = state->base;
    state->base = state->top;

    for (uint8_t i = 0; i < nargs; i++) {
        lf_push(state, args + i);
    }

    lfClosure cl = func.v.cl;
    if (cl.is_c) {
        int returned = cl.f.c.func(state);
        if (returned == 0 && nret == 1) {
            lf_pushnull(state);
        }
    } else {
        lfValue **old_up = state->upvalues;
        state->upvalues = cl.f.lf.upvalues;
        int returned = lf_run(state, cl.f.lf.proto);
        if (returned == 0 && nret == 1) {
            lf_pushnull(state);
        }
        state->upvalues = old_up;
    }

    array_delete(&args);

    lfArray(lfValue) ret = array_new(lfValue);
    array_reserve(&ret, nret);
    length(&ret) = nret;
    for (uint8_t i = 0; i < nret; i++) {
        ret[nret - i - 1] = lf_pop(state);
    }

    while (state->top > state->base) {
        lfValue v = lf_pop(state);
    }
    state->base = old_base;

    for (uint8_t i = 0; i < nret; i++) {
        lf_push(state, ret + i);
    }
    array_delete(&ret);
}

int lf_run(lfState *state, lfProto *proto) {
    if (setjmp(state->error_buf) == 1) {
        return 0;
    }
    int captured = 0;
    for (size_t i = 0; i < proto->szcode; i++) {
        uint32_t ins = proto->code[i];
        switch (INS_OP(ins)) {
            case OP_PUSHSI:
                lf_pushint(state, INS_E(ins));
                break;
            case OP_PUSHLI:
                lf_pushint(state, proto->ints[INS_E(ins)]);
                break;
            case OP_PUSHS:
                lf_pushstring(state, proto->strings[INS_E(ins)], strlen(proto->strings[INS_E(ins)]));
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
                lf_getglobal(state, proto->strings[INS_E(ins)]);
                break;
            case OP_SETGLOBAL:
                lf_setglobal(state, proto->strings[INS_E(ins)]);
                break;
            case OP_GETUPVAL:
                lf_getupval(state, INS_E(ins));
                break;
            case OP_SETUPVAL:
                lf_setupval(state, INS_E(ins));
                break;

            case OP_ADD: {
                lfValue rhs = lf_pop(state);
                lfValue lhs = lf_pop(state);
                switch (lhs.type) {
                    case LF_INT:
                        if (rhs.type == LF_INT) {
                            lf_pushint(state, lhs.v.integer + rhs.v.integer);
                        } /* TODO: float handler */ else {
                            goto unsupported;
                        }
                        break;
                    case LF_STRING:
                        if (rhs.type != LF_STRING) {
                            goto unsupported;
                        }
                        lfArray(char) concatenated = array_new(char);
                        array_reserve(&concatenated, length(&lhs.v.string) + length(&rhs.v.string) + 1);
                        memcpy(concatenated, lhs.v.string, length(&lhs.v.string));
                        memcpy(concatenated + length(&lhs.v.string), rhs.v.string, length(&rhs.v.string));
                        length(&concatenated) = length(&lhs.v.string) + length(&rhs.v.string);
                        concatenated[length(&concatenated)] = 0;
                        lf_pushlfstring(state, concatenated);
                        break;
                    unsupported:
                    default:
                        lf_errorf(state, "unsupported types for addition: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
                }
                lf_value_deleter(&lhs);
                lf_value_deleter(&rhs);
            } break;
            case OP_EQ: {
                lfValue rhs = lf_pop(state);
                lfValue lhs = lf_pop(state);
                switch (lhs.type) {
                    case LF_INT:
                        if (rhs.type == LF_INT) {
                            lf_pushbool(state, lhs.v.integer == rhs.v.integer);
                        } /* TODO: float handler */ else {
                            lf_pushbool(state, false);
                        }
                        break;
                    case LF_STRING:
                        if (rhs.type != LF_STRING) {
                            lf_pushbool(state, false);
                        } else if (length(&lhs.v.string) != length(&rhs.v.string)) {
                            lf_pushbool(state, false);
                        } else {
                            lf_pushbool(state, !strncmp(lhs.v.string, rhs.v.string, length(&lhs.v.string)));
                        }
                        break;
                    default:
                        lf_pushbool(state, false);
                }
                lf_value_deleter(&lhs);
                lf_value_deleter(&rhs);
            } break;

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
                lf_call(state, INS_A(ins), INS_B(ins));
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
                cl->v.cl.f.lf.upvalues[captured++] = capture;
            } break;
            case OP_RET: {
                return INS_E(ins);
            } break;
        }
    }

    return 0;
}
