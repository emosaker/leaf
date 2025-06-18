/*
 * This file is part of the leaf programming language
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "compiler/bytecode.h"
#include "vm/error.h"
#include "vm/value.h"

void lf_setglobal(lfState *state, const lfValue *key) {
    lfValue v = lf_pop(state);
    lf_valuemap_insert(&state->globals, key, &v);
}

void lf_getglobal(lfState *state, const lfValue *key) {
    lfValue v;
    if (lf_valuemap_lookup(&state->globals, key, &v)) {
        lf_push(state, &v);
    } else {
        lf_pushnull(state);
    }
}

void lf_setsglobal(lfState *state, const char *key) {
    lf_pushstring(state, (char *)key, strlen(key));
    lfValue v = lf_pop(state);
    lf_setglobal(state, &v);
}

void lf_getsglobal(lfState *state, const char *key) {
    lf_pushstring(state, (char *)key, strlen(key));
    lfValue v = lf_pop(state);
    lf_getglobal(state, &v);
}

void lf_pushint(lfState *state, uint64_t value) {
    LF_CHECKTOP(state);
    *state->top++ = (lfValue) {
        .type = LF_INT,
        .v.integer = value
    };
}

lfString *alloc_string(lfState *state, int length) {
    lfString *s = malloc(sizeof(lfString) + length + 1);
    s->type = LF_STRING;
    s->length = length;
    s->gc_color = LF_GCWHITE;
    array_push(&state->gc_objects, (lfGCObject *)s);
    return s;
}

void lf_pushstring(lfState *state, char *value, int length) {
    LF_CHECKTOP(state);
    lfString *s = alloc_string(state, length);
    memcpy(s->string, value, length);
    s->string[length] = 0;
    *state->top++ = (lfValue) {
        .type = LF_STRING,
        .v.gco = (lfGCObject *)s
    };
}

void lf_pushbool(lfState *state, bool value) {
    LF_CHECKTOP(state);
    *state->top++ = (lfValue) {
        .type = LF_BOOL,
        .v.boolean = value
    };
}

void lf_pushnull(lfState *state) {
    LF_CHECKTOP(state);
    *state->top++ = (lfValue) {
        .type = LF_NULL
    };
}

lfValue lf_pop(lfState *state) {
    if (LF_STACKSIZE(state) < 1)
        lf_error(state, "attempt to pop empty stack");
    return *--state->top;
}

void lf_push(lfState *state, const lfValue *value) {
    LF_CHECKTOP(state);
    *state->top++ = *value;
}

void lf_getupval(lfState *state, int index) {
    lf_push(state, state->upvalues[index]);
}

void lf_setupval(lfState *state, int index) {
    lfValue v = lf_pop(state);
    *state->upvalues[index] = v;
}

lfClosure *alloc_ccl(lfState *state) {
    lfClosure *cl = malloc(sizeof(lfClosure));
    cl->type = LF_CLOSURE;
    cl->gc_color = LF_GCWHITE;
    array_push(&state->gc_objects, (lfGCObject *)cl);
    return cl;
}

void lf_newccl(lfState *state, lfccl func, const char *name) {
    LF_CHECKTOP(state);
    lfClosure *cl = alloc_ccl(state);
    cl->is_c = true;
    cl->f.c.func = func;
    cl->f.c.name = name;
    *state->top++ = (lfValue) {
        .type = LF_CLOSURE,
        .v.gco = (lfGCObject *)cl
    };
}

lfClosure *alloc_lfcl(lfState *state, int szupvalues) {
    lfClosure *cl = malloc(sizeof(lfClosure) + sizeof(lfValue *) * szupvalues);
    cl->type = LF_CLOSURE;
    cl->gc_color = LF_GCWHITE;
    array_push(&state->gc_objects, (lfGCObject *)cl);
    return cl;
}

void lf_newlfcl(lfState *state, lfProto *proto) {
    LF_CHECKTOP(state);
    lfClosure *cl = alloc_lfcl(state, proto->szupvalues);
    cl->is_c = false;
    cl->f.lf.proto = lf_proto_clone(proto);
    *state->top++ = (lfValue) {
        .type = LF_CLOSURE,
        .v.gco = (lfGCObject *)cl
    };
}

const char *lf_clname(lfClosure *cl) {
    if (cl->is_c) {
        return cl->f.c.name != NULL ? cl->f.c.name : "<anonymous>";
    }
    lfProto *p = cl->f.lf.proto;
    if (p->name) {
        return p->strings[p->name - 1];
    }
    return "<anonymous>";
}

lfValueArray *alloc_array(lfState *state) {
    lfValueArray *arr = malloc(sizeof(lfValueArray));
    arr->type = LF_ARRAY;
    arr->gc_color = LF_GCWHITE;
    array_push(&state->gc_objects, (lfGCObject *)arr);
    return arr;
}

void lf_pusharray(lfState *state, int size) {
    LF_CHECKTOP(state);
    lfValueArray *arr = alloc_array(state);
    arr->values = array_new(lfValue);
    array_reserve(&arr->values, size);
    *state->top++ = (lfValue) {
        .type = LF_ARRAY,
        .v.gco = (lfGCObject *)arr
    };
}

void lf_add(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushint(state, lhs.v.integer + rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        case LF_STRING:
            if (rhs.type != LF_STRING) {
                goto unsupported;
            }
            int len = lf_string(&lhs)->length + lf_string(&rhs)->length;
            char *concatenated = malloc(len + 1);
            memcpy(concatenated, lf_string(&lhs)->string, lf_string(&lhs)->length);
            memcpy(concatenated + lf_string(&lhs)->length, lf_string(&rhs)->string, lf_string(&rhs)->length);
            concatenated[len] = 0;
            lf_pushstring(state, concatenated, len);
            free(concatenated);
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for addition: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_sub(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushint(state, lhs.v.integer - rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for subtraction: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_mul(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushint(state, lhs.v.integer * rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for multiplication: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_div(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushint(state, lhs.v.integer / rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for division: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_pow(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushint(state, (uint64_t)pow((double)lhs.v.integer, (double)rhs.v.integer));
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for exponents: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_eq(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    lf_pushbool(state, lf_valuemap_compare_values(&lhs, &rhs));
}

void lf_ne(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    lf_pushbool(state, !lf_valuemap_compare_values(&lhs, &rhs));
}

void lf_lt(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushbool(state, lhs.v.integer < rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for comparison: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_gt(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushbool(state, lhs.v.integer > rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for comparison: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_le(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushbool(state, lhs.v.integer <= rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for comparison: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_ge(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushbool(state, lhs.v.integer >= rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for comparison: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_band(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushint(state, lhs.v.integer & rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for bitwise operation: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_bor(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushint(state, lhs.v.integer | rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for bitwise operation: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_bxor(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushint(state, lhs.v.integer ^ rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for bitwise operation: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_blsh(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushint(state, lhs.v.integer << rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for bitwise operation: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_brsh(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushint(state, lhs.v.integer >> rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for bitwise operation: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_and(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushbool(state, lhs.v.integer && rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for comparison: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_or(lfState *state) {
    lfValue rhs = lf_pop(state);
    lfValue lhs = lf_pop(state);
    switch (lhs.type) {
        case LF_INT:
            if (rhs.type == LF_INT) {
                lf_pushbool(state, lhs.v.integer || rhs.v.integer);
            } /* TODO: float handler */ else {
                goto unsupported;
            }
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported types for comparison: %s and %s", lf_typeof(&lhs), lf_typeof(&rhs));
    }
}

void lf_neg(lfState *state) {
    lfValue v = lf_pop(state);
    switch (v.type) {
        case LF_INT:
            lf_pushint(state, -v.v.integer);
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported type for negation: %s and %s", lf_typeof(&v));
    }
}

void lf_not(lfState *state) {
    lfValue v = lf_pop(state);
    switch (v.type) {
        case LF_INT:
            lf_pushbool(state, !v.v.integer);
            break;
        case LF_BOOL:
            lf_pushbool(state, !v.v.boolean);
            break;
        unsupported:
        default:
            lf_errorf(state, "unsupported type for comparison: %s and %s", lf_typeof(&v));
    }
}

const char *lf_typeof(const lfValue *value) {
    switch (value->type) {
        case LF_NULL: return "null";
        case LF_INT: return "int";
        case LF_BOOL: return "bool";
        case LF_STRING: return "string";
        case LF_ARRAY: return "array";
        case LF_CLOSURE: return "closure";
        default: return "invalid";
    }
}

void lf_printvalue(const lfValue *value) {
    switch (value->type) {
        case LF_NULL:
            printf("null");
            break;
        case LF_INT:
            printf("%ld", value->v.integer);
            break;
        case LF_BOOL:
            printf("%s", value->v.boolean ? "true" : "false");
            break;
        case LF_STRING:
            printf("%s", lf_string(value)->string);
            break;
        case LF_ARRAY:
            printf("{");
            for (int i = 0; i < length(&lf_array(value)->values); i++) {
                lf_printvalue(lf_array(value)->values + i);
                if (i < length(&lf_array(value)->values) - 1)
                    printf(", ");
            }
            printf("}");
            break;
        case LF_CLOSURE:
            if (!lf_cl(value)->is_c) {
                lfProto *p = lf_cl(value)->f.lf.proto;
                if (p->name)
                    printf("<leaf closure '%s'>", p->strings[p->name - 1]);
                else
                    printf("<anonymous leaf closure>");
            } else {
                if (lf_cl(value)->f.c.name) {
                    printf("<c closure '%s'>", lf_cl(value)->f.c.name);
                } else {
                    printf("<anonymous c closure>");
                }
            }
            break;
        default:
            printf("<invalid>");
    }
}
