#include "morph_markdown_kitty.h"
#include "base/md_array.h"
#include "base/md_buf.h"
#include "base/md_error.h"
#include "base/md_width.h"
#include "md_math_ext.h"

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>
#include <mathjax.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct table_cell_text {
	char *text;
	int width;
	int rows;
};

struct table_row_text {
	struct md_array cells;
};

struct list_state {
	cmark_list_type type;
	int next_number;
};

struct media_ref {
	char *type;
	char *path;
};

struct morph_md_kitty {
	struct morph_md_kitty_options options;
	struct md_buf markdown;
	struct md_buf frame_output;
	struct md_array lists;
	struct md_array media;
	mjx_ctx *math;
	unsigned int viewport_columns;
	unsigned int content_column;
	int line_started;
	int wrap_suppression;
	int item_depth;
	int frame_depth;
	int finalized;
};

struct terminal_cell_size {
	double width;
	double height;
};

static int stdout_write(const char *bytes, size_t len, void *user_data)
{
	(void)user_data;
	return fwrite(bytes, 1u, len, stdout) == len ? MD_OK : -EIO;
}

static int renderer_write(struct morph_md_kitty *renderer,
			  const char *bytes, size_t len)
{
	if (len == 0u)
		return MD_OK;
	if (renderer->frame_depth > 0)
		return md_buf_append(&renderer->frame_output, bytes, len);
	return renderer->options.write(bytes, len, renderer->options.user_data);
}

static int is_video_path(const char *path)
{
	static const char *const extensions[] = {
		".mp4", ".mov", ".avi", ".mkv", ".m4v", ".webm", ".mpeg"
	};
	const char *end;
	size_t path_len;
	size_t i;

	if (!path)
		return 0;
	end = strpbrk(path, "?#");
	path_len = end ? (size_t)(end - path) : strlen(path);
	for (i = 0u; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
		size_t extension_len = strlen(extensions[i]);

		if (path_len >= extension_len &&
		    strcasecmp(path + path_len - extension_len,
			       extensions[i]) == 0)
			return 1;
	}
	return 0;
}

static int collect_media(struct morph_md_kitty *renderer,
			 const char *type, const char *url)
{
	struct media_ref *media;
	const char *path;

	if (!renderer->options.media || !url || !url[0])
		return MD_OK;
	path = strncmp(url, "file://", 7u) == 0 ? url + 7 : url;
	media = md_array_push(&renderer->media);
	if (!media)
		return MD_ERR_NOMEM;
	memset(media, 0, sizeof(*media));
	media->type = strdup(type);
	media->path = strdup(path);
	if (!media->type || !media->path)
		return MD_ERR_NOMEM;
	return MD_OK;
}

static void emit_and_clear_media(struct morph_md_kitty *renderer, int emit)
{
	size_t i;

	for (i = 0u; i < renderer->media.len; i++) {
		struct media_ref *media = md_array_get(&renderer->media, i);

		if (emit && renderer->options.media &&
		    media->type && media->path) {
			renderer->options.media(media->type, media->path,
						renderer->options.media_user_data);
		}
		free(media->type);
		free(media->path);
	}
	renderer->media.len = 0u;
}

static unsigned int terminal_column_count(int fd)
{
	struct winsize ws;

	memset(&ws, 0, sizeof(ws));
	if (fd >= 0 && ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
		return ws.ws_col;
	return 80u;
}

static unsigned int content_right_edge(struct morph_md_kitty *renderer)
{
	unsigned int right;
	unsigned int left;

	left = renderer->options.content_padding_left_columns;
	right = renderer->options.content_padding_right_columns;
	if (renderer->viewport_columns > right + left)
		return renderer->viewport_columns - right;
	return left + 1u;
}

static size_t utf8_sequence_len(const char *bytes, size_t remaining)
{
	unsigned char first;
	size_t length;

	first = (unsigned char)bytes[0];
	if (first < 0x80u)
		length = 1u;
	else if ((first & 0xe0u) == 0xc0u)
		length = 2u;
	else if ((first & 0xf0u) == 0xe0u)
		length = 3u;
	else if ((first & 0xf8u) == 0xf0u)
		length = 4u;
	else
		length = 1u;
	return length <= remaining ? length : 1u;
}

static int renderer_start_content_line(struct morph_md_kitty *renderer)
{
	unsigned int i;
	int rc = MD_OK;

	if (renderer->line_started)
		return MD_OK;
	for (i = 0u;
	     rc == MD_OK && i < renderer->options.content_padding_left_columns;
	     i++)
		rc = renderer_write(renderer, " ", 1u);
	if (rc == MD_OK) {
		renderer->content_column =
			renderer->options.content_padding_left_columns;
		renderer->line_started = 1;
	}
	return rc;
}

static int renderer_newline(struct morph_md_kitty *renderer)
{
	int rc;

	rc = renderer_write(renderer, "\n", 1u);
	if (rc == MD_OK) {
		renderer->content_column = 0u;
		renderer->line_started = 0;
	}
	return rc;
}

static int renderer_prepare_width(struct morph_md_kitty *renderer,
				  unsigned int width)
{
	int rc;

	if (renderer->wrap_suppression == 0 && renderer->line_started &&
	    renderer->content_column + width >
		    content_right_edge(renderer)) {
		rc = renderer_newline(renderer);
		if (rc != MD_OK)
			return rc;
	}
	return renderer_start_content_line(renderer);
}

static int renderer_visible_write(struct morph_md_kitty *renderer,
				  const char *bytes, size_t len)
{
	size_t offset = 0u;
	size_t token_len;
	size_t token_offset;
	size_t step;
	unsigned int available;
	unsigned int token_width;
	int width;
	int rc;

	while (offset < len) {
		if (bytes[offset] == '\n') {
			rc = renderer_newline(renderer);
			offset++;
			if (rc != MD_OK)
				return rc;
			continue;
		}
		token_len = 0u;
		while (offset + token_len < len &&
		       bytes[offset + token_len] != '\n' &&
		       bytes[offset + token_len] != ' ' &&
		       bytes[offset + token_len] != '\t')
			token_len++;
		if (token_len > 0u) {
			token_width = (unsigned int)md_utf8_display_width_n(
				bytes + offset, token_len);
			available = content_right_edge(renderer) -
				renderer->options.content_padding_left_columns;
			if (renderer->wrap_suppression == 0 &&
			    renderer->line_started &&
			    token_width <= available &&
			    renderer->content_column + token_width >
				    content_right_edge(renderer)) {
				rc = renderer_newline(renderer);
				if (rc != MD_OK)
					return rc;
			}
			token_offset = 0u;
			while (token_offset < token_len) {
				step = utf8_sequence_len(
					bytes + offset + token_offset,
					token_len - token_offset);
				width = md_utf8_display_width_n(
					bytes + offset + token_offset, step);
				rc = renderer_prepare_width(
					renderer, (unsigned int)width);
				if (rc == MD_OK)
					rc = renderer_write(
						renderer,
						bytes + offset + token_offset,
						step);
				if (rc != MD_OK)
					return rc;
				renderer->content_column +=
					(unsigned int)width;
				token_offset += step;
			}
			offset += token_len;
			continue;
		}
		step = utf8_sequence_len(bytes + offset, len - offset);
		width = md_utf8_display_width_n(bytes + offset, step);
		if (renderer->wrap_suppression == 0 &&
		    renderer->line_started &&
		    renderer->content_column + (unsigned int)width >
			    content_right_edge(renderer)) {
			rc = renderer_newline(renderer);
			if (rc != MD_OK)
				return rc;
			if (bytes[offset] == ' ') {
				offset += step;
				continue;
			}
		}
		rc = renderer_start_content_line(renderer);
		if (rc == MD_OK)
			rc = renderer_write(renderer, bytes + offset, step);
		if (rc != MD_OK)
			return rc;
		renderer->content_column += (unsigned int)width;
		offset += step;
	}
	return MD_OK;
}

static int renderer_puts(struct morph_md_kitty *renderer, const char *text)
{
	return renderer_visible_write(renderer, text, strlen(text));
}

static int renderer_putc(struct morph_md_kitty *renderer, char value)
{
	return renderer_visible_write(renderer, &value, 1u);
}

static int renderer_printf(struct morph_md_kitty *renderer,
			   const char *format, ...)
{
	struct md_buf output;
	va_list args;
	va_list copy;
	int length;
	int rc;

	va_start(args, format);
	va_copy(copy, args);
	length = vsnprintf(NULL, 0u, format, copy);
	va_end(copy);
	if (length < 0) {
		va_end(args);
		return -EIO;
	}
	md_buf_init(&output);
	rc = md_buf_reserve(&output, (size_t)length + 1u);
	if (rc == MD_OK) {
		(void)vsnprintf(output.data, (size_t)length + 1u, format, args);
		output.len = (size_t)length;
		rc = renderer_visible_write(renderer, output.data, output.len);
	}
	md_buf_cleanup(&output);
	va_end(args);
	return rc;
}

static int renderer_control_puts(struct morph_md_kitty *renderer,
				 const char *text)
{
	return renderer_write(renderer, text, strlen(text));
}

static int renderer_control_printf(struct morph_md_kitty *renderer,
				   const char *format, ...)
{
	struct md_buf output;
	va_list args;
	int rc;

	md_buf_init(&output);
	va_start(args, format);
	rc = md_buf_vprintf(&output, format, args);
	va_end(args);
	if (rc == MD_OK)
		rc = renderer_write(renderer, output.data, output.len);
	md_buf_cleanup(&output);
	return rc;
}

static struct terminal_cell_size terminal_cell_size(int fd)
{
	struct winsize ws;
	struct terminal_cell_size size = { 9.0, 18.0 };

	memset(&ws, 0, sizeof(ws));
	if (fd >= 0 && ioctl(fd, TIOCGWINSZ, &ws) == 0) {
		if (ws.ws_col > 0 && ws.ws_xpixel > 0)
			size.width = (double)ws.ws_xpixel / (double)ws.ws_col;
		if (ws.ws_row > 0 && ws.ws_ypixel > 0)
			size.height = (double)ws.ws_ypixel / (double)ws.ws_row;
	}
	return size;
}

static void attach_extension(cmark_parser *parser, const char *name)
{
	cmark_syntax_extension *extension;

	extension = cmark_find_syntax_extension(name);
	if (extension)
		cmark_parser_attach_syntax_extension(parser, extension);
}

static cmark_node *parse_markdown(const char *md, size_t len,
				  int enable_gfm, int enable_math)
{
	cmark_parser *parser;
	cmark_syntax_extension *math;
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
	if (enable_math) {
		math = morph_md_create_math_extension();
		if (math)
			cmark_parser_attach_syntax_extension(parser, math);
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

static const char kitty_base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static size_t base64_encode(const unsigned char *input, size_t input_len,
			    char *output)
{
	size_t input_pos = 0u;
	size_t output_pos = 0u;
	unsigned int a;
	unsigned int b;
	unsigned int c;
	unsigned int triple;

	while (input_pos < input_len) {
		a = input_pos < input_len ? input[input_pos++] : 0u;
		b = input_pos < input_len ? input[input_pos++] : 0u;
		c = input_pos < input_len ? input[input_pos++] : 0u;
		triple = (a << 16u) | (b << 8u) | c;
		output[output_pos++] = kitty_base64[(triple >> 18u) & 0x3fu];
		output[output_pos++] = kitty_base64[(triple >> 12u) & 0x3fu];
		output[output_pos++] = kitty_base64[(triple >> 6u) & 0x3fu];
		output[output_pos++] = kitty_base64[triple & 0x3fu];
	}
	if (input_len % 3u == 1u) {
		output[output_pos - 2u] = '=';
		output[output_pos - 1u] = '=';
	} else if (input_len % 3u == 2u) {
		output[output_pos - 1u] = '=';
	}
	return output_pos;
}

static unsigned char *copy_rgba_pixels(const mjx_buf *buffer,
					size_t *byte_count)
{
	const uint32_t *pixels;
	unsigned char *rgba;
	size_t pixel_count;
	size_t i;
	uint32_t pixel;

	pixel_count = (size_t)mjx_buf_width(buffer) * mjx_buf_height(buffer);
	if (pixel_count > SIZE_MAX / 4u)
		return NULL;
	*byte_count = pixel_count * 4u;
	rgba = malloc(*byte_count ? *byte_count : 1u);
	if (!rgba)
		return NULL;
	pixels = mjx_buf_pixels(buffer);
	for (i = 0u; i < pixel_count; i++) {
		pixel = pixels[i];
		rgba[i * 4u] = (unsigned char)(pixel >> 24u);
		rgba[i * 4u + 1u] = (unsigned char)(pixel >> 16u);
		rgba[i * 4u + 2u] = (unsigned char)(pixel >> 8u);
		rgba[i * 4u + 3u] = (unsigned char)pixel;
	}
	return rgba;
}

static int send_kitty_rgba(struct morph_md_kitty *renderer,
			   const mjx_buf *buffer)
{
	const size_t raw_chunk_size = 3072u;
	unsigned char *rgba;
	char encoded[4096];
	size_t byte_count;
	size_t offset = 0u;
	size_t chunk_size;
	size_t encoded_len;
	int more;
	int rc;

	rgba = copy_rgba_pixels(buffer, &byte_count);
	if (!rgba)
		return MD_ERR_NOMEM;
	rc = MD_OK;
	while (offset < byte_count) {
		chunk_size = byte_count - offset;
		if (chunk_size > raw_chunk_size)
			chunk_size = raw_chunk_size;
		more = offset + chunk_size < byte_count;
		if (offset == 0u) {
			rc = renderer_control_printf(
				renderer,
				"\033_Ga=T,f=32,s=%u,v=%u,C=1,q=2,m=%d;",
				mjx_buf_width(buffer), mjx_buf_height(buffer), more);
		} else {
			rc = renderer_control_printf(renderer, "\033_Gm=%d;", more);
		}
		encoded_len = base64_encode(rgba + offset, chunk_size, encoded);
		if (rc == MD_OK)
			rc = renderer_write(renderer, encoded, encoded_len);
		if (rc == MD_OK)
			rc = renderer_control_puts(renderer, "\033\\");
		if (rc != MD_OK)
			break;
		offset += chunk_size;
	}
	free(rgba);
	return rc;
}

static unsigned int formula_columns(struct morph_md_kitty *renderer,
				    const mjx_buf *buffer)
{
	struct terminal_cell_size cell;
	unsigned int columns;

	cell = terminal_cell_size(renderer->options.terminal_fd);
	columns = (unsigned int)((double)mjx_buf_width(buffer) / cell.width);
	if ((double)columns * cell.width < (double)mjx_buf_width(buffer))
		columns++;
	if (columns == 0u)
		columns = 1u;
	return columns;
}

static unsigned int formula_rows(struct morph_md_kitty *renderer,
				 const mjx_buf *buffer)
{
	struct terminal_cell_size cell;
	unsigned int rows;

	cell = terminal_cell_size(renderer->options.terminal_fd);
	rows = (unsigned int)((double)mjx_buf_height(buffer) / cell.height);
	if ((double)rows * cell.height < (double)mjx_buf_height(buffer))
		rows++;
	return rows > 0u ? rows : 1u;
}

static mjx_buf *render_formula_buffer(struct morph_md_kitty *renderer,
				      const char *latex, size_t len,
				      mjx_style style)
{
	char *expr;
	mjx_buf *buffer;

	expr = malloc(len + 1u);
	if (!expr)
		return NULL;
	memcpy(expr, latex, len);
	expr[len] = '\0';
	buffer = mjx_render_latex(renderer->math, expr, style);
	free(expr);
	return buffer;
}

static int render_formula(struct morph_md_kitty *renderer,
			  const char *latex, size_t len, mjx_style style)
{
	mjx_buf *buf;
	unsigned int columns;
	int rc;

	buf = render_formula_buffer(renderer, latex, len, style);
	if (!buf)
		return MD_ERR_PARSE;

	columns = formula_columns(renderer, buf);
	rc = renderer_prepare_width(renderer, columns);
	if (rc == MD_OK)
		rc = send_kitty_rgba(renderer, buf);
	if (rc == MD_OK)
		rc = renderer_control_printf(renderer, "\033[%uC", columns);
	if (rc == MD_OK)
		renderer->content_column += columns;
	mjx_buf_free(buf);
	return rc;
}

static int text_math_metrics(struct morph_md_kitty *renderer,
			     const char *text, int *out_width, int *out_rows)
{
	size_t len = strlen(text);
	size_t i = 0u;
	size_t plain_start = 0u;
	size_t open_len;
	size_t close_len;
	size_t close_pos;
	const char *close;
	mjx_style style;
	mjx_buf *buffer;
	unsigned int rows;
	int width = 0;

	while (i < len) {
		if (!((renderer->options.features & MORPH_MD_FEATURE_MATH) != 0u) ||
		    !is_math_start(text, len, i, &open_len, &close,
				   &close_len, &style)) {
			i++;
			continue;
		}
		close_pos = find_close(text, len, i + open_len, close, close_len);
		if (close_pos >= len) {
			i++;
			continue;
		}
		width += md_utf8_display_width_n(text + plain_start,
						 i - plain_start);
		buffer = render_formula_buffer(renderer, text + i + open_len,
					       close_pos - i - open_len, style);
		if (!buffer)
			return MD_ERR_PARSE;
		width += (int)formula_columns(renderer, buffer);
		rows = formula_rows(renderer, buffer);
		if ((int)rows > *out_rows)
			*out_rows = (int)rows;
		mjx_buf_free(buffer);
		i = close_pos + close_len;
		plain_start = i;
	}
	width += md_utf8_display_width_n(text + plain_start, len - plain_start);
	*out_width = width;
	return MD_OK;
}

static int render_text_with_math(struct morph_md_kitty *renderer,
				 const char *text)
{
	size_t len;
	size_t i;
	size_t plain_start;
	size_t open_len;
	size_t close_len;
	size_t close_pos;
	const char *close;
	mjx_style style;
	int rc;

	len = strlen(text);
	i = 0u;
	plain_start = 0u;
	while (i < len) {
		if (!((renderer->options.features & MORPH_MD_FEATURE_MATH) != 0u) ||
		    !is_math_start(text, len, i, &open_len, &close,
				   &close_len, &style)) {
			i += utf8_sequence_len(text + i, len - i);
			continue;
		}
		close_pos = find_close(text, len, i + open_len, close, close_len);
		if (close_pos >= len) {
			i += utf8_sequence_len(text + i, len - i);
			continue;
		}
		rc = renderer_visible_write(renderer, text + plain_start,
					    i - plain_start);
		if (rc != MD_OK)
			return rc;
		if (style == MJX_STYLE_DISPLAY)
			if (renderer_putc(renderer, '\n') != MD_OK)
				return -EIO;
		rc = render_formula(renderer, text + i + open_len,
				    close_pos - i - open_len, style);
		if (rc != MD_OK)
			return rc;
		if (style == MJX_STYLE_DISPLAY)
			if (renderer_putc(renderer, '\n') != MD_OK)
				return -EIO;
		i = close_pos + close_len;
		plain_start = i;
	}
	return renderer_visible_write(renderer, text + plain_start,
				      len - plain_start);
}

static int render_node(struct morph_md_kitty *renderer, cmark_node *node);

static int append_plain_text(struct md_buf *out, cmark_node *node)
{
	cmark_node *child;
	const char *literal;
	int rc;

	if (cmark_node_get_type(node) == MORPH_MD_NODE_MATH_INLINE ||
	    cmark_node_get_type(node) == MORPH_MD_NODE_MATH_BLOCK) {
		literal = morph_md_math_literal(node);
		rc = md_buf_puts(out,
				 cmark_node_get_type(node) == MORPH_MD_NODE_MATH_BLOCK ?
				 "$$" : "$");
		if (rc == MD_OK)
			rc = md_buf_puts(out, literal ? literal : "");
		if (rc == MD_OK)
			rc = md_buf_puts(out,
					 cmark_node_get_type(node) ==
					 MORPH_MD_NODE_MATH_BLOCK ? "$$" : "$");
		return rc;
	}
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
	struct list_state *state;
	cmark_node *item;
	int rc;

	state = md_array_push(&renderer->lists);
	if (!state)
		return MD_ERR_NOMEM;
	state->type = cmark_node_get_list_type(node);
	state->next_number = cmark_node_get_list_start(node);
	for (item = cmark_node_first_child(node); item; item = cmark_node_next(item)) {
		rc = render_node(renderer, item);
		if (rc != MD_OK) {
			md_array_pop(&renderer->lists);
			return rc;
		}
	}
	md_array_pop(&renderer->lists);
	return renderer->lists.len == 0u ? renderer_putc(renderer, '\n') : MD_OK;
}

static int print_list_indent(struct morph_md_kitty *renderer)
{
	size_t i;
	int rc;

	for (i = 1u; i < renderer->lists.len; i++) {
		rc = renderer_puts(renderer, "   ");
		if (rc != MD_OK)
			return rc;
	}
	return MD_OK;
}

static int print_list_prefix(struct morph_md_kitty *renderer)
{
	static const char *const bullets[] = { "• ", "◦ ", "▪ " };
	struct list_state *state;
	int rc;

	rc = print_list_indent(renderer);
	if (rc != MD_OK)
		return rc;
	state = md_array_get(&renderer->lists, renderer->lists.len - 1u);
	if (state->type == CMARK_ORDERED_LIST)
		return renderer_printf(renderer, "%d. ", state->next_number++);
	return renderer_puts(renderer,
			     bullets[(renderer->lists.len - 1u) % 3u]);
}

static int print_task_prefix(struct morph_md_kitty *renderer, cmark_node *item)
{
	int rc;

	rc = print_list_indent(renderer);
	if (rc != MD_OK)
		return rc;
	if (cmark_gfm_extensions_get_tasklist_item_checked(item))
		return renderer_puts(renderer, "☑ ");
	return renderer_puts(renderer, "☐ ");
}

static int render_item(struct morph_md_kitty *renderer, cmark_node *node)
{
	int rc;

	rc = print_list_prefix(renderer);
	if (rc != MD_OK)
		return rc;
	renderer->item_depth++;
	rc = render_children(renderer, node);
	renderer->item_depth--;
	return rc;
}

static int render_task_item(struct morph_md_kitty *renderer, cmark_node *node)
{
	int rc;

	rc = print_task_prefix(renderer, node);
	if (rc != MD_OK)
		return rc;
	renderer->item_depth++;
	rc = render_children(renderer, node);
	renderer->item_depth--;
	return rc;
}

static void table_row_cleanup(struct table_row_text *row)
{
	struct table_cell_text *cell;
	size_t i;

	for (i = 0; i < row->cells.len; i++) {
		cell = md_array_get(&row->cells, i);
		free(cell->text);
	}
	md_array_cleanup(&row->cells);
}

static void table_rows_cleanup(struct md_array *rows)
{
	struct table_row_text *row;
	size_t i;

	for (i = 0; i < rows->len; i++) {
		row = md_array_get(rows, i);
		table_row_cleanup(row);
	}
	md_array_cleanup(rows);
}

static int collect_table(cmark_node *node, struct md_array *rows,
			 size_t *col_count)
{
	struct table_row_text *row_text;
	struct table_cell_text *cell_text;
	cmark_node *row;
	cmark_node *cell;

	md_array_init(rows, sizeof(struct table_row_text));
	*col_count = 0;
	for (row = cmark_node_first_child(node); row; row = cmark_node_next(row)) {
		row_text = md_array_push(rows);
		if (!row_text)
			return MD_ERR_NOMEM;
		md_array_init(&row_text->cells, sizeof(struct table_cell_text));
		for (cell = cmark_node_first_child(row); cell; cell = cmark_node_next(cell)) {
			cell_text = md_array_push(&row_text->cells);
			if (!cell_text)
				return MD_ERR_NOMEM;
			cell_text->text = NULL;
			cell_text->rows = 1;
			cell_text->text = plain_text_dup(cell);
			if (!cell_text->text)
				return MD_ERR_NOMEM;
		}
		if (row_text->cells.len > *col_count)
			*col_count = row_text->cells.len;
	}
	return MD_OK;
}

static int table_metrics(struct morph_md_kitty *renderer,
			 struct md_array *rows, size_t col_count, int *widths)
{
	struct table_row_text *row;
	struct table_cell_text *cell;
	size_t r;
	size_t c;
	int width;
	int rc;

	for (c = 0; c < col_count; c++)
		widths[c] = 3;
	for (r = 0; r < rows->len; r++) {
		row = md_array_get(rows, r);
		for (c = 0; c < row->cells.len; c++) {
			cell = md_array_get(&row->cells, c);
			cell->rows = 1;
			rc = text_math_metrics(renderer, cell->text,
					       &cell->width, &cell->rows);
			if (rc != MD_OK)
				return rc;
			width = cell->width;
			if (width > widths[c])
				widths[c] = width;
		}
	}
	return MD_OK;
}

static int print_border(struct morph_md_kitty *renderer, const char *left,
			const char *mid, const char *right, const int *widths,
			size_t col_count)
{
	size_t c;
	int i;
	int rc;

	rc = renderer_puts(renderer, left);
	for (c = 0; c < col_count; c++) {
		for (i = 0; rc == MD_OK && i < widths[c] + 2; i++)
			rc = renderer_puts(renderer, "─");
		if (rc == MD_OK)
			rc = renderer_puts(renderer,
					   c + 1u == col_count ? right : mid);
	}
	return rc == MD_OK ? renderer_putc(renderer, '\n') : rc;
}

static int print_padded_cell(struct morph_md_kitty *renderer,
			     struct table_cell_text *cell, int width)
{
	int used;
	int i;
	int rc;

	used = cell ? cell->width : 0;
	rc = renderer_putc(renderer, ' ');
	if (rc == MD_OK && cell)
		rc = render_text_with_math(renderer, cell->text);
	for (i = used; rc == MD_OK && i < width; i++)
		rc = renderer_putc(renderer, ' ');
	return rc == MD_OK ? renderer_putc(renderer, ' ') : rc;
}

static int print_empty_table_line(struct morph_md_kitty *renderer,
				  const int *widths, size_t col_count)
{
	size_t c;
	int i;
	int rc;

	rc = renderer_puts(renderer, "│");
	for (c = 0u; rc == MD_OK && c < col_count; c++) {
		for (i = 0; rc == MD_OK && i < widths[c] + 2; i++)
			rc = renderer_putc(renderer, ' ');
		if (rc == MD_OK)
			rc = renderer_puts(renderer, "│");
	}
	return rc == MD_OK ? renderer_putc(renderer, '\n') : rc;
}

static int print_table_row(struct morph_md_kitty *renderer,
			   struct table_row_text *row, const int *widths,
			   size_t col_count)
{
	struct table_cell_text *cell;
	size_t c;
	int row_height = 1;
	int line;
	int rc;

	rc = renderer_puts(renderer, "│");
	for (c = 0; rc == MD_OK && c < col_count; c++) {
		cell = c < row->cells.len ? md_array_get(&row->cells, c) : NULL;
		if (cell && cell->rows > row_height)
			row_height = cell->rows;
		rc = print_padded_cell(renderer, cell, widths[c]);
		if (rc == MD_OK)
			rc = renderer_puts(renderer, "│");
	}
	if (rc == MD_OK)
		rc = renderer_putc(renderer, '\n');
	for (line = 1; rc == MD_OK && line < row_height; line++)
		rc = print_empty_table_line(renderer, widths, col_count);
	return rc;
}

static int render_table(struct morph_md_kitty *renderer, cmark_node *node)
{
	struct table_row_text *row;
	struct md_array rows;
	size_t col_count;
	size_t r;
	int *widths;
	int rc;

	rc = collect_table(node, &rows, &col_count);
	if (rc != MD_OK) {
		table_rows_cleanup(&rows);
		return rc;
	}
	widths = calloc(col_count ? col_count : 1u, sizeof(*widths));
	if (!widths) {
		table_rows_cleanup(&rows);
		return MD_ERR_NOMEM;
	}
	rc = table_metrics(renderer, &rows, col_count, widths);
	if (rc == MD_OK)
		rc = print_border(renderer, "┌", "┬", "┐", widths, col_count);
	for (r = 0; rc == MD_OK && r < rows.len; r++) {
		row = md_array_get(&rows, r);
		rc = print_table_row(renderer, row, widths, col_count);
		if (rc == MD_OK && r == 0)
			rc = print_border(renderer, "├", "┼", "┤",
					  widths, col_count);
	}
	if (rc == MD_OK)
		rc = print_border(renderer, "└", "┴", "┘", widths, col_count);
	if (rc == MD_OK)
		rc = renderer_putc(renderer, '\n');
	free(widths);
	table_rows_cleanup(&rows);
	return rc;
}

static int render_heading(struct morph_md_kitty *renderer, cmark_node *node)
{
	static const char *const styles[] = {
		"\033[1;4;38;5;81m",
		"\033[1;38;5;75m",
		"\033[1;38;5;110m",
		"\033[1;38;5;146m",
		"\033[1;38;5;180m",
		"\033[1;38;5;244m"
	};
	static const char *const prefixes[] = {
		"", "▌ ", "› ", "· ", "  ", "  "
	};
	int level;
	int rc;
	int reset_rc;

	level = cmark_node_get_heading_level(node);
	if (level < 1 || level > 6)
		level = 6;
	rc = renderer_start_content_line(renderer);
	if (rc == MD_OK)
		rc = renderer_control_puts(renderer, styles[level - 1]);
	if (rc == MD_OK)
		rc = renderer_puts(renderer, prefixes[level - 1]);
	if (rc == MD_OK)
		rc = render_children(renderer, node);
	reset_rc = renderer_control_puts(renderer, "\033[0m");
	if (reset_rc == MD_OK)
		reset_rc = renderer_puts(renderer, "\n\n");
	return rc == MD_OK ? reset_rc : rc;
}

static int render_node(struct morph_md_kitty *renderer, cmark_node *node)
{
	const char *literal;
	const char *kind;
	cmark_node_type type;
	int rc;

	type = cmark_node_get_type(node);
	kind = cmark_node_get_type_string(node);
	literal = cmark_node_get_literal(node);
	if (type == MORPH_MD_NODE_MATH_INLINE ||
	    type == MORPH_MD_NODE_MATH_BLOCK) {
		literal = morph_md_math_literal(node);
		if (type == MORPH_MD_NODE_MATH_BLOCK)
			if (renderer_putc(renderer, '\n') != MD_OK)
				return -EIO;
		rc = render_formula(renderer, literal ? literal : "",
				    literal ? strlen(literal) : 0u,
				    type == MORPH_MD_NODE_MATH_BLOCK ?
				    MJX_STYLE_DISPLAY : MJX_STYLE_INLINE);
		if (rc == MD_OK && type == MORPH_MD_NODE_MATH_BLOCK)
			rc = renderer_putc(renderer, '\n');
		return rc;
	}
	if (type == CMARK_NODE_TEXT && literal)
		return render_text_with_math(renderer, literal);
	if (type == CMARK_NODE_STRONG) {
		rc = renderer_control_puts(renderer, "\033[1m");
		if (rc == MD_OK)
			rc = render_children(renderer, node);
		if (renderer_control_puts(renderer, "\033[0m") != MD_OK &&
		    rc == MD_OK)
			rc = -EIO;
		return rc;
	}
	if (type == CMARK_NODE_EMPH) {
		rc = renderer_control_puts(renderer, "\033[3m");
		if (rc == MD_OK)
			rc = render_children(renderer, node);
		if (renderer_control_puts(renderer, "\033[0m") != MD_OK &&
		    rc == MD_OK)
			rc = -EIO;
		return rc;
	}
	if (type == CMARK_NODE_LINK) {
		rc = render_children(renderer, node);
		if (rc == MD_OK && is_video_path(cmark_node_get_url(node)))
			rc = collect_media(renderer, "video",
					   cmark_node_get_url(node));
		return rc == MD_OK ?
			renderer_printf(renderer, " (%s)", cmark_node_get_url(node)) :
			rc;
	}
	if (type == CMARK_NODE_CODE && literal) {
		renderer->wrap_suppression++;
		rc = renderer_printf(renderer, "`%s`", literal);
		renderer->wrap_suppression--;
		return rc;
	}
	if (type == CMARK_NODE_CODE_BLOCK && literal) {
		renderer->wrap_suppression++;
		rc = renderer_printf(renderer, "\n```%s\n%s```\n",
				     cmark_node_get_fence_info(node), literal);
		renderer->wrap_suppression--;
		return rc;
	}
	if (type == CMARK_NODE_SOFTBREAK || type == CMARK_NODE_LINEBREAK) {
		return renderer_putc(renderer, '\n');
	}
	if (type == CMARK_NODE_IMAGE) {
		rc = collect_media(renderer,
				   is_video_path(cmark_node_get_url(node)) ?
				   "video" : "image",
				   cmark_node_get_url(node));
		return rc == MD_OK ?
			renderer_printf(renderer, "[image: %s]",
					cmark_node_get_url(node)) : rc;
	}
	if (kind && strcmp(kind, "table") == 0) {
		renderer->wrap_suppression++;
		rc = render_table(renderer, node);
		renderer->wrap_suppression--;
		return rc;
	}
	if (kind && strcmp(kind, "tasklist") == 0)
		return render_task_item(renderer, node);
	if (type == CMARK_NODE_LIST)
		return render_list(renderer, node);
	if (type == CMARK_NODE_ITEM)
		return render_item(renderer, node);
	if (type == CMARK_NODE_HEADING)
		return render_heading(renderer, node);
	if (type == CMARK_NODE_PARAGRAPH || type == CMARK_NODE_BLOCK_QUOTE) {
		rc = render_children(renderer, node);
		if (rc == MD_OK)
			rc = renderer_puts(renderer,
					   renderer->item_depth > 0 ? "\n" : "\n\n");
		return rc;
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
	else
		renderer->options.terminal_fd = STDOUT_FILENO;
	if (!renderer->options.write)
		renderer->options.write = stdout_write;
	if (renderer->options.font_size <= 0.0)
		renderer->options.font_size =
			terminal_cell_size(renderer->options.terminal_fd).height;
	if (renderer->options.dpi == 0)
		renderer->options.dpi = 72u;
	if (!options) {
		renderer->options.features = MORPH_MD_FEATURE_GFM;
	}

	if ((renderer->options.features & MORPH_MD_FEATURE_MATH) != 0u) {
		memset(&mjx_options, 0, sizeof(mjx_options));
		mjx_options.font_path = renderer->options.font_path;
		mjx_options.font_size = renderer->options.font_size;
		mjx_options.fg_color = renderer->options.fg_color ?
				       renderer->options.fg_color : 0xFFFFFFFFu;
		mjx_options.bg_color = renderer->options.bg_color;
		mjx_options.dpi = renderer->options.dpi;
		renderer->math = mjx_init(&mjx_options);
		if (!renderer->math) {
			free(renderer);
			return NULL;
		}
	}
	md_buf_init(&renderer->markdown);
	md_buf_init(&renderer->frame_output);
	md_array_init(&renderer->lists, sizeof(struct list_state));
	md_array_init(&renderer->media, sizeof(struct media_ref));
	return renderer;
}

int morph_md_kitty_append(struct morph_md_kitty *renderer,
			  const char *bytes,
			  size_t len,
			  int is_final)
{
	if (!renderer || (!bytes && len > 0))
		return MD_ERR_INVALID;
	if (renderer->finalized)
		return MD_ERR_INVALID;
	if (md_buf_append(&renderer->markdown, bytes, len) != MD_OK)
		return MD_ERR_NOMEM;
	renderer->finalized = is_final != 0;
	return MD_OK;
}

int morph_md_kitty_render(struct morph_md_kitty *renderer)
{
	cmark_node *doc;
	unsigned int i;
	int rc;

	if (!renderer)
		return MD_ERR_INVALID;
	renderer->viewport_columns = renderer->options.terminal_columns ?
		renderer->options.terminal_columns :
		terminal_column_count(renderer->options.terminal_fd);
	renderer->content_column = 0u;
	renderer->line_started = 0;
	for (i = 0u;
	     i < renderer->options.content_padding_top_rows;
	     i++) {
		rc = renderer_newline(renderer);
		if (rc != MD_OK)
			return rc;
	}
	doc = parse_markdown(renderer->markdown.data, renderer->markdown.len,
			     ((renderer->options.features & MORPH_MD_FEATURE_GFM) != 0u),
			     ((renderer->options.features & MORPH_MD_FEATURE_MATH) != 0u));
	if (!doc)
		return MD_ERR_PARSE;
	rc = render_node(renderer, doc);
	cmark_node_free(doc);
	for (i = 0u;
	     rc == MD_OK && i < renderer->options.content_padding_bottom_rows;
	     i++)
		rc = renderer_newline(renderer);
	if (renderer->options.write == stdout_write && renderer->frame_depth == 0)
		fflush(stdout);
	emit_and_clear_media(renderer, rc == MD_OK);
	return rc;
}

int morph_md_kitty_write_text(struct morph_md_kitty *renderer,
			      const char *bytes, size_t len)
{
	if (!renderer || (!bytes && len > 0u))
		return MD_ERR_INVALID;
	if (renderer->viewport_columns == 0u)
		renderer->viewport_columns = renderer->options.terminal_columns ?
			renderer->options.terminal_columns :
			terminal_column_count(renderer->options.terminal_fd);
	return renderer_visible_write(renderer, bytes, len);
}

int morph_md_kitty_begin_frame(struct morph_md_kitty *renderer)
{
	int rc;

	if (!renderer)
		return MD_ERR_INVALID;
	if (renderer->frame_depth > 0) {
		renderer->frame_depth++;
		return MD_OK;
	}
	renderer->frame_output.len = 0u;
	if (renderer->frame_output.data)
		renderer->frame_output.data[0] = '\0';
	renderer->frame_depth = 1;
	rc = renderer_control_puts(renderer, "\033[?2026h");
	if (rc != MD_OK)
		renderer->frame_depth = 0;
	return rc;
}

int morph_md_kitty_end_frame(struct morph_md_kitty *renderer)
{
	int rc;

	if (!renderer || renderer->frame_depth <= 0)
		return MD_ERR_INVALID;
	if (renderer->frame_depth > 1) {
		renderer->frame_depth--;
		return MD_OK;
	}
	rc = renderer_control_puts(renderer, "\033[?2026l");
	renderer->frame_depth = 0;
	if (rc == MD_OK)
		rc = renderer->options.write(renderer->frame_output.data,
					     renderer->frame_output.len,
					     renderer->options.user_data);
	renderer->frame_output.len = 0u;
	if (renderer->frame_output.data)
		renderer->frame_output.data[0] = '\0';
	if (renderer->options.write == stdout_write)
		fflush(stdout);
	return rc;
}

int morph_md_kitty_clear(struct morph_md_kitty *renderer)
{
	int rc;

	if (!renderer)
		return MD_ERR_INVALID;
	rc = renderer_control_puts(
		renderer, "\033_Ga=d,d=A,q=2\033\\\033[H\033[2J");
	if (rc == MD_OK) {
		renderer->content_column = 0u;
		renderer->line_started = 0;
	}
	return rc;
}

void morph_md_kitty_destroy(struct morph_md_kitty *renderer)
{
	if (!renderer)
		return;
	if (renderer->frame_depth > 0) {
		renderer->frame_depth = 1;
		(void)morph_md_kitty_end_frame(renderer);
	}
	md_buf_cleanup(&renderer->markdown);
	md_buf_cleanup(&renderer->frame_output);
	md_array_cleanup(&renderer->lists);
	emit_and_clear_media(renderer, 0);
	md_array_cleanup(&renderer->media);
	mjx_free(renderer->math);
	free(renderer);
}
