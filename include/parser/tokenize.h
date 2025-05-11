/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_TOKENIZE_H
#define LEAF_TOKENIZE_H

#include "parser/token.h"
#include "lib/array.h"

lfArray(lfToken) lf_tokenize(const char *source, const char *file);

#endif /* LEAF_TOKENIZE_H */
