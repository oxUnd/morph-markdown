#include "base/md_table_layout.h"
#include "base/md_width.h"

#include <assert.h>
#include <string.h>

static void test_compact_preferred_widths(void)
{
	const struct md_table_column_constraint columns[] = {
		{ 1u, 3u },
		{ 2u, 5u }
	};
	unsigned int widths[2] = { 0u, 0u };

	assert(md_table_size_columns(columns, 2u, 20u, widths) == 0);
	assert(widths[0] == 3u);
	assert(widths[1] == 5u);
}

static void test_flexible_width_distribution(void)
{
	const struct md_table_column_constraint columns[] = {
		{ 2u, 8u },
		{ 2u, 4u }
	};
	unsigned int widths[2] = { 0u, 0u };

	assert(md_table_size_columns(columns, 2u, 8u, widths) == 0);
	assert(widths[0] == 5u);
	assert(widths[1] == 3u);
}

static void test_minimum_width_overflow(void)
{
	const struct md_table_column_constraint columns[] = {
		{ 5u, 9u },
		{ 2u, 6u }
	};
	unsigned int widths[2] = { 0u, 0u };

	assert(md_table_size_columns(columns, 2u, 4u, widths) == 0);
	assert(widths[0] == 5u);
	assert(widths[1] == 2u);
}

static void test_rounding_is_deterministic(void)
{
	const struct md_table_column_constraint columns[] = {
		{ 1u, 3u },
		{ 1u, 3u },
		{ 1u, 3u }
	};
	unsigned int widths[3] = { 0u, 0u, 0u };

	assert(md_table_size_columns(columns, 3u, 5u, widths) == 0);
	assert(widths[0] == 2u);
	assert(widths[1] == 2u);
	assert(widths[2] == 1u);
}

static void test_invalid_preferred_width_is_clamped(void)
{
	const struct md_table_column_constraint columns[] = {
		{ 4u, 2u }
	};
	unsigned int width = 0u;

	assert(md_table_size_columns(columns, 1u, 20u, &width) == 0);
	assert(width == 4u);
	assert(md_table_size_columns(NULL, 0u, 0u, NULL) == 0);
}

static void test_grapheme_widths_are_terminal_safe(void)
{
	const char combining[] = "e\xcc\x81";
	const char emoji[] = "\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x92\xbb";
	const char heart[] = "\xe2\x9d\xa4\xef\xb8\x8f";

	assert(md_utf8_grapheme_len(combining, strlen(combining)) ==
	       strlen(combining));
	assert(md_utf8_display_width(combining) == 1);
	assert(md_utf8_grapheme_len(emoji, strlen(emoji)) == strlen(emoji));
	assert(md_utf8_display_width(emoji) == 2);
	assert(md_utf8_grapheme_len(heart, strlen(heart)) == strlen(heart));
	assert(md_utf8_display_width(heart) == 2);
}

int main(void)
{
	test_compact_preferred_widths();
	test_flexible_width_distribution();
	test_minimum_width_overflow();
	test_rounding_is_deterministic();
	test_invalid_preferred_width_is_clamped();
	test_grapheme_widths_are_terminal_safe();
	return 0;
}
