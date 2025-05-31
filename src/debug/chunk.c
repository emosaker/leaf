/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>

#include "compiler/bytecode.h"
#include "debug/chunk.h"

void print_instruction(const lfChunk *chunk, uint32_t *code, size_t *i) {
    uint32_t ins = code[*i];
    switch (INS_OP(ins)) {
        case OP_PUSHSI:
            printf("pushshort %u\n", INS_E(ins));
            break;
        case OP_PUSHLI:
            printf("pushlong %lu\n", chunk->ints[INS_E(ins)]);
            break;
        case OP_PUSHS:
            printf("pushstring \"%s\"\n", chunk->strings[INS_E(ins)]);
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
    }
    *i += 1;
}

void chunk_print(const lfChunk *chunk) {
    for (int i = 0; i < chunk->szprotos; i++) {
        lfProto proto = chunk->protos[i];
        if (proto.name != 0) {
            printf("%s", chunk->strings[proto.name - 1]);
        } else {
            printf("<anonymous>");
        }
        if (i == chunk->main) {
            printf(" (main):\n");
        } else {
            printf(":\n");
        }
        size_t i = 0;
        while (i < proto.szcode) {
            printf("  ");
            print_instruction(chunk, proto.code, &i);
        }
    }
}
