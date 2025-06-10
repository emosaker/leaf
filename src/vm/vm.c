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
#include "vm/state.h"
#include "vm/value.h"

void lf_run(lfState *state, lfProto *proto) {
    if (setjmp(state->error_buf) == 1) {
        return;
    }
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
            case OP_DUP:
                lf_push(state, state->base + INS_E(ins));
                break;
            case OP_POP:
                state->top -= INS_E(ins);
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
                lf_deletevalue(&lhs);
                lf_deletevalue(&rhs);
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
                lf_deletevalue(&lhs);
                lf_deletevalue(&rhs);
            } break;
        }
    }
}
