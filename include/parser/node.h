/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_NODE_H
#define LEAF_NODE_H

#include "parser/token.h"

typedef enum lfNodeType {
    /* literals */
    NT_INT,
    NT_FLOAT,
    NT_STRING,

    /* operations */
    NT_UNARYOP,
    NT_BINARYOP,

} lfNodeType;

typedef struct lfNode {
    lfNodeType type;
} lfNode;

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

#endif /* LEAF_NODE_H */
