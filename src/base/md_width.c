#include "base/md_width.h"

static size_t decode_utf8(const unsigned char *s, unsigned int *cp)
{
	if (s[0] < 0x80u) {
		*cp = s[0];
		return 1u;
	}
	if ((s[0] & 0xe0u) == 0xc0u) {
		*cp = ((unsigned int)(s[0] & 0x1fu) << 6) |
		      (unsigned int)(s[1] & 0x3fu);
		return 2u;
	}
	if ((s[0] & 0xf0u) == 0xe0u) {
		*cp = ((unsigned int)(s[0] & 0x0fu) << 12) |
		      ((unsigned int)(s[1] & 0x3fu) << 6) |
		      (unsigned int)(s[2] & 0x3fu);
		return 3u;
	}
	if ((s[0] & 0xf8u) == 0xf0u) {
		*cp = ((unsigned int)(s[0] & 0x07u) << 18) |
		      ((unsigned int)(s[1] & 0x3fu) << 12) |
		      ((unsigned int)(s[2] & 0x3fu) << 6) |
		      (unsigned int)(s[3] & 0x3fu);
		return 4u;
	}
	*cp = s[0];
	return 1u;
}

static int is_wide(unsigned int cp)
{
	return (cp >= 0x1100u && cp <= 0x115fu) ||
	       (cp >= 0x2329u && cp <= 0x232au) ||
	       (cp >= 0x2e80u && cp <= 0xa4cfu) ||
	       (cp >= 0xac00u && cp <= 0xd7a3u) ||
	       (cp >= 0xf900u && cp <= 0xfaffu) ||
	       (cp >= 0xfe10u && cp <= 0xfe19u) ||
	       (cp >= 0xfe30u && cp <= 0xfe6fu) ||
	       (cp >= 0xff00u && cp <= 0xff60u) ||
	       (cp >= 0xffe0u && cp <= 0xffe6u) ||
	       (cp >= 0x1f000u && cp <= 0x1faffu);
}

static int is_zero_width(unsigned int cp)
{
	return (cp >= 0x0300u && cp <= 0x036fu) ||
	       (cp >= 0x1ab0u && cp <= 0x1affu) ||
	       (cp >= 0x1dc0u && cp <= 0x1dffu) ||
	       (cp >= 0x20d0u && cp <= 0x20ffu) ||
	       (cp >= 0xfe20u && cp <= 0xfe2fu);
}

int md_utf8_display_width_n(const char *text, size_t len)
{
	const unsigned char *s;
	size_t i;
	size_t step;
	unsigned int cp;
	int width;

	if (!text)
		return 0;

	s = (const unsigned char *)text;
	i = 0;
	width = 0;
	while (i < len && s[i]) {
		step = decode_utf8(s + i, &cp);
		if (i + step > len)
			break;
		if (cp == '\n' || cp == '\r' || cp == '\t') {
			width += 1;
		} else if (!is_zero_width(cp)) {
			width += is_wide(cp) ? 2 : 1;
		}
		i += step;
	}
	return width;
}

int md_utf8_display_width(const char *text)
{
	size_t len;

	if (!text)
		return 0;
	for (len = 0; text[len]; len++)
		;
	return md_utf8_display_width_n(text, len);
}
