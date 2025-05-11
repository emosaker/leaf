/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_ERROR_H
#define LEAF_ERROR_H

#include <stdlib.h>

void error_underline_code(const char *source, size_t idx_start, size_t idx_end);
void error_print(const char *file, const char *source, size_t idx_start, size_t idx_end, const char *message);

#endif /* LEAF_ERROR_H */
