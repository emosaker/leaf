/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_NODE_H
#define LEAF_NODE_H

#include <stdbool.h>

#include "parser/token.h"
#include "lib/array.h"

typedef enum lfNodeType {
    /* literals */
    NT_INT,
    NT_FLOAT,
    NT_STRING,
    NT_ARRAY,

    /* operations */
    NT_UNARYOP,
    NT_BINARYOP,

    /* variables */
    NT_VARACCESS,
    NT_VARDECL,
    NT_SUBSCRIBE,

    /* functional */
    NT_CALL


} lfNodeType;

typedef struct lfNode {
    lfNodeType type;
} lfNode;

typedef struct lfType {
    lfToken typename;
} lfType;

/* nodes */

typedef struct lfLiteralNode {
    lfNodeType type;
    lfToken value;
} lfLiteralNode;

typedef struct lfUnaryOpNode {
    lfNodeType type;
    lfToken op;
    lfNode *value;
} lfUnaryOpNode;

typedef struct lfBinaryOpNode {
    lfNodeType type;
    lfToken op;
    lfNode *lhs;
    lfNode *rhs;
} lfBinaryOpNode;

typedef struct lfVarAccessNode {
    lfNodeType type;
    lfToken var;
} lfVarAccessNode;

typedef struct lfSubscriptionNode {
    lfNodeType type;
    lfNode *object;
    lfNode *index;
} lfSubscriptionNode;

typedef struct lfCallNode {
    lfNodeType type;
    lfNode *func;
    lfArray(lfNode *) args;
} lfCallNode;

typedef struct lfArrayNode {
    lfNodeType type;
    lfArray(lfNode *) values;
} lfArrayNode;

/* statements */

typedef struct lfVarDeclNode {
    lfNodeType type;
    lfToken name;
    lfNode *initializer;
    bool is_const;
    bool is_ref; /* for functions */
    bool is_typed;
    lfType vartype;
} lfVarDeclNode;

#endif /* LEAF_NODE_H */
