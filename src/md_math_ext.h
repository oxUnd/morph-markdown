#ifndef MORPH_MD_MATH_EXT_H
#define MORPH_MD_MATH_EXT_H

#include <cmark-gfm-extension_api.h>

extern cmark_node_type MORPH_MD_NODE_MATH_INLINE;
extern cmark_node_type MORPH_MD_NODE_MATH_BLOCK;

cmark_syntax_extension *morph_md_create_math_extension(void);
const char *morph_md_math_literal(cmark_node *node);

#endif
