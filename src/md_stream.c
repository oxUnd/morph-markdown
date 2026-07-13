#include "morph_markdown_stream.h"
#include "base/md_buf.h"
#include "base/md_error.h"

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TAIL_BYTES 65536u

struct morph_md_stream {
	struct morph_md_options options;
	morph_md_patch_cb callback;
	void *user_data;
	struct md_buf sealed;
	struct md_buf tail;
	char *pending_json;
	uint64_t next_id;
	int finished;
};

static int json_escape(struct md_buf *out, const char *text, size_t len)
{
	size_t i;
	unsigned char c;
	int rc;

	rc = md_buf_append(out, "\"", 1u);
	if (rc != 0)
		return rc;

	for (i = 0; i < len; i++) {
		c = (unsigned char)text[i];
		if (c == '"' || c == '\\') {
			rc = md_buf_append(out, "\\", 1u);
			if (rc == 0)
				rc = md_buf_append(out, (const char *)&c, 1u);
		} else if (c == '\n') {
			rc = md_buf_append(out, "\\n", 2u);
		} else if (c == '\r') {
			rc = md_buf_append(out, "\\r", 2u);
		} else if (c == '\t') {
			rc = md_buf_append(out, "\\t", 2u);
		} else if (c < 0x20u) {
			rc = md_buf_printf(out, "\\u%04x", (unsigned)c);
		} else {
			rc = md_buf_append(out, (const char *)&c, 1u);
		}
		if (rc != 0)
			return rc;
	}

	return md_buf_append(out, "\"", 1u);
}

static const char *node_kind(cmark_node *node)
{
	const char *kind;

	kind = cmark_node_get_type_string(node);
	if (!kind)
		return "unknown";
	if (strcmp(kind, "code_block") == 0)
		return "code_block";
	if (strcmp(kind, "softbreak") == 0)
		return "soft_break";
	if (strcmp(kind, "linebreak") == 0)
		return "hard_break";
	return kind;
}

static int append_attr_string(struct md_buf *out, const char *name,
			      const char *value)
{
	int rc;

	if (!value)
		return 0;

	rc = md_buf_printf(out, ",\"%s\":", name);
	if (rc != 0)
		return rc;
	return json_escape(out, value, strlen(value));
}

static int append_node_attrs(struct md_buf *out, cmark_node *node)
{
	cmark_node_type type;
	const char *literal;
	const char *url;
	const char *title;
	const char *info;
	int rc;

	type = cmark_node_get_type(node);
	if (type == CMARK_NODE_HEADING) {
		rc = md_buf_printf(out, ",\"level\":%d", cmark_node_get_heading_level(node));
		if (rc != 0)
			return rc;
	}

	literal = cmark_node_get_literal(node);
	rc = append_attr_string(out, "literal", literal);
	if (rc != 0)
		return rc;

	url = cmark_node_get_url(node);
	rc = append_attr_string(out, "url", url);
	if (rc != 0)
		return rc;

	title = cmark_node_get_title(node);
	rc = append_attr_string(out, "title", title);
	if (rc != 0)
		return rc;

	info = cmark_node_get_fence_info(node);
	return append_attr_string(out, "info", info);
}

static int append_leaf_node(struct md_buf *out, const char *kind,
			    const char *literal, size_t len,
			    uint64_t *next_id)
{
	int rc;

	rc = md_buf_printf(out, "{\"id\":%llu,\"kind\":",
			(unsigned long long)(*next_id)++);
	if (rc != 0)
		return rc;
	rc = json_escape(out, kind, strlen(kind));
	if (rc != 0)
		return rc;
	rc = md_buf_puts(out, ",\"literal\":");
	if (rc != 0)
		return rc;
	rc = json_escape(out, literal, len);
	if (rc != 0)
		return rc;
	return md_buf_puts(out, ",\"children\":[]}");
}

static size_t find_math_close(const char *text, size_t len, size_t start)
{
	size_t i;

	if (start + 1u < len && text[start] == '\\' && text[start + 1u] == '(') {
		for (i = start + 2u; i + 1u < len; i++) {
			if (text[i] == '\\' && text[i + 1u] == ')')
				return i;
		}
		return len;
	}

	for (i = start + 1u; i < len; i++) {
		if (text[i] == '$' && text[i - 1u] != '\\')
			return i;
	}
	return len;
}

static int append_math_text_nodes(struct md_buf *out, const char *text,
				  size_t len, uint64_t *next_id)
{
	size_t i;
	size_t plain_start;
	size_t close;
	int first;
	int rc;

	i = 0;
	first = 1;
	while (i < len) {
		plain_start = i;
		while (i < len && text[i] != '$' &&
		       !(i + 1u < len && text[i] == '\\' && text[i + 1u] == '('))
			i++;
		if (i > plain_start) {
			if (!first && md_buf_append(out, ",", 1u) != 0)
				return MD_ERR_NOMEM;
			first = 0;
			rc = append_leaf_node(out, "text", text + plain_start,
					      i - plain_start, next_id);
			if (rc != 0)
				return rc;
		}
		if (i >= len)
			break;
		close = find_math_close(text, len, i);
		if (close >= len) {
			if (!first && md_buf_append(out, ",", 1u) != 0)
				return MD_ERR_NOMEM;
			return append_leaf_node(out, "text", text + i, len - i,
						next_id);
		}
		if (!first && md_buf_append(out, ",", 1u) != 0)
			return MD_ERR_NOMEM;
		first = 0;
		if (text[i] == '$')
			rc = append_leaf_node(out, "math_inline", text + i + 1u,
					      close - i - 1u, next_id);
		else
			rc = append_leaf_node(out, "math_inline", text + i + 2u,
					      close - i - 2u, next_id);
		if (rc != 0)
			return rc;
		i = text[i] == '$' ? close + 1u : close + 2u;
	}
	return 0;
}

static int append_json_children(struct md_buf *out, cmark_node *node,
				uint64_t *next_id, int enable_math);

static int append_json_node(struct md_buf *out, cmark_node *node,
			    uint64_t *next_id, int enable_math)
{
	cmark_node *child;
	int first;
	int rc;

	rc = md_buf_printf(out, "{\"id\":%llu,\"kind\":",
			(unsigned long long)(*next_id)++);
	if (rc != 0)
		return rc;

	rc = json_escape(out, node_kind(node), strlen(node_kind(node)));
	if (rc != 0)
		return rc;

	rc = append_node_attrs(out, node);
	if (rc != 0)
		return rc;

	rc = md_buf_puts(out, ",\"children\":[");
	if (rc != 0)
		return rc;

	first = 1;
	for (child = cmark_node_first_child(node); child; child = cmark_node_next(child)) {
		if (!first) {
			rc = md_buf_append(out, ",", 1u);
			if (rc != 0)
				return rc;
		}
		first = 0;
		rc = append_json_children(out, child, next_id, enable_math);
		if (rc != 0)
			return rc;
	}

	return md_buf_puts(out, "]}");
}

static int append_json_children(struct md_buf *out, cmark_node *node,
				uint64_t *next_id, int enable_math)
{
	const char *literal;

	literal = cmark_node_get_literal(node);
	if (enable_math && cmark_node_get_type(node) == CMARK_NODE_TEXT &&
	    literal && (strchr(literal, '$') || strstr(literal, "\\("))) {
		return append_math_text_nodes(out, literal, strlen(literal),
					      next_id);
	}

	return append_json_node(out, node, next_id, enable_math);
}

static void attach_extension(cmark_parser *parser, const char *name)
{
	cmark_syntax_extension *extension;

	extension = cmark_find_syntax_extension(name);
	if (extension)
		cmark_parser_attach_syntax_extension(parser, extension);
}

static cmark_node *parse_markdown(const char *md, size_t len, int enable_gfm)
{
	cmark_parser *parser;
	cmark_node *doc;
	int options;

	options = CMARK_OPT_DEFAULT | CMARK_OPT_UNSAFE;
	cmark_gfm_core_extensions_ensure_registered();
	parser = cmark_parser_new(options);
	if (!parser)
		return NULL;

	if (enable_gfm) {
		attach_extension(parser, "table");
		attach_extension(parser, "strikethrough");
		attach_extension(parser, "autolink");
		attach_extension(parser, "tagfilter");
		attach_extension(parser, "tasklist");
	}

	cmark_parser_feed(parser, md ? md : "", len);
	doc = cmark_parser_finish(parser);
	cmark_parser_free(parser);
	return doc;
}

static char *render_ir_json(const char *md, size_t len, int enable_gfm,
			    int enable_math, uint64_t first_id)
{
	struct md_buf out;
	cmark_node *doc;
	uint64_t next_id;
	int rc;

	doc = parse_markdown(md, len, enable_gfm);
	if (!doc)
		return NULL;

	md_buf_init(&out);
	next_id = first_id;
	rc = append_json_node(&out, doc, &next_id, enable_math);
	cmark_node_free(doc);
	if (rc != 0) {
		md_buf_cleanup(&out);
		return NULL;
	}

	return md_buf_detach(&out);
}

static void emit_patch(struct morph_md_stream *stream,
		       enum morph_md_patch_op op,
		       const char *json)
{
	if (stream->callback)
		stream->callback(op, json ? json : "{}", stream->user_data);
}

static int is_blank_line(const char *line, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		if (line[i] != ' ' && line[i] != '\t' &&
		    line[i] != '\r' && line[i] != '\n')
			return 0;
	}
	return 1;
}

static int fence_marker(const char *line, size_t len, char *marker)
{
	size_t i;
	size_t indent;
	size_t count;
	char ch;

	i = 0;
	indent = 0;
	while (i < len && indent < 4u && line[i] == ' ') {
		i++;
		indent++;
	}
	if (i >= len || (line[i] != '`' && line[i] != '~'))
		return 0;

	ch = line[i];
	count = 0;
	while (i < len && line[i] == ch) {
		i++;
		count++;
	}
	if (count < 3u)
		return 0;

	*marker = ch;
	return 1;
}

static size_t stable_prefix_len(const char *text, size_t len, int is_final)
{
	size_t start;
	size_t end;
	size_t safe;
	int in_fence;
	char fence_ch;
	char marker;

	if (is_final)
		return len;

	start = 0;
	safe = 0;
	in_fence = 0;
	fence_ch = '\0';

	while (start < len) {
		end = start;
		while (end < len && text[end] != '\n')
			end++;
		if (end == len)
			break;
		end++;

		if (fence_marker(text + start, end - start, &marker)) {
			if (!in_fence) {
				in_fence = 1;
				fence_ch = marker;
			} else if (marker == fence_ch) {
				in_fence = 0;
				fence_ch = '\0';
			}
		}

		if (!in_fence && is_blank_line(text + start, end - start))
			safe = end;
		start = end;
	}

	return safe;
}

static int shift_tail(struct morph_md_stream *stream, size_t prefix_len)
{
	size_t rest;

	if (prefix_len == 0)
		return 0;
	if (prefix_len > stream->tail.len)
		return MD_ERR_INVALID;

	rest = stream->tail.len - prefix_len;
	memmove(stream->tail.data, stream->tail.data + prefix_len, rest);
	stream->tail.len = rest;
	if (stream->tail.data)
		stream->tail.data[rest] = '\0';
	return 0;
}

static int commit_prefix(struct morph_md_stream *stream, size_t prefix_len)
{
	char *json;
	int rc;

	if (prefix_len == 0)
		return 0;

	json = render_ir_json(stream->tail.data, prefix_len,
			      stream->options.enable_gfm,
			      stream->options.enable_math, stream->next_id);
	if (!json)
		return MD_ERR_NOMEM;

	rc = md_buf_append(&stream->sealed, stream->tail.data, prefix_len);
	if (rc == 0)
		rc = shift_tail(stream, prefix_len);
	if (rc == 0)
		emit_patch(stream, MORPH_MD_PATCH_INSERT, json);

	free(json);
	return rc;
}

static int emit_pending(struct morph_md_stream *stream)
{
	char *json;

	if (stream->tail.len == 0) {
		free(stream->pending_json);
		stream->pending_json = NULL;
		return 0;
	}

	json = render_ir_json(stream->tail.data, stream->tail.len,
			      stream->options.enable_gfm,
			      stream->options.enable_math, 1u);
	if (!json)
		return MD_ERR_NOMEM;

	if (!stream->pending_json || strcmp(stream->pending_json, json) != 0) {
		emit_patch(stream, MORPH_MD_PATCH_UPDATE, json);
		free(stream->pending_json);
		stream->pending_json = json;
		return 0;
	}

	free(json);
	return 0;
}

static int process_stream(struct morph_md_stream *stream, int is_final)
{
	size_t prefix_len;
	int rc;

	prefix_len = stable_prefix_len(stream->tail.data, stream->tail.len,
				       is_final);
	rc = commit_prefix(stream, prefix_len);
	if (rc != 0)
		return rc;

	rc = emit_pending(stream);
	if (rc != 0)
		return rc;

	if (is_final && !stream->finished) {
		stream->finished = 1;
		emit_patch(stream, MORPH_MD_PATCH_FINISH, "{}");
	}
	return 0;
}

struct morph_md_stream *morph_md_stream_create(
	const struct morph_md_options *options,
	morph_md_patch_cb callback,
	void *user_data)
{
	struct morph_md_stream *stream;

	stream = calloc(1u, sizeof(*stream));
	if (!stream)
		return NULL;

	if (options)
		stream->options = *options;
	if (stream->options.max_tail_bytes == 0)
		stream->options.max_tail_bytes = DEFAULT_TAIL_BYTES;
	if (!options)
		stream->options.enable_gfm = 1;

	stream->callback = callback;
	stream->user_data = user_data;
	stream->next_id = 1u;
	md_buf_init(&stream->sealed);
	md_buf_init(&stream->tail);
	return stream;
}

int morph_md_stream_append(struct morph_md_stream *stream,
			   const char *bytes,
			   size_t len,
			   int is_final)
{
	int rc;

	if (!stream || (!bytes && len > 0))
		return MD_ERR_INVALID;
	if (stream->finished)
		return MD_ERR_INVALID;

	rc = md_buf_append(&stream->tail, bytes, len);
	if (rc != 0)
		return rc;

	if (stream->tail.len > stream->options.max_tail_bytes && !is_final)
		is_final = 1;

	rc = process_stream(stream, is_final);
	if (rc != 0)
		emit_patch(stream, MORPH_MD_PATCH_ERROR, "{\"error\":\"parse\"}");
	return rc;
}

char *morph_md_stream_snapshot(struct morph_md_stream *stream)
{
	struct md_buf all;
	char *json;

	if (!stream)
		return NULL;

	md_buf_init(&all);
	if (md_buf_append(&all, stream->sealed.data, stream->sealed.len) != 0)
		return NULL;
	if (md_buf_append(&all, stream->tail.data, stream->tail.len) != 0) {
		md_buf_cleanup(&all);
		return NULL;
	}

	json = render_ir_json(all.data, all.len, stream->options.enable_gfm,
			      stream->options.enable_math, 1u);
	md_buf_cleanup(&all);
	return json;
}

void morph_md_stream_destroy(struct morph_md_stream *stream)
{
	if (!stream)
		return;

	md_buf_cleanup(&stream->sealed);
	md_buf_cleanup(&stream->tail);
	free(stream->pending_json);
	free(stream);
}

void morph_md_free(void *ptr)
{
	free(ptr);
}
