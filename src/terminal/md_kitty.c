#include "morph_markdown_kitty.h"
#include "base/md_buf.h"
#include "base/md_error.h"

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>
#include <mathjax.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct morph_md_kitty {
	struct morph_md_kitty_options options;
	struct md_buf markdown;
	mjx_ctx *math;
	int list_depth;
};

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

	cmark_gfm_core_extensions_ensure_registered();
	parser = cmark_parser_new(CMARK_OPT_DEFAULT);
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

static int is_math_start(const char *text, size_t len, size_t i,
			 size_t *open_len, const char **close,
			 size_t *close_len, mjx_style *style)
{
	if (i + 1u < len && text[i] == '$' && text[i + 1u] == '$') {
		*open_len = 2u;
		*close = "$$";
		*close_len = 2u;
		*style = MJX_STYLE_DISPLAY;
		return 1;
	}
	if (i + 1u < len && text[i] == '\\' && text[i + 1u] == '[') {
		*open_len = 2u;
		*close = "\\]";
		*close_len = 2u;
		*style = MJX_STYLE_DISPLAY;
		return 1;
	}
	if (i + 1u < len && text[i] == '\\' && text[i + 1u] == '(') {
		*open_len = 2u;
		*close = "\\)";
		*close_len = 2u;
		*style = MJX_STYLE_INLINE;
		return 1;
	}
	if (text[i] == '$') {
		*open_len = 1u;
		*close = "$";
		*close_len = 1u;
		*style = MJX_STYLE_INLINE;
		return 1;
	}
	return 0;
}

static size_t find_close(const char *text, size_t len, size_t start,
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

static int render_formula(struct morph_md_kitty *renderer,
			  const char *latex,
			  size_t len,
			  mjx_style style)
{
	char *expr;
	mjx_buf *buf;
	mjx_kitty_opts opts;

	expr = malloc(len + 1u);
	if (!expr)
		return MD_ERR_NOMEM;
	memcpy(expr, latex, len);
	expr[len] = '\0';

	buf = mjx_render_latex(renderer->math, expr, style);
	free(expr);
	if (!buf)
		return MD_ERR_PARSE;

	memset(&opts, 0, sizeof(opts));
	opts.placement = MJX_KITTY_CURSOR;
	opts.width = mjx_buf_width(buf);
	opts.height = mjx_buf_height(buf);
	(void)mjx_display_kitty(buf, &opts);
	mjx_buf_free(buf);
	return MD_OK;
}

static int render_text_with_math(struct morph_md_kitty *renderer,
				 const char *text)
{
	size_t len;
	size_t i;
	size_t open_len;
	size_t close_len;
	size_t close_pos;
	const char *close;
	mjx_style style;

	len = strlen(text);
	i = 0;
	while (i < len) {
		if (!renderer->options.enable_math ||
		    !is_math_start(text, len, i, &open_len, &close,
				   &close_len, &style)) {
			fputc(text[i], stdout);
			i++;
			continue;
		}
		close_pos = find_close(text, len, i + open_len, close, close_len);
		if (close_pos >= len) {
			fputc(text[i], stdout);
			i++;
			continue;
		}
		if (style == MJX_STYLE_DISPLAY)
			fputc('\n', stdout);
		(void)render_formula(renderer, text + i + open_len,
				      close_pos - i - open_len, style);
		if (style == MJX_STYLE_DISPLAY)
			fputc('\n', stdout);
		i = close_pos + close_len;
	}
	return MD_OK;
}

static int render_node(struct morph_md_kitty *renderer, cmark_node *node);

static int append_plain_text(struct md_buf *out, cmark_node *node)
{
	cmark_node *child;
	const char *literal;
	int rc;

	literal = cmark_node_get_literal(node);
	if (literal) {
		rc = md_buf_puts(out, literal);
		if (rc != MD_OK)
			return rc;
	}
	for (child = cmark_node_first_child(node); child;
	     child = cmark_node_next(child)) {
		rc = append_plain_text(out, child);
		if (rc != MD_OK)
			return rc;
	}
	return MD_OK;
}

static char *plain_text_dup(cmark_node *node)
{
	struct md_buf out;

	md_buf_init(&out);
	if (append_plain_text(&out, node) != MD_OK) {
		md_buf_cleanup(&out);
		return NULL;
	}
	return md_buf_detach(&out);
}

static int render_children(struct morph_md_kitty *renderer, cmark_node *node)
{
	cmark_node *child;
	int rc;

	for (child = cmark_node_first_child(node); child;
	     child = cmark_node_next(child)) {
		rc = render_node(renderer, child);
		if (rc != MD_OK)
			return rc;
	}
	return MD_OK;
}

static int render_list(struct morph_md_kitty *renderer, cmark_node *node)
{
	cmark_node *item;
	int rc;

	renderer->list_depth++;
	for (item = cmark_node_first_child(node); item; item = cmark_node_next(item)) {
		rc = render_node(renderer, item);
		if (rc != MD_OK)
			return rc;
	}
	renderer->list_depth--;
	fputc('\n', stdout);
	return MD_OK;
}

static void print_list_prefix(struct morph_md_kitty *renderer)
{
	int i;

	for (i = 1; i < renderer->list_depth; i++)
		fputs("  ", stdout);
	fputs("- ", stdout);
}

static void print_task_prefix(struct morph_md_kitty *renderer, cmark_node *item)
{
	int i;

	for (i = 1; i < renderer->list_depth; i++)
		fputs("  ", stdout);
	if (cmark_gfm_extensions_get_tasklist_item_checked(item))
		fputs("- [x] ", stdout);
	else
		fputs("- [ ] ", stdout);
}

static int render_item(struct morph_md_kitty *renderer, cmark_node *node)
{
	print_list_prefix(renderer);
	(void)render_children(renderer, node);
	return MD_OK;
}

static int render_task_item(struct morph_md_kitty *renderer, cmark_node *node)
{
	print_task_prefix(renderer, node);
	(void)render_children(renderer, node);
	return MD_OK;
}

static int render_table_row(cmark_node *row)
{
	cmark_node *cell;
	char *text;

	fputc('|', stdout);
	for (cell = cmark_node_first_child(row); cell; cell = cmark_node_next(cell)) {
		text = plain_text_dup(cell);
		printf(" %s |", text ? text : "");
		free(text);
	}
	fputc('\n', stdout);
	return MD_OK;
}

static int render_table(cmark_node *node)
{
	cmark_node *row;
	int first;

	first = 1;
	for (row = cmark_node_first_child(node); row; row = cmark_node_next(row)) {
		(void)render_table_row(row);
		if (first) {
			fputs("|", stdout);
			for (cmark_node *cell = cmark_node_first_child(row); cell;
			     cell = cmark_node_next(cell))
				fputs(" --- |", stdout);
			fputc('\n', stdout);
			first = 0;
		}
	}
	fputc('\n', stdout);
	return MD_OK;
}

static int render_node(struct morph_md_kitty *renderer, cmark_node *node)
{
	const char *literal;
	const char *kind;
	cmark_node_type type;

	type = cmark_node_get_type(node);
	kind = cmark_node_get_type_string(node);
	literal = cmark_node_get_literal(node);
	if (type == CMARK_NODE_TEXT && literal)
		return render_text_with_math(renderer, literal);
	if (type == CMARK_NODE_STRONG) {
		fputs("\033[1m", stdout);
		(void)render_children(renderer, node);
		fputs("\033[0m", stdout);
		return MD_OK;
	}
	if (type == CMARK_NODE_EMPH) {
		fputs("\033[3m", stdout);
		(void)render_children(renderer, node);
		fputs("\033[0m", stdout);
		return MD_OK;
	}
	if (type == CMARK_NODE_LINK) {
		(void)render_children(renderer, node);
		printf(" (%s)", cmark_node_get_url(node));
		return MD_OK;
	}
	if (type == CMARK_NODE_CODE && literal) {
		printf("`%s`", literal);
		return MD_OK;
	}
	if (type == CMARK_NODE_CODE_BLOCK && literal) {
		printf("\n```%s\n%s```\n", cmark_node_get_fence_info(node), literal);
		return MD_OK;
	}
	if (type == CMARK_NODE_SOFTBREAK || type == CMARK_NODE_LINEBREAK) {
		fputc('\n', stdout);
		return MD_OK;
	}
	if (type == CMARK_NODE_IMAGE) {
		printf("[image: %s]", cmark_node_get_url(node));
		return MD_OK;
	}
	if (kind && strcmp(kind, "table") == 0)
		return render_table(node);
	if (kind && strcmp(kind, "tasklist") == 0)
		return render_task_item(renderer, node);
	if (type == CMARK_NODE_LIST)
		return render_list(renderer, node);
	if (type == CMARK_NODE_ITEM)
		return render_item(renderer, node);
	if (type == CMARK_NODE_PARAGRAPH || type == CMARK_NODE_HEADING ||
	    type == CMARK_NODE_BLOCK_QUOTE) {
		if (type == CMARK_NODE_HEADING)
			printf("\033[1m");
		(void)render_children(renderer, node);
		if (type == CMARK_NODE_HEADING)
			printf("\033[0m");
		fputs("\n\n", stdout);
		return MD_OK;
	}
	if (type == CMARK_NODE_DOCUMENT)
		return render_children(renderer, node);
	return render_children(renderer, node);
}

struct morph_md_kitty *morph_md_kitty_create(
	const struct morph_md_kitty_options *options)
{
	struct morph_md_kitty *renderer;
	mjx_opts mjx_options;

	renderer = calloc(1u, sizeof(*renderer));
	if (!renderer)
		return NULL;
	if (options)
		renderer->options = *options;
	if (renderer->options.font_size <= 0.0)
		renderer->options.font_size = 30.0;
	if (renderer->options.dpi == 0)
		renderer->options.dpi = 72u;
	if (!options) {
		renderer->options.enable_gfm = 1;
		renderer->options.enable_math = 1;
	}

	memset(&mjx_options, 0, sizeof(mjx_options));
	mjx_options.font_path = renderer->options.font_path;
	mjx_options.font_size = renderer->options.font_size;
	mjx_options.fg_color = renderer->options.fg_color ?
			       renderer->options.fg_color : 0xFFFFFFu;
	mjx_options.bg_color = renderer->options.bg_color;
	mjx_options.dpi = renderer->options.dpi;
	renderer->math = mjx_init(&mjx_options);
	if (!renderer->math) {
		free(renderer);
		return NULL;
	}
	md_buf_init(&renderer->markdown);
	return renderer;
}

int morph_md_kitty_append(struct morph_md_kitty *renderer,
			  const char *bytes,
			  size_t len,
			  int is_final)
{
	(void)is_final;
	if (!renderer || (!bytes && len > 0))
		return MD_ERR_INVALID;
	return md_buf_append(&renderer->markdown, bytes, len);
}

int morph_md_kitty_render(struct morph_md_kitty *renderer)
{
	cmark_node *doc;
	int rc;

	if (!renderer)
		return MD_ERR_INVALID;
	doc = parse_markdown(renderer->markdown.data, renderer->markdown.len,
			     renderer->options.enable_gfm);
	if (!doc)
		return MD_ERR_PARSE;
	rc = render_node(renderer, doc);
	cmark_node_free(doc);
	fflush(stdout);
	return rc;
}

void morph_md_kitty_destroy(struct morph_md_kitty *renderer)
{
	if (!renderer)
		return;
	md_buf_cleanup(&renderer->markdown);
	mjx_free(renderer->math);
	free(renderer);
}
