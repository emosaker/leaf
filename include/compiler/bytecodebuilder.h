/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_BYTECODEBUILDER_H
#define LEAF_BYTECODEBUILDER_H

#include <stdbool.h>

#include "compiler/bytecode.h"
#include "lib/array.h"

typedef struct lfBytecodeBuilder {
    lfArray(uint8_t) bytecode;
    lfArray(int) lines;
    lfArray(uint64_t) ints;
    lfArray(char *) strings;
    lfArray(lfProto *) protos;
} lfBytecodeBuilder;

lfBytecodeBuilder bytecodebuilder_init(lfBytecodeBuilder *bb);
void bytecodebuilder_restore(lfBytecodeBuilder *bb, lfBytecodeBuilder old, bool retain);
void emit_op(lfBytecodeBuilder *bb, lfOpCode op, int lineno);
void emit_ins_abc(lfBytecodeBuilder *bb, lfOpCode op, uint8_t a, uint8_t b, uint8_t c, int lineno);
void emit_ins_ad(lfBytecodeBuilder *bb, lfOpCode op, uint8_t a, uint16_t d, int lineno);
void emit_ins_e(lfBytecodeBuilder *bb, lfOpCode op, uint32_t e, int lineno);
void emit_ins_e_at(lfBytecodeBuilder *bb, lfOpCode op, uint32_t e, int idx, int lineno);
void emit_u24(lfBytecodeBuilder *bb, uint32_t value);

uint32_t new_u64(lfBytecodeBuilder *bb, uint64_t value);
uint32_t new_string(lfBytecodeBuilder *bb, char *value, int length);

lfProto *bytecodebuilder_allocproto(lfBytecodeBuilder *bb, char *name, int szupvalues, int szargs);

#endif /* LEAF_BYTECODEBUILDER_H */
