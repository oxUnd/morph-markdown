#ifndef MORPH_MD_WIDTH_H
#define MORPH_MD_WIDTH_H

#include <stddef.h>

int md_utf8_display_width(const char *text);
int md_utf8_display_width_n(const char *text, size_t len);

#endif
