#ifndef MORPH_MD_BUF_H
#define MORPH_MD_BUF_H

#include <stdarg.h>
#include <stddef.h>

struct md_buf {
	char *data;
	size_t len;
	size_t cap;
};

void md_buf_init(struct md_buf *buf);
void md_buf_cleanup(struct md_buf *buf);
int md_buf_reserve(struct md_buf *buf, size_t needed);
int md_buf_append(struct md_buf *buf, const char *data, size_t len);
int md_buf_puts(struct md_buf *buf, const char *text);
int md_buf_vprintf(struct md_buf *buf, const char *fmt, va_list ap);
int md_buf_printf(struct md_buf *buf, const char *fmt, ...);
char *md_buf_detach(struct md_buf *buf);

#endif
