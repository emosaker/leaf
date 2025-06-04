/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_BYTECODE_H
#define LEAF_BYTECODE_H

#include <stdint.h>
#include <stdlib.h>

typedef enum lfOpCode {
    OP_NOP,

    OP_PUSHSI,
    OP_PUSHLI,
    OP_PUSHS,
    OP_PUSHNULL,
    OP_DUP,

    OP_GETGLOBAL,
    OP_SETGLOBAL,
    OP_INDEX,
    OP_ASSIGN,
    OP_SET,

    OP_NEWARR,
    OP_NEWMAP,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,

    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE,

    OP_BAND,
    OP_BOR,
    OP_BXOR,
    OP_BLSH,
    OP_BRSH,

    OP_AND,
    OP_OR,

    OP_NEG,
    OP_NOT,

    OP_JMP,
    OP_JMPBACK,
    OP_JMPIFNOT,

    OP_CALL,
    OP_RET
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
    uint64_t *ints;
    size_t szprotos;
    lfProto *protos;
    size_t main;
} lfChunk;

#define INS_OP(INSTRUCTION) (((INSTRUCTION) >> 0)  & 0xFF)
#define INS_A(INSTRUCTION)  (((INSTRUCTION) >> 8)  & 0xFF)
#define INS_B(INSTRUCTION)  (((INSTRUCTION) >> 16) & 0xFF)
#define INS_C(INSTRUCTION)  (((INSTRUCTION) >> 24) & 0xFF)
#define INS_E(INSTRUCTION)  (((INSTRUCTION) & 0xFFFFFF00) >> 8)

#endif /* LEAF_BYTECODE_H */
