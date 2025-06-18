/*
 * This file is part of the leaf programming language
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "parser/tokenize.h"
#include "parser/token.h"
#include "lib/array.h"
#include "lib/error.h"

const char *keywords[] = {
    /* var decl */
    "var", "const", "ref",
    /* functions and classes */
    "fn", "class", "struct",
    /* control flow */
    "if", "else", "while", "for", "continue", "break", "return",
    /* imports */
    "include",
    NULL
};

void lf_token_deleter(lfToken *tok) {
    if (tok->value != NULL) {
        if (tok->type == TT_STRING) {
            array_delete(&tok->value);
            tok->value = NULL;
        } else {
            free(tok->value);
            tok->value = NULL;
        }
    }
}

static inline lfToken token_single(lfTokenType type, int idx) {
    return (lfToken) {
        .type = type,
        .value = NULL,
        .idx_start = idx,
        .idx_end = idx + 1
    };
}

static inline lfToken token_double(lfTokenType type, int idx) {
    return (lfToken) {
        .type = type,
        .value = NULL,
        .idx_start = idx,
        .idx_end = idx + 2
    };
}

static inline void token_singledouble(const char *source, int *index, lfArray(lfToken) *tokens, lfTokenType ttsingle, lfTokenType ttdouble, char doublematch) {
    if (source[*index + 1] == doublematch) {
        array_push(tokens, token_double(ttdouble, *index));
        *index += 2;
        return;
    }
    array_push(tokens, token_single(ttsingle, *index));
    *index += 1;
}

static inline void token_singledoubledouble(const char *source, int *index, lfArray(lfToken) *tokens, lfTokenType ttsingle, lfTokenType ttdouble1, lfTokenType ttdouble2, char doublematch1, char doublematch2) {
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
    lfArray(lfToken) tokens = array_new(lfToken, lf_token_deleter);

    int i = 0;
    while (source[i]) {
        switch (source[i]) {
            case '\n':
            case ' ':
            case '\t':
                i += 1;
                break;
            case '+':
                token_singledouble(source, &i, &tokens, TT_ADD, TT_ADDASSIGN, '=');
                break;
            case '-':
                token_singledoubledouble(source, &i, &tokens, TT_SUB, TT_SUBASSIGN, TT_ARROW, '=', '>');
                break;
            case '*':
                token_singledouble(source, &i, &tokens, TT_MUL, TT_MULASSIGN, '=');
                break;
            case '/':
                if (source[i + 1] == '/') {
                    while (source[i] && source[i] != '\n') {
                        i += 1;
                    }
                    if (source[i] == '\n') {
                        i += 1;
                    }
                    break;
                } else if (source[i + 1] == '*') {
                    int start = i;
                    i += 2;
                    bool closed = false;
                    while (source[i]) {
                        if (source[i] && source[i] == '*' && source[i + 1] && source[i + 1] == '/') {
                            closed = true;
                            i += 2;
                            break;
                        }
                        i += 1;
                    }
                    if (!closed) {
                        lf_error_print(file, source, start, start + 2, "unclosed '/*'");
                        array_delete(&tokens);
                        return NULL;
                    }
                    break;
                }
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

            case ':':
                array_push(&tokens, token_single(TT_COLON, i));
                i += 1;
                break;

            case '.':
                array_push(&tokens, token_single(TT_DOT, i));
                i += 1;
                break;
            case ',':
                array_push(&tokens, token_single(TT_COMMA, i));
                i += 1;
                break;

            default:
                if (source[i] >= '0' && source[i] <= '9') {
                    int start = i;
                    int dots = 0;
                    while (source[i] && ((source[i] >= '0' && source[i] <= '9') || source[i] == '.')) {
                        if (source[i] == '.') {
                            dots += 1;
                        }
                        i += 1;
                    }
                    if (dots > 1) {
                        lf_error_print(file, source, start, i, "malformed number");
                        array_delete(&tokens);
                        return NULL;
                    }
                    lfToken tok = (lfToken) {
                        .type = dots == 0 ? TT_INT : TT_FLOAT,
                        .value = malloc(i - start + 1),
                        .idx_start = start,
                        .idx_end = i
                    };
                    memcpy(tok.value, source + start, i - start);
                    tok.value[i - start] = 0;
                    array_push(&tokens, tok);
                } else if (
                    (source[i] >= 'A' && source[i] <= 'Z') ||
                    (source[i] >= 'a' && source[i] <= 'z') ||
                     source[i] == '_'
                ) {
                    int start = i;
                    while (
                          source[i] &&
                        ((source[i] >= 'A' && source[i] <= 'Z') ||
                         (source[i] >= 'a' && source[i] <= 'z') ||
                          source[i] == '_')
                    ) {
                        i += 1;
                    }
                    lfToken tok = (lfToken) {
                        .type = TT_IDENTIFIER,
                        .value = malloc(i - start + 1),
                        .idx_start = start,
                        .idx_end = i
                    };
                    memcpy(tok.value, source + start, i - start);
                    tok.value[i - start] = 0;
                    int j = 0;
                    while (keywords[j] != NULL) {
                        if (!strcmp(tok.value, keywords[j])) {
                            tok.type = TT_KEYWORD;
                        }
                        j += 1;
                    }
                    array_push(&tokens, tok);
                } else if (source[i] == '"' || source[i] == '\'') {
                    int start = i;
                    char opener = source[i];
                    i += 1;
                    lfArray(char) buffer = array_new(char);
                    while (source[i] && source[i] != '\n' && source[i] != opener) {
                        if (source[i] == '\\') {
                            switch (source[i + 1]) {
                                case 'a':
                                    array_push(&buffer, 0x7);
                                    break;
                                case 'b':
                                    array_push(&buffer, 0x8);
                                    break;
                                case 'f':
                                    array_push(&buffer, 0xc);
                                    break;
                                case 'n':
                                    array_push(&buffer, 0xa);
                                    break;
                                case 'r':
                                    array_push(&buffer, 0xd);
                                    break;
                                case 't':
                                    array_push(&buffer, 0x9);
                                    break;
                                case 'v':
                                    array_push(&buffer, 0xb);
                                    break;
                                case '\\':
                                    array_push(&buffer, '\\');
                                    break;
                                case '\'':
                                    array_push(&buffer, '\'');
                                    break;
                                case '"':
                                    array_push(&buffer, '"');
                                    break;
                                case 'x':
                                    if (!source[i + 2] || !source[i + 3]) {
                                        lf_error_print(file, source, i, i + 1, "incomplete hexadecimal escape");
                                        array_delete(&buffer);
                                        array_delete(&tokens);
                                        return NULL;
                                    }
                                    char tmp[3] = { source[i + 2], source[i + 3], 0 };
                                    char v = strtol(tmp, NULL, 16);
                                    array_push(&buffer, v);
                                    i += 2;
                                    break;
                                default:
                                    lf_error_print(file, source, i, i + 1, "unknown escape sequence");
                                    array_delete(&buffer);
                                    array_delete(&tokens);
                                    return NULL;
                            }
                            i += 2;
                        } else {
                            array_push(&buffer, source[i]);
                            i += 1;
                        }
                    }
                    if (source[i] != opener) {
                        lf_error_print(file, source, start, i, "unterminated string literal");
                        array_delete(&buffer);
                        array_delete(&tokens);
                        return NULL;
                    }
                    i += 1;
                    lfToken tok = (lfToken) {
                        .type = TT_STRING,
                        .value = buffer,
                        .idx_start = start,
                        .idx_end = i
                    };
                    array_push(&tokens, tok);
                }
        }
    }

    array_push(&tokens, token_single(TT_EOF, i - 1));

    deleter(&tokens) = NULL;
    return tokens;
}
