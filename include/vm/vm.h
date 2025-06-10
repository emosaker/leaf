/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_VM_H
#define LEAF_VM_H

#include "compiler/bytecode.h"
#include "vm/value.h"

void lf_run(lfState *state, lfProto *proto);

#endif /* LEAF_VM_H */
