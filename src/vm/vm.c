/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <setjmp.h>

#include "vm/vm.h"
#include "compiler/bytecode.h"
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

            case OP_ADD: {
                if (LF_STACKSIZE(state) < 2) {
                    lf_error(state, "too few values for addition");
                }
                lfValue rhs = *--state->top;
                lfValue lhs = *--state->top;
                switch (lhs.type) {
                    case LF_INT:
                        if (rhs.type == LF_INT) {
                            lf_pushint(state, lhs.v.integer + rhs.v.integer);
                        } else {
                            goto unsupported;
                        }
                        break;
                    unsupported:
                    default:
                        lf_errorf(state, "unsupported types for addition: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
                }
            } break;
        }
    }
}
