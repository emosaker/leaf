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
            printf("pushshort %d\n", INS_E(ins));
            *i += 1;
            break;
        case OP_PUSHLI:
            printf("pushlong %ld\n", chunk->ints[INS_E(ins)]);
            *i += 1;
            break;
    }
}

void chunk_print(const lfChunk *chunk) {
    for (int i = 0; i < chunk->szprotos; i++) {
        lfProto proto = chunk->protos[i];
        if (proto.name != 0) {
            printf("%s:", chunk->strings[proto.name - 1]);
        } else {
            printf("<anonymous>:");
        }
        if (i == chunk->main) {
            printf(" (main):\n");
        } else {
            printf("\n");
        }
        size_t i = 0;
        while (i < proto.szcode) {
            printf("  ");
            print_instruction(chunk, proto.code, &i);
        }
    }
}
