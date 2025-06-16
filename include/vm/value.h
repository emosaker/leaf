/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_VALUE_H
#define LEAF_VALUE_H

#include <stdint.h>
#include <setjmp.h>
#include <stdbool.h>

#include "compiler/bytecode.h"
#include "lib/array.h"

#define lf_string(VALUE) ((lfString *)((VALUE)->v.gco))
#define lf_array(VALUE) ((lfValueArray *)((VALUE)->v.gco))
#define lf_cl(VALUE) ((lfClosure *)((VALUE)->v.gco))

typedef enum lfValueType {
    LF_NULL,
    LF_INT,
    LF_BOOL,
    /* GC objects */
    LF_CLOSURE,
    LF_STRING,
    LF_ARRAY
} lfValueType;

typedef enum lfGCColor {
    LF_GCBLACK,
    LF_GCWHITE
} lfGCColor;

#define LF_GCHEADER \
    struct lfGCObject *next; \
    lfGCColor gc_color; \
    lfValueType t;

typedef struct lfGCObject {
    LF_GCHEADER;
} lfGCObject;

#define LF_CHECKTOP(STATE) { \
    if (((STATE)->top - (STATE)->stack) >= (STATE)->stack_size) { \
        int top_off = (STATE)->top - (STATE)->stack; \
        int base_off = (STATE)->base - (STATE)->stack; \
        (STATE)->stack_size *= 2; \
        (STATE)->stack = realloc((STATE)->stack, (STATE)->stack_size * sizeof(lfValue)); \
        (STATE)->top = (STATE)->stack + top_off; \
        (STATE)->base = (STATE)->stack + base_off; \
    } \
}

#define LF_STACKSIZE(STATE) ((STATE)->top - (STATE)->base)

typedef lfArray(struct lfValueBucket *) lfValueMap;

typedef struct lfCallFrame {
    struct lfClosure *cl;
    /* for stack re-allocation */
    int top;
    int base;
    /* for stack trace */
    int ip;
} lfCallFrame;

typedef struct lfState {
    int stack_size;
    struct lfValue *stack;
    struct lfValue *base;
    struct lfValue *top;

    lfValueMap globals;
    lfValueMap strays;

    struct lfValue **upvalues;

    lfGCObject *gc_objects;
    lfGCObject *gray_objects;

    lfArray(lfCallFrame) frame;

    bool errored;
    jmp_buf error_buf;
} lfState;

typedef int(*lfccl)(lfState *state);

typedef struct lfClosure {
    LF_GCHEADER;
    bool is_c;
    union {
        struct {
            lfProto *proto;
            struct lfValue *upvalues[];
        } lf;
        struct {
            lfccl func;
            const char *name;
        } c;
    } f;
} lfClosure;

typedef struct lfString {
    LF_GCHEADER;
    int length;
    char string[];
} lfString;

typedef struct lfValueArray {
    LF_GCHEADER;
    lfArray(struct lfValue) values;
} lfValueArray;

typedef struct lfValue {
    lfValueType type;
    union {
        uint64_t integer;
        bool boolean;
        lfGCObject *gco;
    } v;
} lfValue;

typedef struct lfValueBucket {
    struct lfValueBucket *next;
    struct lfValueBucket *previous;
    lfValue key;
    lfValue value;
} lfValueBucket;

/* value map */
lfValueMap lf_valuemap_create(int size);
lfValueMap lf_valuemap_clone(const lfValueMap *map);
bool lf_valuemap_lookup(const lfValueMap *map, const lfValue *key, lfValue *out);
void lf_valuemap_insert(lfValueMap *map, const lfValue *key, const lfValue *value); /* TODO: possibly resize the map when beneficial */
void lf_valuemap_delete(lfValueMap *map);

/* state */
lfState *lf_state_create(void);
void lf_state_delete(lfState *state);

/* globals */
void lf_setglobal(lfState *state, const lfValue *key);
void lf_getglobal(lfState *state, const lfValue *key);
void lf_setsglobal(lfState *state, const char *key);
void lf_getsglobal(lfState *state, const char *key);

/* stack */
void lf_pushint(lfState *state, uint64_t value);
void lf_pushstring(lfState *state, char *value, int length);
void lf_pushbool(lfState *state, bool value);
void lf_pushnull(lfState *state);
void lf_pusharray(lfState *state, int size);
lfValue lf_pop(lfState *state);
void lf_push(lfState *state, const lfValue *value);
void lf_getupval(lfState *state, int index);
void lf_setupval(lfState *state, int index);

/* value */
void lf_newccl(lfState *state, lfccl func, const char *name);
void lf_newlfcl(lfState *state, lfProto *proto);
const char *lf_clname(lfClosure *cl);

const char *lf_typeof(const lfValue *value);
void lf_printvalue(const lfValue *value);

/* gc */
void lf_gc_step(lfState *state);

#endif /* LEAF_VALUE_H */
