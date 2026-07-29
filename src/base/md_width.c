#include "base/md_width.h"

static int utf8_cont(unsigned char value)
{
	return (value & 0xc0u) == 0x80u;
}

static size_t decode_utf8(const unsigned char *s, size_t len,
			  unsigned int *cp)
{
	if (len == 0u)
		return 0u;
	if (s[0] < 0x80u) {
		*cp = s[0];
		return 1u;
	}
	if ((s[0] & 0xe0u) == 0xc0u && len >= 2u && utf8_cont(s[1])) {
		*cp = ((unsigned int)(s[0] & 0x1fu) << 6) |
		      (unsigned int)(s[1] & 0x3fu);
		return 2u;
	}
	if ((s[0] & 0xf0u) == 0xe0u && len >= 3u &&
	    utf8_cont(s[1]) && utf8_cont(s[2])) {
		*cp = ((unsigned int)(s[0] & 0x0fu) << 12) |
		      ((unsigned int)(s[1] & 0x3fu) << 6) |
		      (unsigned int)(s[2] & 0x3fu);
		return 3u;
	}
	if ((s[0] & 0xf8u) == 0xf0u && len >= 4u &&
	    utf8_cont(s[1]) && utf8_cont(s[2]) && utf8_cont(s[3])) {
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

struct codepoint_range {
	unsigned int first;
	unsigned int last;
};

static int is_default_emoji(unsigned int cp)
{
	static const struct codepoint_range ranges[] = {
		{ 0x231au, 0x231bu }, { 0x23e9u, 0x23ecu },
		{ 0x23f0u, 0x23f0u }, { 0x23f3u, 0x23f3u },
		{ 0x25fdu, 0x25feu }, { 0x2614u, 0x2615u },
		{ 0x2648u, 0x2653u }, { 0x267fu, 0x267fu },
		{ 0x2693u, 0x2693u }, { 0x26a1u, 0x26a1u },
		{ 0x26aau, 0x26abu }, { 0x26bdu, 0x26beu },
		{ 0x26c4u, 0x26c5u }, { 0x26ceu, 0x26ceu },
		{ 0x26d4u, 0x26d4u }, { 0x26eau, 0x26eau },
		{ 0x26f2u, 0x26f3u }, { 0x26f5u, 0x26f5u },
		{ 0x26fau, 0x26fau }, { 0x26fdu, 0x26fdu },
		{ 0x2705u, 0x2705u }, { 0x270au, 0x270bu },
		{ 0x2728u, 0x2728u }, { 0x274cu, 0x274cu },
		{ 0x274eu, 0x274eu }, { 0x2753u, 0x2755u },
		{ 0x2757u, 0x2757u }, { 0x2795u, 0x2797u },
		{ 0x27b0u, 0x27b0u }, { 0x27bfu, 0x27bfu },
		{ 0x2b1bu, 0x2b1cu }, { 0x2b50u, 0x2b50u },
		{ 0x2b55u, 0x2b55u }
	};
	size_t low = 0u;
	size_t high = sizeof(ranges) / sizeof(ranges[0]);

	while (low < high) {
		size_t middle = low + (high - low) / 2u;

		if (cp < ranges[middle].first)
			high = middle;
		else if (cp > ranges[middle].last)
			low = middle + 1u;
		else
			return 1;
	}
	return cp >= 0x1f000u && cp <= 0x1faffu;
}

static int is_zero_width(unsigned int cp)
{
	return (cp >= 0x0300u && cp <= 0x036fu) ||
	       (cp >= 0x1ab0u && cp <= 0x1affu) ||
	       (cp >= 0x1dc0u && cp <= 0x1dffu) ||
	       (cp >= 0x20d0u && cp <= 0x20ffu) ||
	       (cp >= 0xfe20u && cp <= 0xfe2fu) ||
	       (cp >= 0xfe00u && cp <= 0xfe0fu) ||
	       (cp >= 0xe0100u && cp <= 0xe01efu) ||
	       (cp >= 0xe0020u && cp <= 0xe007fu) ||
	       (cp >= 0x1f3fbu && cp <= 0x1f3ffu) ||
	       cp == 0x200du;
}

static int is_regional_indicator(unsigned int cp)
{
	return cp >= 0x1f1e6u && cp <= 0x1f1ffu;
}

static int is_cjk(unsigned int cp)
{
	return (cp >= 0x2e80u && cp <= 0x9fffu) ||
	       (cp >= 0xac00u && cp <= 0xd7a3u) ||
	       (cp >= 0xf900u && cp <= 0xfaffu) ||
	       (cp >= 0x30000u && cp <= 0x323afu);
}

static int is_open_punctuation(unsigned int cp)
{
	return cp == '(' || cp == '[' || cp == '{' || cp == '<' ||
	       cp == 0x2018u || cp == 0x201cu || cp == 0x3008u ||
	       cp == 0x300au || cp == 0x300cu || cp == 0x300eu ||
	       cp == 0x3010u || cp == 0x3014u || cp == 0xff08u;
}

static int is_close_punctuation(unsigned int cp)
{
	return cp == ')' || cp == ']' || cp == '}' || cp == '>' ||
	       cp == ',' || cp == '.' || cp == ':' || cp == ';' ||
	       cp == '!' || cp == '?' || cp == 0x2019u || cp == 0x201du ||
	       cp == 0x3001u || cp == 0x3002u || cp == 0x3009u ||
	       cp == 0x300bu || cp == 0x300du || cp == 0x300fu ||
	       cp == 0x3011u || cp == 0x3015u || cp == 0xff01u ||
	       cp == 0xff09u || cp == 0xff0cu || cp == 0xff0eu ||
	       cp == 0xff1au || cp == 0xff1bu || cp == 0xff1fu;
}

size_t md_utf8_grapheme_len(const char *text, size_t len)
{
	const unsigned char *bytes = (const unsigned char *)text;
	unsigned int first;
	unsigned int cp;
	size_t offset;
	size_t step;
	int join_next = 0;

	if (!text || len == 0u)
		return 0u;
	offset = decode_utf8(bytes, len, &first);
	while (offset < len) {
		step = decode_utf8(bytes + offset, len - offset, &cp);
		if (join_next) {
			offset += step;
			join_next = 0;
			continue;
		}
		if (cp == 0x200du) {
			offset += step;
			join_next = 1;
			continue;
		}
		if (is_zero_width(cp)) {
			offset += step;
			continue;
		}
		if (is_regional_indicator(first) &&
		    is_regional_indicator(cp))
			offset += step;
		break;
	}
	return offset;
}

int md_utf8_grapheme_width_n(const char *text, size_t len)
{
	const unsigned char *bytes = (const unsigned char *)text;
	unsigned int cp;
	size_t offset = 0u;
	size_t step;
	int width = 0;
	int emoji = 0;

	while (offset < len) {
		step = decode_utf8(bytes + offset, len - offset, &cp);
		if (cp == 0xfe0fu || cp == 0x200du || cp == 0x20e3u ||
		    is_default_emoji(cp))
			emoji = 1;
		if (!is_zero_width(cp)) {
			int current = is_wide(cp) ? 2 : 1;

			if (current > width)
				width = current;
		}
		offset += step;
	}
	return emoji && width > 0 && width < 2 ? 2 : width;
}

int md_utf8_is_space_n(const char *text, size_t len)
{
	unsigned int cp;

	if (!text || decode_utf8((const unsigned char *)text, len, &cp) == 0u)
		return 0;
	return cp == ' ' || cp == '\t' || cp == 0x00a0u ||
	       cp == 0x3000u;
}

int md_utf8_break_allowed_between(const char *left, size_t left_len,
				  const char *right, size_t right_len)
{
	unsigned int a;
	unsigned int b;

	if (!left || !right ||
	    decode_utf8((const unsigned char *)left, left_len, &a) == 0u ||
	    decode_utf8((const unsigned char *)right, right_len, &b) == 0u)
		return 0;
	if (md_utf8_is_space_n(left, left_len))
		return 1;
	if (is_open_punctuation(a) || is_close_punctuation(b))
		return 0;
	if (a == '-' || a == '/' || a == 0x2010u || a == 0x2013u)
		return 1;
	return is_cjk(a) || is_cjk(b);
}

int md_utf8_display_width_n(const char *text, size_t len)
{
	size_t i;
	size_t step;
	int width;

	if (!text)
		return 0;
	i = 0;
	width = 0;
	while (i < len && text[i]) {
		step = md_utf8_grapheme_len(text + i, len - i);
		if (text[i] == '\n' || text[i] == '\r' || text[i] == '\t') {
			width += 1;
		} else {
			width += md_utf8_grapheme_width_n(text + i, step);
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
