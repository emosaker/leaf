/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>

#include "compiler/bytecode.h"
#include "lib/array.h"
#include "vm/value.h"

void mark(lfState *state, lfValue *v) {
    if (v->type != LF_STRING && v->type != LF_CLOSURE && v->type != LF_ARRAY) return;
    lfGCObject *header = v->v.gco;
    if (header->t == LF_STRING || header->t == LF_CLOSURE) {
        header->gc_color = LF_GCBLACK;
    } else if (header->t == LF_ARRAY) {
        header->next = state->gray_objects;
        state->gray_objects = header;
    }
}

void delete(lfGCObject *gco) {
    switch (gco->t) {
        case LF_CLOSURE:
            if (!((lfClosure *)gco)->is_c)
                lf_proto_deleter(&((lfClosure *)gco)->f.lf.proto);
            break;
        case LF_ARRAY:
            array_delete(&((lfValueArray *)gco)->values);
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
        mark(state, current);
        current += 1;
    }
    /* mark the globals */
    if (state->globals != NULL) {
        for (int i = 0; i < size(&state->globals); i++) {
            lfValueBucket *current = state->globals[i];
            while (current) {
                mark(state, &current->key);
                mark(state, &current->value);
                current = current->next;
            }
        }
    }

    /* handle gray objects */
    while (state->gray_objects) {
        lfGCObject *new = NULL;
        lfGCObject *gco = state->gray_objects;
        while (gco) {
            lfGCObject *next = gco->next;
            if (gco->t == LF_ARRAY) {
                lfValueArray *arr = (lfValueArray *)gco;
                for (int i = 0; i < length(&arr->values); i++) {
                    if (arr->values[i].type == LF_STRING || arr->values[i].type == LF_CLOSURE || arr->values[i].type == LF_ARRAY) {
                        lfGCObject *header = arr->values[i].v.gco;
                        if (header->t == LF_STRING || header->t == LF_CLOSURE) {
                            header->gc_color = LF_GCBLACK;
                        } else if (header->t == LF_ARRAY) {
                            header->next = new;
                            new = header;
                        }
                    }
                }
            }
            gco->gc_color = LF_GCBLACK;
            gco->next = state->gc_objects;
            state->gc_objects = gco;
            gco = next;
        }
        state->gray_objects = new;
    }

    /* mark strays */
    if (state->strays != NULL) {
        for (int i = 0; i < size(&state->strays); i++) {
            lfValueBucket *current = state->strays[i];
            lfValueBucket *previous = NULL;
            while (current) {
                if (lf_cl(&current->key)->gc_color != LF_GCBLACK) {
                    if (previous) {
                        previous->next = current->next;
                    } else {
                        state->strays[i] = current->next;
                    }
                } else {
                    mark(state, &current->value);
                }
                previous = current;
                current = current->next;
            }
        }
    }

    if (state->gray_objects == NULL) {
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
}
