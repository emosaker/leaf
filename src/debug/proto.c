/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>

#include "compiler/bytecode.h"
#include "debug/proto.h"

void print_instruction(const lfProto *proto, uint32_t *code, size_t *i) {
    uint32_t ins = code[*i];
    switch (INS_OP(ins)) {
        case OP_PUSHSI:
            printf("pushshort %u\n", INS_E(ins));
            break;
        case OP_PUSHLI:
            printf("pushlong %lu\n", proto->ints[INS_E(ins)]);
            break;
        case OP_PUSHS:
            printf("pushstring \"%s\"\n", proto->strings[INS_E(ins)]);
            break;
        case OP_PUSHNULL:
            printf("pushnull\n");
            break;
        case OP_DUP:
            printf("dup offset=%u\n", INS_E(ins));
            break;
        case OP_POP:
            printf("pop values=%d\n", INS_E(ins));
            break;

        case OP_GETGLOBAL:
            printf("getglob name=%s\n", proto->strings[INS_E(ins)]);
            break;
        case OP_SETGLOBAL:
            printf("setglob name=%s\n", proto->strings[INS_E(ins)]);
            break;
        case OP_GETUPVAL:
            printf("getupval offset=%d\n", INS_E(ins));
            break;
        case OP_SETUPVAL:
            printf("setupval offset=%d\n", INS_E(ins));
            break;
        case OP_INDEX:
            printf("index\n");
            break;
        case OP_ASSIGN:
            printf("assign\n");
            break;
        case OP_SET:
            printf("set\n");
            break;

        case OP_NEWARR:
            printf("newarr nvalues=%u\n", INS_E(ins));
            break;
        case OP_NEWMAP:
            printf("newmap nmembers=%u\n", INS_E(ins));
            break;

        case OP_ADD: printf("add\n"); break;
        case OP_SUB: printf("sub\n"); break;
        case OP_MUL: printf("mul\n"); break;
        case OP_DIV: printf("div\n"); break;
        case OP_POW: printf("pow\n"); break;
        case OP_EQ: printf("eq\n"); break;
        case OP_NE: printf("ne\n"); break;
        case OP_LT: printf("lt\n"); break;
        case OP_GT: printf("gt\n"); break;
        case OP_LE: printf("le\n"); break;
        case OP_GE: printf("ge\n"); break;
        case OP_BAND: printf("band\n"); break;
        case OP_BOR: printf("bor\n"); break;
        case OP_BXOR: printf("bxor\n"); break;
        case OP_BLSH: printf("blsh\n"); break;
        case OP_BRSH: printf("brsh\n"); break;
        case OP_AND: printf("band\n"); break;
        case OP_OR: printf("bor\n"); break;

        case OP_NEG: printf("neg\n"); break;
        case OP_NOT: printf("not\n"); break;

        case OP_JMP:
            printf("jmp +%u\n", INS_E(ins));
            break;
        case OP_JMPBACK:
            printf("jmp -%u\n", INS_E(ins));
            break;
        case OP_JMPIFNOT:
            printf("jmpifnot +%u\n", INS_E(ins));
            break;

        case OP_CALL:
            printf("call args=%d, ret=%d\n", INS_A(ins), INS_B(ins));
            break;

        case OP_CL:
            printf("closure proto=%s\n", proto->protos[INS_E(ins)]->name != 0 ? proto->protos[INS_E(ins)]->strings[proto->protos[INS_E(ins)]->name - 1] : "<anonymous>");
            break;
        case OP_CAPTURE:
            printf("capture offset=%d by=%s\n", INS_D(ins), INS_A(ins) == UVT_REF ? "ref" : "idx");
            break;
        case OP_RET:
            printf("return values=%d\n", INS_A(ins));
            break;

        case OP_CLS:
            printf("class members=%d\n", INS_E(ins));
            break;

        default:
            printf("unhandled: %d\n", INS_OP(ins));
    }
    *i += 1;
}

void lf_proto_print(const lfProto *proto) {
    for (int i = 0; i < proto->szprotos; i++) {
        lfProto *child = proto->protos[i];
        lf_proto_print(child);
    }
    if (proto->name != 0) {
        printf("%s:\n", proto->strings[proto->name - 1]);
    } else {
        printf("<anonymous>:\n");
    }
    size_t i = 0;
    while (i < proto->szcode) {
        printf("  ");
        print_instruction(proto, proto->code, &i);
    }
}
