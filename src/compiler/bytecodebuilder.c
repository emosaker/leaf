/*
 * This file is part of the leaf programming language
 */

#include <string.h>
#include <stdlib.h>
#include "compiler/bytecodebuilder.h"
#include "compiler/bytecode.h"
#include "lib/array.h"

void string_deleter(char **string) {
    free(*string);
}

void lf_proto_deleter(lfProto **proto) {
    for (int i = 0; i < (*proto)->szprotos; i++)
        lf_proto_deleter((*proto)->protos + i);
    for (int i = 0; i < (*proto)->szstrings; i++)
        string_deleter((*proto)->strings + i);
    free((*proto)->protos);
    free((*proto)->strings);
    free((*proto)->ints);
    free((*proto)->floats);
    free((*proto)->code);
    free((*proto)->linenumbers);
    free((*proto));
}


lfBytecodeBuilder bytecodebuilder_init(lfBytecodeBuilder *bb) {
    lfBytecodeBuilder old = *bb;
    *bb = (lfBytecodeBuilder) {
        .bytecode = array_new(uint8_t),
        .lines = array_new(int),
        .ints = array_new(uint64_t),
        .floats = array_new(double),
        .strings = array_new(char *, string_deleter),
        .protos = array_new(lfProto *, lf_proto_deleter)
    };
    return old;
}

void bytecodebuilder_restore(lfBytecodeBuilder *bb, lfBytecodeBuilder old, bool retain) {
    if (bb->bytecode != NULL) array_delete(&bb->bytecode);
    if (bb->ints != NULL) array_delete(&bb->ints);
    if (bb->floats != NULL) array_delete(&bb->floats);
    if (bb->lines != NULL) array_delete(&bb->lines);
    if (bb->strings != NULL) {
        if (retain) deleter(&bb->strings) = NULL;
        array_delete(&bb->strings);
    }
    if (bb->protos != NULL) {
        if (retain) deleter(&bb->protos) = NULL;
        array_delete(&bb->protos);
    }
    *bb = old;
}

void emit_ins_abc(lfBytecodeBuilder *bb, lfOpCode op, uint8_t a, uint8_t b, uint8_t c, int lineno) {
    array_push(&bb->bytecode, op);
    array_push(&bb->bytecode, a);
    array_push(&bb->bytecode, b);
    array_push(&bb->bytecode, c);
    array_push(&bb->lines, lineno);
}

void emit_ins_ad(lfBytecodeBuilder *bb, lfOpCode op, uint8_t a, uint16_t d, int lineno) {
    array_push(&bb->bytecode, op);
    array_push(&bb->bytecode, a);
    array_push(&bb->bytecode, (d >> 0) & 0xFF);
    array_push(&bb->bytecode, (d >> 8) & 0xFF);
    array_push(&bb->lines, lineno);
}

void emit_ins_e(lfBytecodeBuilder *bb, lfOpCode op, uint32_t e, int lineno) {
    array_push(&bb->bytecode, op);
    emit_u24(bb, e);
    array_push(&bb->lines, lineno);
}

void emit_ins_e_at(lfBytecodeBuilder *bb, lfOpCode op, uint32_t e, int idx, int lineno) {
    int curr = length(&bb->bytecode);
    length(&bb->bytecode) = idx;
    emit_ins_e(bb, op, e, lineno);
    length(&bb->bytecode) = curr;
}

void emit_op(lfBytecodeBuilder *bb, lfOpCode op, int lineno) {
    emit_ins_e(bb, op, 0, lineno);
}

void emit_u24(lfBytecodeBuilder *bb, uint32_t value) {
    array_push(&bb->bytecode, (value >> 0)  & 0xFF);
    array_push(&bb->bytecode, (value >> 8)  & 0xFF);
    array_push(&bb->bytecode, (value >> 16) & 0xFF);
}

uint32_t new_u64(lfBytecodeBuilder *bb, uint64_t value) {
    array_push(&bb->ints, value);
    return length(&bb->ints) - 1;
}

uint32_t new_f64(lfBytecodeBuilder *bb, double value) {
    array_push(&bb->floats, value);
    return length(&bb->floats) - 1;
}

uint32_t new_string(lfBytecodeBuilder *bb, char *value, int length) {
    for (int i = 0; i < length(&bb->strings); i++) { /* TODO: Replace with a map for O(1) insertion */
        if (!strncmp(bb->strings[i], value, length))
            return i;
    }
    char *clone = malloc(length + 1);
    memcpy(clone, value, length);
    clone[length] = 0;
    array_push(&bb->strings, clone);
    return length(&bb->strings) - 1;
}

lfProto *bytecodebuilder_allocproto(lfBytecodeBuilder *bb, char *name, int szupvalues, int szargs) {
    lfProto *proto = malloc(sizeof(lfProto));
    proto->code = malloc(length(&bb->bytecode));
    proto->szcode = length(&bb->bytecode) / 4;
    if (name) proto->name = new_string(bb, name, strlen(name)) + 1;
    else proto->name = 0;
    proto->protos = malloc(sizeof(lfProto *) * length(&bb->protos));
    proto->szprotos = length(&bb->protos);
    proto->strings = malloc(sizeof(char *) * length(&bb->strings));
    proto->szstrings = length(&bb->strings);
    proto->ints = malloc(sizeof(uint64_t) * length(&bb->ints));
    proto->szints = length(&bb->ints);
    proto->floats = malloc(sizeof(double) * length(&bb->floats));
    proto->szfloats = length(&bb->floats);
    proto->linenumbers = malloc(sizeof(int) * length(&bb->lines));
    proto->szlinenumbers = length(&bb->lines);
    proto->szupvalues = szupvalues;
    proto->szargs = szargs;

    memcpy(proto->code, bb->bytecode, length(&bb->bytecode));
    memcpy(proto->protos, bb->protos, sizeof(lfProto *) * length(&bb->protos));
    memcpy(proto->strings, bb->strings, sizeof(char *) * length(&bb->strings));
    memcpy(proto->ints, bb->ints, sizeof(uint64_t) * length(&bb->ints));
    memcpy(proto->floats, bb->floats, sizeof(double) * length(&bb->floats));
    memcpy(proto->linenumbers, bb->lines, sizeof(int) * length(&bb->lines));

    return proto;
}

lfProto *lf_proto_clone(lfProto *proto) {
    lfProto *new = malloc(sizeof(lfProto));
    new->strings = malloc(sizeof(char *) * proto->szstrings);
    new->szstrings = proto->szstrings;
    new->ints = malloc(sizeof(uint64_t) * proto->szints);
    new->szints = proto->szints;
    new->floats = malloc(sizeof(double) * proto->szfloats);
    new->szfloats = proto->szfloats;
    new->protos = malloc(sizeof(lfProto *) * proto->szprotos);
    new->szprotos = proto->szprotos;
    new->code = malloc(sizeof(uint32_t) * proto->szcode);
    new->szcode = proto->szcode;
    new->linenumbers = malloc(sizeof(int) * proto->szlinenumbers);
    new->szlinenumbers = proto->szlinenumbers;
    new->name = proto->name;
    new->szargs = proto->szargs;
    new->szupvalues = proto->szupvalues;

    for (int i = 0; i < proto->szstrings; i++) {
        char *clone = malloc(strlen(proto->strings[i]) + 1);
        memcpy(clone, proto->strings[i], strlen(proto->strings[i]) + 1);
        new->strings[i] = clone;
    }

    for (int i = 0; i < proto->szprotos; i++) {
        new->protos[i] = lf_proto_clone(proto->protos[i]);
    }

    memcpy(new->ints, proto->ints, proto->szints * sizeof(uint64_t));
    memcpy(new->floats, proto->floats, proto->szfloats * sizeof(double));
    memcpy(new->code, proto->code, proto->szcode * sizeof(uint32_t));
    memcpy(new->linenumbers, proto->linenumbers, proto->szlinenumbers * sizeof(int));

    return new;
}
