/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_BYTECODE_H
#define LEAF_BYTECODE_H

#include <stdint.h>
#include <stdlib.h>

typedef enum lfOpCode {
    OP_PUSHSI,
    OP_PUSHLI,
} lfOpCode;

typedef struct lfProto {
    size_t szcode;
    uint32_t *code;
    size_t name;
} lfProto;

typedef struct lfChunk {
    size_t szstrings;
    char **strings;
    size_t szints;
    int64_t *ints;
    size_t szprotos;
    lfProto *protos;
    size_t main;
} lfChunk;

#define INS_OP(INSTRUCTION) (((INSTRUCTION) >> 0)  & 0xFF)
#define INS_A(INSTRUCTION)  (((INSTRUCTION) >> 8)  & 0xFF)
#define INS_B(INSTRUCTION)  (((INSTRUCTION) >> 16) & 0xFF)
#define INS_C(INSTRUCTION)  (((INSTRUCTION) >> 24) & 0xFF)
#define INS_E(INSTRUCTION)  (INS_A(INSTRUCTION) | INS_B(INSTRUCTION) << 8 | INS_C(INSTRUCTION) << 16)

#endif /* LEAF_BYTECODE_H */
