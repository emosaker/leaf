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
    OP_PUSHBOOL,
    OP_PUSHS,
    OP_PUSHNULL,
    OP_DUP,
    OP_POP,

    OP_GETGLOBAL,
    OP_SETGLOBAL,
    OP_GETUPVAL,
    OP_SETUPVAL,
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

    OP_CL,
    OP_CAPTURE,
    OP_RET,

    OP_CLS
} lfOpCode;

typedef struct lfUpValue {
    enum {
        UVT_REF,
        UVT_IDX
    } by;
    uint32_t index;
} lfUpValue;

typedef struct lfProto {
    size_t szcode;
    uint32_t *code;
    size_t szstrings;
    char **strings;
    size_t szints;
    uint64_t *ints;
    size_t szfloats;
    double *floats;
    size_t szprotos;
    struct lfProto **protos;

    int nupvalues;
    int nargs;

    size_t name;
    size_t line;
} lfProto;

void lf_proto_deleter(lfProto **proto);
lfProto *lf_proto_clone(lfProto *proto);

#define INS_OP(INSTRUCTION) (((INSTRUCTION) >> 0)  & 0xFF)
#define INS_A(INSTRUCTION)  (((INSTRUCTION) >> 8)  & 0xFF)
#define INS_B(INSTRUCTION)  (((INSTRUCTION) >> 16) & 0xFF)
#define INS_C(INSTRUCTION)  (((INSTRUCTION) >> 24) & 0xFF)
#define INS_D(INSTRUCTION)  (((INSTRUCTION) & 0xFFFF0000) >> 16)
#define INS_E(INSTRUCTION)  (((INSTRUCTION) & 0xFFFFFF00) >> 8)

#endif /* LEAF_BYTECODE_H */
