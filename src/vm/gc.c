/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>

#include "vm/value.h"

void mark(lfValue *v) {
    if (v->type != LF_STRING && v->type != LF_CLOSURE) return;
    lfGCObject *header = (lfGCObject *)v->v.string;
    header->gc_color = LF_GCBLACK;
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
            if (current) {
                while (current->next != NULL) current = current->next;
                while (current->previous) {
                    lfValueBucket *tbm = current;
                    current = current->previous;
                    mark(&tbm->key);
                    mark(&tbm->value);
                }
                mark(&current->key);
                mark(&current->value);
            }
        }
    }

    /* sweep */
    lfGCObject *new = NULL;
    lfGCObject *gco = state->gc_objects;
    while (gco) {
        lfGCObject *next = gco->next;
        if (gco->gc_color == LF_GCWHITE) {
            free(gco);
        } else {
            gco->gc_color = LF_GCWHITE;
            gco->next = new;
            new = gco;
        }
        gco = next;
    }
    state->gc_objects = new;
}
