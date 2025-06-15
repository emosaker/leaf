/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_VM_H
#define LEAF_VM_H

#include "vm/value.h"

void lf_call(lfState *state, int nargs, int nret);
int lf_run(lfState *state);

#endif /* LEAF_VM_H */
