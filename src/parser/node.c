#include "parser/node.h"

void lf_type_deleter(lfType **pt) {
    lfType *t = *pt;
    if (t->type == VT_UNION || t->type == VT_INTERSECTION) {
        lfTypeOp *op = (lfTypeOp *)t;
        lf_type_deleter(&op->lhs);
        lf_type_deleter(&op->rhs);
        free(t);
    } else if (t->type == VT_ARRAY) {
        lfArrayType *arr = (lfArrayType *)t;
        array_delete(&arr->values);
        free(t);
    } else if (t->type == VT_MAP) {
        lfMapType *map = (lfMapType *)t;
        array_delete(&map->keys);
        array_delete(&map->values);
        free(t);
    } else if (t->type == VT_FUNC) {
        lfFuncType *f = (lfFuncType *)t;
        array_delete(&f->params);
        lf_type_deleter(&f->ret);
        free(t);
    } else if (t->type == VT_TYPENAME) {
        lfTypeName *tn = (lfTypeName *)t;
        lf_token_deleter(&tn->typename);
        free(t);
    } else if (t->type == VT_ANY) {
        free(t);
    }
}

void lf_node_deleter(lfNode **pnode) {
    lfNode *node = *pnode;
    switch (node->type) {
        case NT_IF: {
            lfIfNode *ifnode = (lfIfNode *)node;
            lf_node_deleter(&ifnode->condition);
            lf_node_deleter(&ifnode->body);
            if (ifnode->else_body) {
                lf_node_deleter(&ifnode->else_body);
            }
            free(node);
        } break;
        case NT_WHILE: {
            lfWhileNode *whilenode = (lfWhileNode *)node;
            lf_node_deleter(&whilenode->condition);
            lf_node_deleter(&whilenode->body);
            free(node);
        } break;
        case NT_UNARYOP: {
            lfUnaryOpNode *unop = (lfUnaryOpNode *)node;
            lf_token_deleter(&unop->op);
            lf_node_deleter(&unop->value);
            free(node);
        } break;
        case NT_BINARYOP: {
            lfBinaryOpNode *binop = (lfBinaryOpNode *)node;
            lf_token_deleter(&binop->op);
            lf_node_deleter(&binop->lhs);
            lf_node_deleter(&binop->rhs);
            free(node);
        } break;
        case NT_VARDECL: {
            lfVarDeclNode *decl = (lfVarDeclNode *)node;
            lf_token_deleter(&decl->name);
            if (decl->vartype) {
                lf_type_deleter(&decl->vartype);
            }
            if (decl->initializer)
                lf_node_deleter(&decl->initializer);
            free(node);
        } break;
        case NT_CALL: {
            lfCallNode *call = (lfCallNode *)node;
            lf_node_deleter(&call->func);
            array_delete(&call->args);
            free(node);
        } break;
        case NT_SUBSCRIBE: {
            lfSubscriptionNode *sub = (lfSubscriptionNode *)node;
            lf_node_deleter(&sub->object);
            lf_node_deleter(&sub->index);
            free(node);
        } break;
        case NT_ARRAY: {
            lfArrayNode *arr = (lfArrayNode *)node;
            array_delete(&arr->values);
            free(node);
        } break;
        case NT_MAP: {
            lfMapNode *arr = (lfMapNode *)node;
            array_delete(&arr->keys);
            array_delete(&arr->values);
            free(node);
        } break;
        case NT_VARACCESS: {
            lfVarAccessNode *var_access = (lfVarAccessNode *)node;
            lf_token_deleter(&var_access->var);
            free(node);
        } break;
        case NT_ASSIGN: {
            lfAssignNode *assign = (lfAssignNode *)node;
            lf_token_deleter(&assign->var);
            lf_node_deleter(&assign->value);
            free(node);
        } break;
        case NT_OBJASSIGN: {
            lfObjectAssignNode *assign = (lfObjectAssignNode *)node;
            lf_node_deleter(&assign->object);
            lf_node_deleter(&assign->key);
            lf_node_deleter(&assign->value);
            free(node);
        } break;
        case NT_FUNC: {
            lfFunctionNode *f = (lfFunctionNode *)node;
            lf_token_deleter(&f->name);
            array_delete(&f->params);
            array_delete(&f->body);
            array_delete(&f->type_names);
            array_delete(&f->types);
            if (f->return_type) {
                lf_type_deleter(&f->return_type);
            }
            free(node);
        } break;
        case NT_CLASS: {
            lfClassNode *cls = (lfClassNode *)node;
            lf_token_deleter(&cls->name);
            array_delete(&cls->body);
            free(node);
        } break;
        case NT_RETURN: {
            lfReturnNode *ret = (lfReturnNode *)node;
            if (ret->value) {
                lf_node_deleter(&ret->value);
            }
            free(node);
        } break;
        case NT_COMPOUND: {
            lfCompoundNode *comp = (lfCompoundNode *)node;
            array_delete(&comp->statements);
            free(node);
        } break;
        case NT_IMPORT: {
            lfImportNode *import = (lfImportNode *)node;
            array_delete(&import->path);
            free(node);
        } break;
        case NT_INT:
        case NT_FLOAT:
        case NT_STRING: {
            lfLiteralNode *lit = (lfLiteralNode *)node;
            lf_token_deleter(&lit->value);
            free(node);
        } break;
    }
}
