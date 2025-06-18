/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_BUILTINS_H
#define LEAF_BUILTINS_H

#include "vm/value.h"

/* console I/O */
int lf_print(lfState *state);
int lf_input(lfState *state);
/* array utilities */
int lf_arr_length(lfState *state);
int lf_arr_push(lfState *state);
int lf_arr_pop(lfState *state);
/* string utilities */
int lf_str_split(lfState *state);
int lf_str_contains(lfState *state);
/* casting */
int lf_toint(lfState *state);
int lf_tostring(lfState *state);
int lf_toboolean(lfState *state);
int lf_tofloat(lfState *state);
int lf_toarray(lfState *state);

#endif /* LEAF_BUILTINS_H */
