#ifndef MORPH_MD_WIDTH_H
#define MORPH_MD_WIDTH_H

#include <stddef.h>

int md_utf8_display_width(const char *text);
int md_utf8_display_width_n(const char *text, size_t len);
size_t md_utf8_grapheme_len(const char *text, size_t len);
int md_utf8_grapheme_width_n(const char *text, size_t len);
int md_utf8_is_space_n(const char *text, size_t len);
int md_utf8_break_allowed_between(const char *left, size_t left_len,
				  const char *right, size_t right_len);

#endif
