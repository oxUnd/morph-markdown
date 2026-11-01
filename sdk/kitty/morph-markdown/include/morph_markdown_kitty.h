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
	 * remote images, invalid/non-PNG files and videos when no image loader is
	 * configured. With load_image, image failures stay in layout and only
	 * videos use this callback. file:// prefixes are
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
	/* Resolve an image URL to a local PNG without writing to the terminal.
	 * The SDK caches successes and failures for this renderer's lifetime.
	 * Return 0 with an allocated path on success, nonzero with NULL on failure.
	 * release_image releases the path at destroy (NULL defaults to free).
	 * Failed images remain placeholders in their original layout position.
	 */
	int (*load_image)(const char *url, char **path, void *user_data);
	void (*release_image)(char *path, void *user_data);
	void *image_user_data;
	/* Extra limits for block image previews; zero keeps viewport sizing.
	 * Images retain their aspect ratio and are never enlarged. Table images
	 * retain intrinsic sizing for table layout.
	 */
	unsigned int image_max_width_columns;
	unsigned int image_max_height_rows;
	/* Show (original path or URL · plain alt text) below block images.
	 * The path or URL is an OSC 8 hyperlink; local paths use file:// targets.
	 */
	int image_caption;
	/* Optional replacement for the mutable Markdown tail. Receives rendered
	 * ANSI text without cursor movement, image transfers or media callbacks.
	 * Empty text clears the preview before stable output is appended. The host
	 * owns a bounded, redrawable region outside committed scrollback.
	 * Uses user_data and returns zero on success.
	 */
	int (*preview)(const char *bytes, size_t len, void *user_data);
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
 * Mutable input is sent to options.preview when configured, otherwise buffered.
 * Stable output never rewinds or mutates scrollback.
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
