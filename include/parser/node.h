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
    NT_MAP,

    /* operations */
    NT_UNARYOP,
    NT_BINARYOP,

    /* variables */
    NT_VARACCESS,
    NT_VARDECL,
    NT_SUBSCRIBE,
    NT_ASSIGN,    /* foo = "bar"     */
    NT_OBJASSIGN, /* foo.bar = "baz" */

    /* functional */
    NT_CALL,
    NT_FUNC,

    /* control flow */
    NT_IF,
    NT_WHILE,
    NT_RETURN,

    /* types */
    NT_CLASS,

    /* misc */
    NT_COMPOUND
} lfNodeType;

/* typing */

typedef enum lfTypeType {
    VT_TYPENAME,
    VT_UNION,
    VT_INTERSECTION,
    VT_FUNC,
    VT_ARRAY,
    VT_MAP,
    VT_ANY /* for empty generics, like fn<T> ... where T is any */
} lfTypeType;

typedef struct lfNode {
    lfNodeType type;
} lfNode;

typedef struct lfType {
    lfTypeType type;
} lfType;

typedef struct lfTypeOp {
    lfTypeType type;
    lfType *lhs;
    lfType *rhs;
} lfTypeOp;

typedef struct lfTypeName {
    lfTypeType type;
    lfToken typename;
} lfTypeName;

typedef struct lfMapType {
    lfTypeType type;
    lfArray(lfType *) keys;
    lfArray(lfType *) values;
} lfMapType;

typedef struct lfArrayType {
    lfTypeType type;
    lfArray(lfType *) values;
} lfArrayType;

typedef struct lfFuncType {
    lfTypeType type;
    lfArray(lfType *) params;
    lfType *ret;
} lfFuncType;

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

typedef struct lfMapNode {
    lfNodeType type;
    lfArray(lfNode *) keys;
    lfArray(lfNode *) values;
} lfMapNode;

typedef struct lfAssignNode {
    lfNodeType type;
    lfToken variable;
    lfNode *value;
} lfAssignNode;

typedef struct lfObjectAssignNode {
    lfNodeType type;
    lfNode *object;
    lfNode *key;
    lfNode *value;
} lfObjectAssignNode;

/* statements */

typedef struct lfVarDeclNode {
    lfNodeType type;
    lfToken name;
    lfNode *initializer;
    bool is_const;
    bool is_ref; /* for functions */
    lfType *vartype;
} lfVarDeclNode;

typedef struct lfIfNode {
    lfNodeType type;
    lfNode *condition;
    lfNode *body;
    lfNode *else_body;
} lfIfNode;

typedef struct lfWhileNode {
    lfNodeType type;
    lfNode *condition;
    lfNode *body;
} lfWhileNode;

typedef struct lfFunctionNode {
    lfNodeType type;
    lfToken name;
    lfArray(lfVarDeclNode *) params;
    lfArray(lfNode *) body;
    lfType *return_type;
    /* for generic typing */
    lfArray(lfToken) type_names;
    lfArray(lfType *) types;
} lfFunctionNode;

typedef struct lfReturnNode {
    lfNodeType type;
    lfNode *value;
} lfReturnNode;

typedef struct lfCompoundNode {
    lfNodeType type;
    lfArray(lfNode *) statements;
} lfCompoundNode;

typedef struct lfClassNode {
    lfNodeType type;
    lfToken name;
    lfArray(lfNode *) body;
} lfClassNode;

#endif /* LEAF_NODE_H */
