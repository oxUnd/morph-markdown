#ifndef MORPH_MARKDOWN_KITTY_H
#define MORPH_MARKDOWN_KITTY_H

#include <stddef.h>
#include <stdint.h>

#include "morph_markdown.h"

#ifdef __cplusplus
extern "C" {
#endif

struct morph_md_kitty_options {
	const char *font_path;
	/* <= 0 means use current terminal cell height. */
	double font_size;
	/* mathjax-c stores colors as 0xRRGGBBAA. */
	uint32_t fg_color;
	uint32_t bg_color;
	unsigned int dpi;
	uint32_t features;
	/*
	 * Receives UTF-8 text, ANSI styling and Kitty graphics escape sequences.
	 * Return 0 on success. NULL writes to stdout.
	 */
	int (*write)(const char *bytes, size_t len, void *user_data);
	void *user_data;
	/*
	 * Receives media references the SDK could not render directly, including
	 * remote images, invalid/non-PNG files and videos. file:// prefixes are
	 * removed before the callback is invoked. Callbacks are invoked in document
	 * order at the corresponding Markdown position, between ordered text
	 * writes, and never deferred until the end of the document.
	 */
	void (*media)(const char *type, const char *path, void *user_data);
	void *media_user_data;
	/* File descriptor used only for terminal cell-size detection. */
	int terminal_fd;
	/* Zero queries terminal_fd and falls back to 80 columns. */
	unsigned int terminal_columns;
	/* Zero queries terminal_fd and falls back to 24 rows. */
	unsigned int terminal_rows;
	/* Content padding uses terminal rows and columns, not pixels. */
	unsigned int content_padding_top_rows;
	unsigned int content_padding_right_columns;
	unsigned int content_padding_bottom_rows;
	unsigned int content_padding_left_columns;
	/*
	 * Cursor column before the first rendered row. Use this when a caller
	 * writes a visible prefix before rendering; continuation rows still use
	 * content_padding_left_columns.
	 */
	unsigned int initial_cursor_column;
};

struct morph_md_kitty;

struct morph_md_kitty *morph_md_kitty_create(
	const struct morph_md_kitty_options *options);

int morph_md_kitty_append(struct morph_md_kitty *renderer,
			  const char *bytes,
			  size_t len,
			  int is_final);

/*
 * Appends only newly stable rendered rows at the current terminal position.
 * Mutable input remains buffered until it is stable, so normal-screen output
 * never rewinds or mutates scrollback.
 */
int morph_md_kitty_render(struct morph_md_kitty *renderer);

/* Writes visible UTF-8 text using the renderer's padding and wrapping state. */
int morph_md_kitty_write_text(struct morph_md_kitty *renderer,
			      const char *bytes,
			      size_t len);

/*
 * Brackets output with synchronized-output mode so supporting terminals
 * publish the buffered update atomically.
 */
int morph_md_kitty_begin_frame(struct morph_md_kitty *renderer);
int morph_md_kitty_end_frame(struct morph_md_kitty *renderer);

/*
 * Explicitly deletes Kitty placements, clears the terminal viewport and
 * resets incremental presentation state. The next render paints a snapshot.
 */
int morph_md_kitty_clear(struct morph_md_kitty *renderer);

void morph_md_kitty_destroy(struct morph_md_kitty *renderer);

#ifdef __cplusplus
}
#endif

#endif
