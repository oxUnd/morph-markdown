#include "md_math_ext.h"

#include <chunk.h>
#include <node.h>

#include <stdint.h>
#include <string.h>

cmark_node_type MORPH_MD_NODE_MATH_INLINE;
cmark_node_type MORPH_MD_NODE_MATH_BLOCK;

struct math_delim {
	const char *open;
	const char *close;
	size_t open_len;
	size_t close_len;
	cmark_node_type type;
};

struct math_payload {
	char *literal;
	size_t len;
};

static void math_opaque_alloc(cmark_syntax_extension *extension,
			      cmark_mem *mem,
			      cmark_node *node)
{
	(void)extension;
	node->as.opaque = mem->calloc(1u, sizeof(struct math_payload));
}

static void math_opaque_free(cmark_syntax_extension *extension,
			     cmark_mem *mem,
			     cmark_node *node)
{
	struct math_payload *payload;

	(void)extension;
	payload = node->as.opaque;
	if (!payload)
		return;
	mem->free(payload->literal);
	mem->free(payload);
	node->as.opaque = NULL;
}

static int math_delim_at(const uint8_t *data, size_t len,
			 struct math_delim *delim)
{
	if (len >= 2u && data[0] == '$' && data[1] == '$') {
		delim->open = "$$";
		delim->close = "$$";
		delim->open_len = 2u;
		delim->close_len = 2u;
		delim->type = MORPH_MD_NODE_MATH_BLOCK;
		return 1;
	}
	if (len >= 2u && data[0] == '\\' && data[1] == '[') {
		delim->open = "\\[";
		delim->close = "\\]";
		delim->open_len = 2u;
		delim->close_len = 2u;
		delim->type = MORPH_MD_NODE_MATH_BLOCK;
		return 1;
	}
	if (len >= 2u && data[0] == '\\' && data[1] == '(') {
		delim->open = "\\(";
		delim->close = "\\)";
		delim->open_len = 2u;
		delim->close_len = 2u;
		delim->type = MORPH_MD_NODE_MATH_INLINE;
		return 1;
	}
	if (len >= 1u && data[0] == '$') {
		delim->open = "$";
		delim->close = "$";
		delim->open_len = 1u;
		delim->close_len = 1u;
		delim->type = MORPH_MD_NODE_MATH_INLINE;
		return 1;
	}
	return 0;
}

static size_t find_math_close(const uint8_t *data, size_t len,
			      const struct math_delim *delim)
{
	size_t i;

	for (i = delim->open_len; i + delim->close_len <= len; i++) {
		if (data[i] == '\\' && delim->close[0] == '$')
			continue;
		if (memcmp(data + i, delim->close, delim->close_len) == 0)
			return i;
	}
	return len;
}

static cmark_node *make_math_node(cmark_inline_parser *inline_parser,
				  cmark_syntax_extension *extension,
				  const struct math_delim *delim,
				  size_t close)
{
	cmark_chunk *chunk;
	cmark_node *node;
	struct math_payload *payload;
	cmark_mem *mem;
	int start_offset;
	int start_column;
	size_t literal_len;

	chunk = cmark_inline_parser_get_chunk(inline_parser);
	start_offset = cmark_inline_parser_get_offset(inline_parser);
	start_column = cmark_inline_parser_get_column(inline_parser);
	literal_len = close - delim->open_len;

	mem = cmark_get_default_mem_allocator();
	node = cmark_node_new_with_ext(delim->type, extension);
	if (!node)
		return NULL;
	payload = node->as.opaque;
	if (!payload) {
		cmark_node_free(node);
		return NULL;
	}
	payload->literal = mem->calloc(literal_len + 1u, 1u);
	if (!payload->literal) {
		cmark_node_free(node);
		return NULL;
	}
	memcpy(payload->literal,
	       chunk->data + (size_t)start_offset + delim->open_len,
	       literal_len);
	payload->literal[literal_len] = '\0';
	payload->len = literal_len;
	node->start_line = node->end_line =
		cmark_inline_parser_get_line(inline_parser);
	node->start_column = start_column;
	cmark_inline_parser_set_offset(
		inline_parser,
		start_offset + (int)close + (int)delim->close_len);
	node->end_column = cmark_inline_parser_get_column(inline_parser) - 1;
	return node;
}

const char *morph_md_math_literal(cmark_node *node)
{
	struct math_payload *payload;

	if (!node)
		return NULL;
	if (node->type != MORPH_MD_NODE_MATH_INLINE &&
	    node->type != MORPH_MD_NODE_MATH_BLOCK)
		return NULL;
	payload = node->as.opaque;
	return payload ? payload->literal : NULL;
}

static cmark_node *match_math(cmark_syntax_extension *extension,
			      cmark_parser *parser,
			      cmark_node *parent,
			      unsigned char character,
			      cmark_inline_parser *inline_parser)
{
	cmark_chunk *chunk;
	struct math_delim delim;
	size_t offset;
	size_t len;
	size_t close;
	const uint8_t *data;

	(void)parent;
	(void)character;

	chunk = cmark_inline_parser_get_chunk(inline_parser);
	offset = (size_t)cmark_inline_parser_get_offset(inline_parser);
	if (offset >= (size_t)chunk->len)
		return NULL;
	data = chunk->data + offset;
	len = (size_t)chunk->len - offset;
	if (!math_delim_at(data, len, &delim))
		return NULL;
	close = find_math_close(data, len, &delim);
	if (close >= len)
		return NULL;
	(void)parser;
	return make_math_node(inline_parser, extension, &delim, close);
}

static const char *math_type_string(cmark_syntax_extension *extension,
				    cmark_node *node)
{
	(void)extension;
	if (node->type == MORPH_MD_NODE_MATH_INLINE)
		return "math_inline";
	if (node->type == MORPH_MD_NODE_MATH_BLOCK)
		return "math_block";
	return "unknown";
}

static int math_can_contain(cmark_syntax_extension *extension,
			    cmark_node *node,
			    cmark_node_type child_type)
{
	(void)extension;
	(void)node;
	(void)child_type;
	return 0;
}

cmark_syntax_extension *morph_md_create_math_extension(void)
{
	static cmark_syntax_extension *singleton;
	cmark_syntax_extension *ext;
	cmark_llist *special_chars;
	cmark_mem *mem;

	if (singleton)
		return singleton;
	ext = cmark_syntax_extension_new("morph_math");
	if (!ext)
		return NULL;
	MORPH_MD_NODE_MATH_INLINE = cmark_syntax_extension_add_node(1);
	MORPH_MD_NODE_MATH_BLOCK = cmark_syntax_extension_add_node(1);
	cmark_syntax_extension_set_get_type_string_func(ext, math_type_string);
	cmark_syntax_extension_set_can_contain_func(ext, math_can_contain);
	cmark_syntax_extension_set_match_inline_func(ext, match_math);
	cmark_syntax_extension_set_opaque_alloc_func(ext, math_opaque_alloc);
	cmark_syntax_extension_set_opaque_free_func(ext, math_opaque_free);

	mem = cmark_get_default_mem_allocator();
	special_chars = NULL;
	special_chars = cmark_llist_append(mem, special_chars, (void *)'$');
	special_chars = cmark_llist_append(mem, special_chars, (void *)'\\');
	cmark_syntax_extension_set_special_inline_chars(ext, special_chars);
	singleton = ext;
	return singleton;
}
