/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_PARSER_H
#define LEAF_PARSER_H

#include "parser/node.h"

lfNode *lf_parse(const char *source, const char *file) ;
void lf_node_delete(lfNode *node);
void lf_node_print(lfNode *node);

#endif /* LEAF_PARSER_H */
