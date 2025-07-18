/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_ERROR_H
#define LEAF_ERROR_H

void lf_error_underline_code(const char *source, int idx_start, int idx_end);
void lf_error_print(const char *file, const char *source, int idx_start, int idx_end, const char *message);

#endif /* LEAF_ERROR_H */
