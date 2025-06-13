/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>

#include "compiler/bytecode.h"
#include "vm/value.h"

void mark(lfValue *v) {
    if (v->type != LF_STRING && v->type != LF_CLOSURE) return;
    lfGCObject *header = v->v.gco;
    header->gc_color = LF_GCBLACK;
}

void delete(lfGCObject *gco) {
    switch (gco->t) {
        case LF_CLOSURE:
            if (!((lfClosure *)gco)->is_c)
                lf_proto_deleter(&((lfClosure *)gco)->f.lf.proto);
            break;
        default:
            break;
    }
    free(gco);
}

void lf_gc_step(lfState *state) {
    /* mark the stack */
    lfValue *current = state->stack;
    while (current < state->top) {
        mark(current);
        current += 1;
    }
    /* mark the globals */
    if (state->globals != NULL) {
        for (size_t i = 0; i < size(&state->globals); i++) {
            lfValueBucket *current = state->globals[i];
            while (current) {
                mark(&current->key);
                mark(&current->value);
                current = current->next;
            }
        }
    }

    /* sweep */
    lfGCObject *new = NULL;
    lfGCObject *gco = state->gc_objects;
    while (gco) {
        lfGCObject *next = gco->next;
        if (gco->gc_color == LF_GCWHITE) {
            delete(gco);
        } else {
            gco->gc_color = LF_GCWHITE;
            gco->next = new;
            new = gco;
        }
        gco = next;
    }
    state->gc_objects = new;
}
