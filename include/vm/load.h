/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_LOAD_H
#define LEAF_LOAD_H

#include "vm/value.h"

bool lf_load(lfState *state, const char *source, const char *file);

#endif /* LEAF_LOAD_H */
