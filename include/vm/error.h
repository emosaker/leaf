/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_ERROR_H
#define LEAF_ERROR_H

#include "vm/state.h"

void lf_error(lfState *state, const char *message);
void lf_errorf(lfState *state, const char *fmt, ...);

#endif /* LEAF_ERROR_H */
