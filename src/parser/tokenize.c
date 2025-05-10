/*
 * This file is part of the leaf programming language
 */

#include <stdbool.h>

#include "parser/tokenize.h"
#include "lib/array.h"
#include "parser/token.h"

void token_deleter(lfToken *tok) {
    if (tok->value != NULL) {
        free(tok->value);
    }
}

static inline lfToken token_single(lfTokenType type, size_t idx) {
    return (lfToken) {
        .type = type,
        .value = NULL,
        .idx_start = idx,
        .idx_end = idx + 1
    };
}

static inline lfToken token_double(lfTokenType type, size_t idx) {
    return (lfToken) {
        .type = type,
        .value = NULL,
        .idx_start = idx,
        .idx_end = idx + 2
    };
}

static inline void token_singledouble(const char *source, size_t *index, lfArray(lfToken) *tokens, lfTokenType ttsingle, lfTokenType ttdouble, char doublematch) {
    if (source[*index + 1] == doublematch) {
        array_push(tokens, token_double(ttdouble, *index));
        *index += 2;
        return;
    }
    array_push(tokens, token_single(ttsingle, *index));
    *index += 1;
}

static inline void token_singledoubledouble(const char *source, size_t *index, lfArray(lfToken) *tokens, lfTokenType ttsingle, lfTokenType ttdouble1, lfTokenType ttdouble2, char doublematch1, char doublematch2) {
    if (source[*index + 1] == doublematch1) {
        array_push(tokens, token_double(ttdouble1, *index));
        *index += 2;
        return;
    } else if (source[*index + 1] == doublematch2) {
        array_push(tokens, token_double(ttdouble2, *index));
        *index += 2;
        return;
    }
    array_push(tokens, token_single(ttsingle, *index));
    *index += 1;
}

lfArray(lfToken) lf_tokenize(const char *source, const char *file) {
    lfArray(lfToken) tokens = array_new(lfToken, token_deleter);

    size_t i = 0;
    while (source[i]) {
        switch (source[i]) {
            case '+':
                token_singledouble(source, &i, &tokens, TT_ADD, TT_ADDASSIGN, '=');
                break;
            case '-':
                token_singledouble(source, &i, &tokens, TT_SUB, TT_SUBASSIGN, '=');
                break;
            case '*':
                token_singledouble(source, &i, &tokens, TT_MUL, TT_MULASSIGN, '=');
                break;
            case '/':
                token_singledouble(source, &i, &tokens, TT_DIV, TT_DIVASSIGN, '=');
                break;

            case '&':
                token_singledouble(source, &i, &tokens, TT_BAND, TT_AND, '&');
                break;
            case '|':
                token_singledouble(source, &i, &tokens, TT_BOR, TT_OR, '|');
                break;

            case '=':
                token_singledouble(source, &i, &tokens, TT_ASSIGN, TT_EQ, '=');
                break;
            case '!':
                token_singledouble(source, &i, &tokens, TT_NOT, TT_NE, '=');
                break;
            case '<':
                token_singledoubledouble(source, &i, &tokens, TT_LT, TT_LE, TT_LSHIFT, '=', '<');
                break;
            case '>':
                token_singledoubledouble(source, &i, &tokens, TT_GT, TT_GE, TT_RSHIFT, '=', '>');
                break;

            case '(':
                array_push(&tokens, token_single(TT_LPAREN, i));
                i += 1;
                break;
            case ')':
                array_push(&tokens, token_single(TT_RPAREN, i));
                i += 1;
                break;
            case '{':
                array_push(&tokens, token_single(TT_LBRACE, i));
                i += 1;
                break;
            case '}':
                array_push(&tokens, token_single(TT_RBRACE, i));
                i += 1;
                break;
            case '[':
                array_push(&tokens, token_single(TT_LBRACKET, i));
                i += 1;
                break;
            case ']':
                array_push(&tokens, token_single(TT_RBRACKET, i));
                i += 1;
                break;
        }
    }

    return tokens;
}
