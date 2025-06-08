/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>

#include "vm/vm.h"
#include "compiler/bytecode.h"
#include "vm/state.h"

void lf_run(lfState *state, lfProto *proto) {
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
                    printf("too few stack values\n");
                    state->errored = true;
                    return;
                }
                lfValue rhs = *--state->top;
                lfValue lhs = *--state->top;
                if (lhs.type != rhs.type) {
                    printf("attempt to add values of varying types\n");
                    state->errored = true;
                    return;
                }
                switch (lhs.type) {
                    case LF_INT:
                        lf_pushint(state, lhs.v.integer + rhs.v.integer);
                        break;
                    default:
                        printf("unhandled type in add: %d\n", lhs.type);
                        break;
                }
            } break;
        }
    }
}
