#include "morph_markdown_stream.h"
#include "base/md_array.h"
#include "base/md_buf.h"
#include "base/md_error.h"
#include "base/md_hash.h"
#include "base/md_utf8.h"

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TAIL_BYTES 65536u

struct sealed_block {
	size_t offset;
	size_t len;
	uint64_t hash;
};

struct render_options {
	int enable_math;
	int html_policy;
};

struct morph_md_stream {
	struct morph_md_options options;
	morph_md_patch_cb callback;
	void *user_data;
	struct md_buf sealed;
	struct md_buf tail;
	struct md_buf utf8_pending;
	struct md_array sealed_blocks;
	char *pending_json;
	uint64_t next_id;
	int finished;
};

static void emit_patch(struct morph_md_stream *stream,
		       enum morph_md_patch_op op,
		       const char *json);

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

static int append_sourcepos(struct md_buf *out, cmark_node *node)
{
	if (cmark_node_get_start_line(node) <= 0)
		return 0;
	return md_buf_printf(out, ",\"sourcepos\":\"%d:%d-%d:%d\"",
			     cmark_node_get_start_line(node),
			     cmark_node_get_start_column(node),
			     cmark_node_get_end_line(node),
			     cmark_node_get_end_column(node));
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
	rc = append_sourcepos(out, node);
	if (rc != 0)
		return rc;

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

static int append_leaf_node_ex(struct md_buf *out, const char *kind,
			       const char *literal, size_t len,
			       cmark_node *source, uint64_t *next_id)
{
	int rc;

	rc = md_buf_printf(out, "{\"id\":%llu,\"kind\":",
			(unsigned long long)(*next_id)++);
	if (rc != 0)
		return rc;
	rc = json_escape(out, kind, strlen(kind));
	if (rc != 0)
		return rc;
	if (source) {
		rc = append_sourcepos(out, source);
		if (rc != 0)
			return rc;
	}
	rc = md_buf_puts(out, ",\"literal\":");
	if (rc != 0)
		return rc;
	rc = json_escape(out, literal, len);
	if (rc != 0)
		return rc;
	return md_buf_puts(out, ",\"children\":[]}");
}

static int append_leaf_node(struct md_buf *out, const char *kind,
			    const char *literal, size_t len,
			    uint64_t *next_id)
{
	return append_leaf_node_ex(out, kind, literal, len, NULL, next_id);
}

static size_t find_delim_close(const char *text, size_t len, size_t start,
			       const char *close, size_t close_len)
{
	size_t i;

	for (i = start; i + close_len <= len; i++) {
		if (text[i] == '\\' && close[0] == '$')
			continue;
		if (memcmp(text + i, close, close_len) == 0)
			return i;
	}
	return len;
}

static int math_open_at(const char *text, size_t len, size_t i,
			size_t *open_len, const char **close,
			size_t *close_len, const char **kind)
{
	if (i + 1u < len && text[i] == '$' && text[i + 1u] == '$') {
		*open_len = 2u;
		*close = "$$";
		*close_len = 2u;
		*kind = "math_block";
		return 1;
	}
	if (i + 1u < len && text[i] == '\\' && text[i + 1u] == '[') {
		*open_len = 2u;
		*close = "\\]";
		*close_len = 2u;
		*kind = "math_block";
		return 1;
	}
	if (i + 1u < len && text[i] == '\\' && text[i + 1u] == '(') {
		*open_len = 2u;
		*close = "\\)";
		*close_len = 2u;
		*kind = "math_inline";
		return 1;
	}
	if (text[i] == '$') {
		*open_len = 1u;
		*close = "$";
		*close_len = 1u;
		*kind = "math_inline";
		return 1;
	}
	return 0;
}

static size_t find_math_start(const char *text, size_t len, size_t start)
{
	size_t i;

	for (i = start; i < len; i++) {
		if (text[i] == '$' ||
		    (i + 1u < len && text[i] == '\\' &&
		     (text[i + 1u] == '(' || text[i + 1u] == '[')))
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
	size_t open_len;
	size_t close_len;
	const char *close_delim;
	const char *kind;
	int first;
	int rc;

	i = 0;
	first = 1;
	while (i < len) {
		plain_start = i;
		i = find_math_start(text, len, i);
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
		if (!math_open_at(text, len, i, &open_len, &close_delim,
				  &close_len, &kind)) {
			i++;
			continue;
		}
		close = find_delim_close(text, len, i + open_len, close_delim,
					 close_len);
		if (close >= len) {
			if (!first && md_buf_append(out, ",", 1u) != 0)
				return MD_ERR_NOMEM;
			return append_leaf_node(out, "text", text + i, len - i,
						next_id);
		}
		if (!first && md_buf_append(out, ",", 1u) != 0)
			return MD_ERR_NOMEM;
		first = 0;
		rc = append_leaf_node(out, kind, text + i + open_len,
				      close - i - open_len, next_id);
		if (rc != 0)
			return rc;
		i = close + close_len;
	}
	return 0;
}

static int append_json_children(struct md_buf *out, cmark_node *node,
				uint64_t *next_id,
				const struct render_options *opts);

static int should_skip_node(cmark_node *node, const struct render_options *opts)
{
	cmark_node_type type;

	type = cmark_node_get_type(node);
	return (type == CMARK_NODE_HTML_BLOCK || type == CMARK_NODE_HTML_INLINE) &&
	       opts->html_policy == MORPH_MD_HTML_STRIP;
}

static int append_json_node(struct md_buf *out, cmark_node *node,
			    uint64_t *next_id,
			    const struct render_options *opts)
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
		if (should_skip_node(child, opts))
			continue;
		if (!first && md_buf_append(out, ",", 1u) != 0)
			return MD_ERR_NOMEM;
		rc = append_json_children(out, child, next_id, opts);
		if (rc != 0)
			return rc;
		first = 0;
	}

	return md_buf_puts(out, "]}");
}

static int append_json_children(struct md_buf *out, cmark_node *node,
				uint64_t *next_id,
				const struct render_options *opts)
{
	cmark_node_type type;
	const char *literal;

	type = cmark_node_get_type(node);
	literal = cmark_node_get_literal(node);
	if ((type == CMARK_NODE_HTML_BLOCK || type == CMARK_NODE_HTML_INLINE) &&
	    opts->html_policy == MORPH_MD_HTML_STRIP)
		return MD_ERR_SKIP;
	if ((type == CMARK_NODE_HTML_BLOCK || type == CMARK_NODE_HTML_INLINE) &&
	    opts->html_policy == MORPH_MD_HTML_TEXT)
		return append_leaf_node_ex(out, "text", literal ? literal : "",
					   literal ? strlen(literal) : 0u,
					   node, next_id);
	if (opts->enable_math && type == CMARK_NODE_TEXT &&
	    literal && (strchr(literal, '$') || strstr(literal, "\\("))) {
		return append_math_text_nodes(out, literal, strlen(literal),
					      next_id);
	}

	return append_json_node(out, node, next_id, opts);
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
	options |= CMARK_OPT_SOURCEPOS;
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
		attach_extension(parser, "footnotes");
	}

	cmark_parser_feed(parser, md ? md : "", len);
	doc = cmark_parser_finish(parser);
	cmark_parser_free(parser);
	return doc;
}

static char *render_ir_json(const char *md, size_t len, int enable_gfm,
			    const struct render_options *opts,
			    uint64_t *next_id)
{
	struct md_buf out;
	cmark_node *doc;
	int rc;

	doc = parse_markdown(md, len, enable_gfm);
	if (!doc)
		return NULL;

	md_buf_init(&out);
	rc = append_json_node(&out, doc, next_id, opts);
	cmark_node_free(doc);
	if (rc != 0) {
		md_buf_cleanup(&out);
		return NULL;
	}

	return md_buf_detach(&out);
}

static void stream_render_options(const struct morph_md_stream *stream,
				  struct render_options *opts)
{
	opts->enable_math = stream->options.enable_math;
	opts->html_policy = stream->options.html_policy;
}

static char *render_node_json(cmark_node *node,
			      const struct render_options *opts,
			      uint64_t *next_id)
{
	struct md_buf out;
	int rc;

	md_buf_init(&out);
	rc = append_json_node(&out, node, next_id, opts);
	if (rc != 0) {
		md_buf_cleanup(&out);
		return NULL;
	}
	return md_buf_detach(&out);
}

static int emit_top_level_inserts(struct morph_md_stream *stream,
				  const char *md, size_t len,
				  uint64_t *next_id)
{
	struct render_options opts;
	cmark_node *doc;
	cmark_node *child;
	char *json;

	stream_render_options(stream, &opts);
	doc = parse_markdown(md, len, stream->options.enable_gfm);
	if (!doc)
		return MD_ERR_PARSE;

	for (child = cmark_node_first_child(doc); child;
	     child = cmark_node_next(child)) {
		if (should_skip_node(child, &opts))
			continue;
		json = render_node_json(child, &opts, next_id);
		if (!json) {
			cmark_node_free(doc);
			return MD_ERR_NOMEM;
		}
		emit_patch(stream, MORPH_MD_PATCH_INSERT, json);
		free(json);
	}

	cmark_node_free(doc);
	return MD_OK;
}

static void emit_patch(struct morph_md_stream *stream,
		       enum morph_md_patch_op op,
		       const char *json)
{
	if (stream->callback)
		stream->callback(op, json ? json : "{}", stream->user_data);
}

static int emit_seal_patch(struct morph_md_stream *stream,
			   const struct sealed_block *block)
{
	struct md_buf json;
	int rc;

	md_buf_init(&json);
	rc = md_buf_printf(&json,
			   "{\"offset\":%llu,\"len\":%llu,\"hash\":%llu}",
			   (unsigned long long)block->offset,
			   (unsigned long long)block->len,
			   (unsigned long long)block->hash);
	if (rc == 0)
		emit_patch(stream, MORPH_MD_PATCH_SEAL, json.data);
	md_buf_cleanup(&json);
	return rc;
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
	struct sealed_block *block;
	uint64_t next_id;
	int rc;

	if (prefix_len == 0)
		return 0;

	next_id = stream->next_id;
	rc = emit_top_level_inserts(stream, stream->tail.data, prefix_len,
				    &next_id);
	if (rc != MD_OK)
		return rc;

	block = md_array_push(&stream->sealed_blocks);
	if (!block) {
		return MD_ERR_NOMEM;
	}
	block->offset = stream->sealed.len;
	block->len = prefix_len;
	block->hash = md_hash_fnv1a(stream->tail.data, prefix_len);

	rc = md_buf_append(&stream->sealed, stream->tail.data, prefix_len);
	if (rc == 0)
		rc = shift_tail(stream, prefix_len);
	if (rc == 0) {
		stream->next_id = next_id;
		rc = emit_seal_patch(stream, block);
	}

	return rc;
}

static int emit_pending(struct morph_md_stream *stream)
{
	char *json;
	uint64_t next_id;
	struct render_options opts;

	if (stream->tail.len == 0) {
		free(stream->pending_json);
		stream->pending_json = NULL;
		return 0;
	}

	next_id = stream->next_id;
	stream_render_options(stream, &opts);
	json = render_ir_json(stream->tail.data, stream->tail.len,
			      stream->options.enable_gfm,
			      &opts, &next_id);
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

static int append_complete_utf8(struct morph_md_stream *stream,
				const char *bytes, size_t len,
				int is_final)
{
	struct md_buf input;
	size_t complete;
	int rc;

	md_buf_init(&input);
	rc = md_buf_append(&input, stream->utf8_pending.data,
			   stream->utf8_pending.len);
	if (rc == 0)
		rc = md_buf_append(&input, bytes, len);
	if (rc != 0) {
		md_buf_cleanup(&input);
		return rc;
	}

	stream->utf8_pending.len = 0;
	if (stream->utf8_pending.data)
		stream->utf8_pending.data[0] = '\0';

	complete = is_final ? input.len :
		   md_utf8_complete_prefix_len(input.data, input.len);
	rc = md_buf_append(&stream->tail, input.data, complete);
	if (rc == 0 && complete < input.len)
		rc = md_buf_append(&stream->utf8_pending, input.data + complete,
				   input.len - complete);

	md_buf_cleanup(&input);
	return rc;
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
	md_buf_init(&stream->utf8_pending);
	md_array_init(&stream->sealed_blocks, sizeof(struct sealed_block));
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

	rc = append_complete_utf8(stream, bytes, len, is_final);
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
	uint64_t next_id;
	struct render_options opts;

	if (!stream)
		return NULL;

	md_buf_init(&all);
	if (md_buf_append(&all, stream->sealed.data, stream->sealed.len) != 0)
		return NULL;
	if (md_buf_append(&all, stream->tail.data, stream->tail.len) != 0) {
		md_buf_cleanup(&all);
		return NULL;
	}

	next_id = 1u;
	stream_render_options(stream, &opts);
	json = render_ir_json(all.data, all.len, stream->options.enable_gfm,
			      &opts, &next_id);
	md_buf_cleanup(&all);
	return json;
}

int morph_md_stream_get_stats(struct morph_md_stream *stream,
			      struct morph_md_stats *stats)
{
	if (!stream || !stats)
		return MD_ERR_INVALID;

	stats->sealed_bytes = stream->sealed.len;
	stats->tail_bytes = stream->tail.len;
	stats->utf8_pending_bytes = stream->utf8_pending.len;
	stats->sealed_blocks = stream->sealed_blocks.len;
	stats->finished = stream->finished;
	return MD_OK;
}

void morph_md_stream_destroy(struct morph_md_stream *stream)
{
	if (!stream)
		return;

	md_buf_cleanup(&stream->sealed);
	md_buf_cleanup(&stream->tail);
	md_buf_cleanup(&stream->utf8_pending);
	md_array_cleanup(&stream->sealed_blocks);
	free(stream->pending_json);
	free(stream);
}

void morph_md_free(void *ptr)
{
	free(ptr);
}
