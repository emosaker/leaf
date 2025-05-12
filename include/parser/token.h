/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_TOKEN_H
#define LEAF_TOKEN_H

#include <stdint.h>
#include <stdlib.h>

typedef enum lfTokenType {
    TT_EOF,

    /* identifiers */
    TT_IDENTIFIER,
    TT_KEYWORD,
    TT_INT,
    TT_FLOAT,
    TT_STRING,

    /* operators */
    TT_ADD,
    TT_SUB,
    TT_MUL,
    TT_DIV,
    TT_POW,

    TT_AND,
    TT_OR,
    TT_NOT,

    /* bitwise */
    TT_LSHIFT,
    TT_RSHIFT,
    TT_BAND,
    TT_BOR,
    TT_BXOR,

    /* comparative */
    TT_EQ,
    TT_NE,
    TT_LT,
    TT_GT,
    TT_LE,
    TT_GE,

    /* assignative */
    TT_ASSIGN,
    TT_ADDASSIGN,
    TT_SUBASSIGN,
    TT_MULASSIGN,
    TT_DIVASSIGN,

    TT_COLON,

    /* braces */
    TT_LPAREN,
    TT_RPAREN,
    TT_LBRACE,
    TT_RBRACE,
    TT_LBRACKET,
    TT_RBRACKET,

    /* misc */
    TT_DOT,
    TT_COMMA
} lfTokenType;

typedef struct lfToken {
    lfTokenType type;
    char *value;
    size_t idx_start;
    size_t idx_end;
} lfToken;

#endif /* LEAF_TOKEN_H */
