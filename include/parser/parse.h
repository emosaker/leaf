/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_PARSER_H
#define LEAF_PARSER_H

#include "parser/node.h"
#include "parser/token.h"
#include "lib/array.h"

lfNode *lf_parse(const lfArray(lfToken) tokens, const char *source, const char *file) ;
void lf_node_delete(lfNode *node);
void lf_node_print(lfNode *node);

#endif /* LEAF_PARSER_H */
