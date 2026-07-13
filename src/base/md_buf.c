#include "base/md_buf.h"
#include "base/md_error.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void md_buf_init(struct md_buf *buf)
{
	buf->data = NULL;
	buf->len = 0;
	buf->cap = 0;
}

void md_buf_cleanup(struct md_buf *buf)
{
	free(buf->data);
	md_buf_init(buf);
}

int md_buf_reserve(struct md_buf *buf, size_t needed)
{
	char *next;
	size_t cap;

	if (needed <= buf->cap)
		return MD_OK;

	cap = buf->cap ? buf->cap : 256u;
	while (cap < needed) {
		if (cap > SIZE_MAX / 2u)
			return MD_ERR_NOMEM;
		cap *= 2u;
	}

	next = realloc(buf->data, cap);
	if (!next)
		return MD_ERR_NOMEM;

	buf->data = next;
	buf->cap = cap;
	return MD_OK;
}

int md_buf_append(struct md_buf *buf, const char *data, size_t len)
{
	int rc;

	if (!data || len == 0)
		return MD_OK;
	if (len > SIZE_MAX - buf->len - 1u)
		return MD_ERR_NOMEM;

	rc = md_buf_reserve(buf, buf->len + len + 1u);
	if (rc != MD_OK)
		return rc;

	memcpy(buf->data + buf->len, data, len);
	buf->len += len;
	buf->data[buf->len] = '\0';
	return MD_OK;
}

int md_buf_puts(struct md_buf *buf, const char *text)
{
	return md_buf_append(buf, text, strlen(text));
}

int md_buf_vprintf(struct md_buf *buf, const char *fmt, va_list ap)
{
	va_list copy;
	int n;
	int rc;
	size_t avail;

	while (1) {
		rc = md_buf_reserve(buf, buf->len + 128u);
		if (rc != MD_OK)
			return rc;
		avail = buf->cap - buf->len;

		va_copy(copy, ap);
		n = vsnprintf(buf->data + buf->len, avail, fmt, copy);
		va_end(copy);
		if (n < 0)
			return MD_ERR_INVALID;
		if ((size_t)n < avail) {
			buf->len += (size_t)n;
			return MD_OK;
		}
		rc = md_buf_reserve(buf, buf->len + (size_t)n + 1u);
		if (rc != MD_OK)
			return rc;
	}
}

int md_buf_printf(struct md_buf *buf, const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = md_buf_vprintf(buf, fmt, ap);
	va_end(ap);
	return rc;
}

char *md_buf_detach(struct md_buf *buf)
{
	char *out;

	if (!buf->data) {
		out = malloc(1u);
		if (out)
			out[0] = '\0';
		return out;
	}

	out = buf->data;
	md_buf_init(buf);
	return out;
}
