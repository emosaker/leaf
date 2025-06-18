/*
 * This file is part of the leaf programming language
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/array.h"
#include "vm/error.h"
#include "vm/value.h"
#include "vm/builtins.h"

/* console I/O */
int lf_print(lfState *state) {
    int nargs = LF_STACKSIZE(state);
    for (int i = 0; i < nargs; i++) {
        lf_printvalue(state->base + i);
        if (i < nargs - 1)
            printf(", ");
    }
    printf("\n");
    return 0;
}

int lf_input(lfState *state) {
    if (LF_STACKSIZE(state) > 0) {
        lf_printvalue(state->base);
    }
    lfArray(char) buf = array_new(char);
    char next;
    while ((next = getchar()) != '\n') {
        array_push(&buf, next);
    }
    lf_pushstring(state, buf, length(&buf));
    array_delete(&buf);
    return 1;
}

/* array utilities */
int lf_arr_length(lfState *state) {
    lf_checkargs(state, 1);
    lf_checkargtype(state, 0, LF_ARRAY);
    lfValueArray *arr = lf_arrayvalue(state, 0);
    lf_pushint(state, length(&arr->values));
    return 1;
}

int lf_arr_push(lfState *state) {
    lf_checkargs(state, 2);
    lf_checkargtype(state, 0, LF_ARRAY);
    lfValueArray *arr = lf_arrayvalue(state, 0);
    lfValue v = lf_pop(state);
    array_push(&arr->values, v);
    return 0;
}

int lf_arr_pop(lfState *state) {
    lf_checkargs(state, 1);
    lf_checkargtype(state, 0, LF_ARRAY);
    lfValueArray *arr = lf_arrayvalue(state, 0);
    lf_push(state, &arr->values[--length(&arr->values)]);
    return 1;
}

/* string utilities */
int lf_str_split(lfState *state) {
    lf_checkargs(state, 2);
    lf_checkargtype(state, 0, LF_STRING);
    lf_checkargtype(state, 1, LF_STRING);
    lf_pusharray(state, 0);
    lfValueArray *arr = lf_arrayvalue(state, 2);
    int string_len;
    char *string = lf_stringvalue(state, 0, &string_len);
    int delim_len;
    char *delim = lf_stringvalue(state, 1, &delim_len);
    lfArray(char) buf = array_new(char);
    for (int i = 0; i < string_len - delim_len + 1; i++) {
        if (!memcmp(string + i, delim, delim_len)) {
            lf_pushstring(state, buf, length(&buf));
            array_push(&arr->values, lf_pop(state));
            length(&buf) = 0;
            i += delim_len;
        }
        array_push(&buf, string[i]);
    }
    lf_pushstring(state, buf, length(&buf));
    array_push(&arr->values, lf_pop(state));
    array_delete(&buf);
    return 1;
}

int lf_str_contains(lfState *state) {
    lf_checkargs(state, 2);
    lf_checkargtype(state, 0, LF_STRING);
    lf_checkargtype(state, 1, LF_STRING);
    int string_len;
    char *string = lf_stringvalue(state, 0, &string_len);
    int sub_len;
    char *sub = lf_stringvalue(state, 1, &sub_len);
    for (int i = 0; i < string_len - sub_len + 1; i++) {
        if (!memcmp(string + i, sub, sub_len)) {
            lf_pushbool(state, true);
            return 1;
        }
    }
    lf_pushbool(state, false);
    return 1;
}

/* casting */
int lf_toint(lfState *state) {
    lf_checkargs(state, 1);
    lfValue v = lf_pop(state);
    switch (v.type) {
        case LF_INT:
            lf_push(state, &v);
            return 1;
        case LF_FLOAT:
            lf_pushint(state, (uint64_t)v.v.floating);
            return 1;
        case LF_STRING: {
            uint64_t acc = 0;
            for (int i = 0; i < lf_string(&v)->length; i++) {
                int digit = lf_string(&v)->string[i] - '0';
                if (digit < 0 || digit > 9)
                    lf_errorf(state, "non numeric character passed to int at index %d", i);
                acc += digit * pow(10, lf_string(&v)->length - i - 1);
            }
            lf_pushint(state, acc);
            return 1;
        }
        case LF_BOOL:
            lf_pushint(state, (uint64_t)v.v.boolean);
            return 1;
        case LF_NULL:
            lf_pushint(state, 0);
            return 1;
        default:
            break;
    }
    lf_errorf(state, "cannot cast %s to int", lf_typeof(&v));
    return 0;
}

int lf_tostring(lfState *state) {
    lf_checkargs(state, 1);
    lfValue v = lf_pop(state);
    switch (v.type) {
        case LF_INT: {
            int len = (int)ceil(log10(v.v.integer))+1;
            char *buf = malloc(len);
            snprintf(buf, len, "%ld", v.v.integer);
            lf_pushstring(state, buf, len - 1);
            free(buf);
            return 1;
        }
        case LF_FLOAT: {
            char buf[100];
            snprintf(buf, 50, "%lf", v.v.floating);
            lf_pushstring(state, buf, strlen(buf));
            return 1;
        }
        case LF_STRING:
            lf_push(state, &v);
            return 1;
        case LF_BOOL:
            if (v.v.boolean) lf_pushstring(state, "true", 4);
            else lf_pushstring(state, "false", 5);
            return 1;
        case LF_NULL:
            lf_pushstring(state, "null", 4);
            return 1;
        default:
            break;
    }
    lf_errorf(state, "cannot cast %s to string", lf_typeof(&v));
    return 0;
}

int lf_toboolean(lfState *state) {
    lf_checkargs(state, 1);
    lfValue v = lf_pop(state);
    switch (v.type) {
        case LF_INT:
            lf_pushbool(state, v.v.integer != 0);
            return 1;
        case LF_FLOAT:
            lf_pushbool(state, v.v.floating != 0.0f);
            return 1;
        case LF_STRING:
            lf_pushbool(state, true);
            return 1;
        case LF_BOOL:
            lf_push(state, &v);
            return 1;
        case LF_NULL:
            lf_pushbool(state, false);
            return 1;
        default:
            break;
    }
    lf_errorf(state, "cannot cast %s to boolean", lf_typeof(&v));
    return 0;
}

int lf_tofloat(lfState *state) {
    lf_checkargs(state, 1);
    lfValue v = lf_pop(state);
    switch (v.type) {
        case LF_INT:
            lf_pushfloat(state, (double)v.v.integer);
            return 1;
        case LF_FLOAT:
            lf_push(state, &v);
            return 1;
        case LF_STRING: {
            char *tmp = malloc(lf_string(&v)->length + 1);
            memcpy(tmp, lf_string(&v)->string, lf_string(&v)->length);
            tmp[lf_string(&v)->length] = 0;
            lf_pushfloat(state, strtof(tmp, NULL));
            free(tmp);
            return 1;
        }
        case LF_BOOL:
            lf_pushfloat(state, v.v.boolean ? 1.0f : 0.0f);
            return 1;
        case LF_NULL:
            lf_pushfloat(state, 0.0f);
            return 1;
        default:
            break;
    }
    lf_errorf(state, "cannot cast %s to float", lf_typeof(&v));
    return 0;
}

int lf_toarray(lfState *state) {
    lf_checkargs(state, 1);
    lfValue v = lf_pop(state);
    switch (v.type) {
        case LF_STRING: {
            lfString *s = lf_string(&v);
            lf_pusharray(state, s->length);
            for (int i = 0; i < s->length; i++) {
                lf_pushstring(state, s->string + i, 1);
                lf_pushto(state, 0);
            }
            return 1;
        }
        default:
            break;
    }
    lf_errorf(state, "cannot cast %s to array", lf_typeof(&v));
    return 0;
}
