#ifndef MORPH_MARKDOWN_KITTY_H
#define MORPH_MARKDOWN_KITTY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct morph_md_kitty_options {
	const char *font_path;
	/* <= 0 means use current terminal cell height. */
	double font_size;
	uint32_t fg_color;
	uint32_t bg_color;
	unsigned int dpi;
	int enable_gfm;
	int enable_math;
};

struct morph_md_kitty;

struct morph_md_kitty *morph_md_kitty_create(
	const struct morph_md_kitty_options *options);

int morph_md_kitty_append(struct morph_md_kitty *renderer,
			  const char *bytes,
			  size_t len,
			  int is_final);

int morph_md_kitty_render(struct morph_md_kitty *renderer);

void morph_md_kitty_destroy(struct morph_md_kitty *renderer);

#ifdef __cplusplus
}
#endif

#endif
