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
    NT_COMPOUND,
    NT_IMPORT
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

#define LF_NODE_HEADER \
    lfNodeType type; \
    size_t lineno;

typedef struct lfNode {
    LF_NODE_HEADER;
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
    LF_NODE_HEADER;
    lfToken value;
} lfLiteralNode;

typedef struct lfUnaryOpNode {
    LF_NODE_HEADER;
    lfToken op;
    lfNode *value;
} lfUnaryOpNode;

typedef struct lfBinaryOpNode {
    LF_NODE_HEADER;
    lfToken op;
    lfNode *lhs;
    lfNode *rhs;
} lfBinaryOpNode;

typedef struct lfVarAccessNode {
    LF_NODE_HEADER;
    lfToken var;
} lfVarAccessNode;

typedef struct lfSubscriptionNode {
    LF_NODE_HEADER;
    lfNode *object;
    lfNode *index;
} lfSubscriptionNode;

typedef struct lfCallNode {
    LF_NODE_HEADER;
    lfNode *func;
    lfArray(lfNode *) args;
} lfCallNode;

typedef struct lfArrayNode {
    LF_NODE_HEADER;
    lfArray(lfNode *) values;
} lfArrayNode;

typedef struct lfMapNode {
    LF_NODE_HEADER;
    lfArray(lfNode *) keys;
    lfArray(lfNode *) values;
} lfMapNode;

typedef struct lfAssignNode {
    LF_NODE_HEADER;
    lfToken var;
    lfNode *value;
} lfAssignNode;

typedef struct lfObjectAssignNode {
    LF_NODE_HEADER;
    lfNode *object;
    lfNode *key;
    lfNode *value;
} lfObjectAssignNode;

/* statements */

typedef struct lfVarDeclNode {
    LF_NODE_HEADER;
    lfToken name;
    lfNode *initializer;
    bool is_const;
    bool is_ref; /* for functions */
    lfType *vartype;
} lfVarDeclNode;

typedef struct lfIfNode {
    LF_NODE_HEADER;
    lfNode *condition;
    lfNode *body;
    lfNode *else_body;
} lfIfNode;

typedef struct lfWhileNode {
    LF_NODE_HEADER;
    lfNode *condition;
    lfNode *body;
} lfWhileNode;

typedef struct lfFunctionNode {
    LF_NODE_HEADER;
    lfToken name;
    lfArray(lfVarDeclNode *) params;
    lfArray(lfNode *) body;
    lfType *return_type;
    /* for generic typing */
    lfArray(lfToken) type_names;
    lfArray(lfType *) types;
} lfFunctionNode;

typedef struct lfReturnNode {
    LF_NODE_HEADER;
    lfNode *value;
} lfReturnNode;

typedef struct lfCompoundNode {
    LF_NODE_HEADER;
    lfArray(lfNode *) statements;
} lfCompoundNode;

typedef struct lfClassNode {
    LF_NODE_HEADER;
    lfToken name;
    lfArray(lfNode *) body;
} lfClassNode;

typedef struct lfImportNode {
    LF_NODE_HEADER;
    lfArray(lfToken) path;
} lfImportNode;

void lf_node_deleter(lfNode **node);
void lf_type_deleter(lfType **t);

#endif /* LEAF_NODE_H */
