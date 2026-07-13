#include "base/md_utf8.h"

static int utf8_cont(unsigned char c)
{
	return (c & 0xc0u) == 0x80u;
}

static size_t utf8_seq_len(unsigned char c)
{
	if (c < 0x80u)
		return 1u;
	if ((c & 0xe0u) == 0xc0u)
		return 2u;
	if ((c & 0xf0u) == 0xe0u)
		return 3u;
	if ((c & 0xf8u) == 0xf0u)
		return 4u;
	return 1u;
}

size_t md_utf8_complete_prefix_len(const char *data, size_t len)
{
	size_t start;
	size_t need;
	size_t i;

	if (!data || len == 0)
		return 0;

	start = len - 1u;
	while (start > 0 && utf8_cont((unsigned char)data[start]))
		start--;

	need = utf8_seq_len((unsigned char)data[start]);
	if (need == 1u)
		return len;
	if (len - start < need)
		return start;

	for (i = start + 1u; i < start + need; i++) {
		if (!utf8_cont((unsigned char)data[i]))
			return len;
	}
	return len;
}
