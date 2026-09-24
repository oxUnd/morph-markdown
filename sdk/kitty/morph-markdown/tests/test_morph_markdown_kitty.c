#include "morph_markdown_kitty.h"
#include "base/md_width.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef MORPH_TEST_MATH_FONT_PATH
#define MORPH_TEST_MATH_FONT_PATH "fonts/STIXTwoMath-Regular.ttf"
#endif

struct capture {
	char bytes[262144];
	size_t len;
};

struct media_capture {
	char type[16];
	char path[256];
	int count;
};

static int capture_write(const char *bytes, size_t len, void *user_data)
{
	struct capture *capture = user_data;

	if (len > sizeof(capture->bytes) - capture->len)
		return -1;
	memcpy(capture->bytes + capture->len, bytes, len);
	capture->len += len;
	return 0;
}

static void capture_media(const char *type, const char *path, void *user_data)
{
	struct media_capture *capture = user_data;

	snprintf(capture->type, sizeof(capture->type), "%s", type);
	snprintf(capture->path, sizeof(capture->path), "%s", path);
	capture->count++;
}

static void capture_ordered_media(const char *type, const char *path,
				  void *user_data)
{
	struct capture *capture = user_data;
	char marker[320];
	int len;

	len = snprintf(marker, sizeof(marker), "<%s:%s>", type, path);
	assert(len > 0);
	assert((size_t)len < sizeof(marker));
	assert(capture_write(marker, (size_t)len, capture) == 0);
}

static void capture_reset(struct capture *capture)
{
	memset(capture, 0, sizeof(*capture));
}

static unsigned int max_kitty_image_height(const char *output)
{
	const char *command = output;
	unsigned int height;
	unsigned int maximum = 0u;

	while ((command = strstr(command, "\033_Ga=T,f=32,s=")) != NULL) {
		if (sscanf(command, "\033_Ga=T,f=32,s=%*u,v=%u", &height) == 1 &&
		    height > maximum)
			maximum = height;
		command++;
	}
	return maximum;
}

static int write_test_png(const char *path)
{
	static const unsigned char png[] = {
		0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
		0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
		0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
		0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4,
		0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41,
		0x54, 0x78, 0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0xf0,
		0x1f, 0x00, 0x05, 0x00, 0x01, 0xff, 0x89, 0x99,
		0x3d, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
		0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
	};
	FILE *file = fopen(path, "wb");
	size_t written;

	if (!file)
		return -1;
	written = fwrite(png, 1u, sizeof(png), file);
	if (fclose(file) != 0)
		return -1;
	return written == sizeof(png) ? 0 : -1;
}

static int substring_count(const char *text, const char *needle)
{
	int count = 0;
	size_t len = strlen(needle);

	while ((text = strstr(text, needle)) != NULL) {
		count++;
		text += len;
	}
	return count;
}

static const char *next_kitty_image_id(const char *output,
				       unsigned int *image_id)
{
	const char *command;
	const char *id;

	command = strstr(output, "\033_Ga=T,");
	if (!command)
		return NULL;
	id = strstr(command, ",i=");
	if (!id || sscanf(id, ",i=%u", image_id) != 1)
		return NULL;
	return id + 3u;
}

static void test_stream_render_and_final(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *first =
		"# SDK\n\nA **bold** value.\n\n"
		"3. ordered\n4. next\n"
		"   - nested\n\n"
		"- [x] done\n- [ ] pending\n\n";
	const char *second = "| name | value |\n|---|---|\n| kitty | yes |\n";

	memset(&options, 0, sizeof(options));
	memset(&output, 0, sizeof(output));
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(renderer, first, strlen(first), 0) == 0);
	assert(morph_md_kitty_append(renderer, second, strlen(second), 1) == 0);
	assert(morph_md_kitty_append(renderer, "late", 4u, 0) != 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "\033[1;4;38;5;81mSDK\033[0m") != NULL);
	assert(strstr(output.bytes, "\033[1mbold\033[0m") != NULL);
	assert(strstr(output.bytes, "3. ordered\n4. next\n") != NULL);
	assert(strstr(output.bytes, "   ◦ nested\n") != NULL);
	assert(strstr(output.bytes, "☑ done\n☐ pending\n") != NULL);
	assert(strstr(output.bytes, "ordered\n\n4.") == NULL);
	assert(strstr(output.bytes, "│ kitty │ yes") != NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_clear_sequence(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;

	memset(&options, 0, sizeof(options));
	memset(&output, 0, sizeof(output));
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_end_frame(renderer) != 0);
	assert(morph_md_kitty_begin_frame(renderer) == 0);
	assert(morph_md_kitty_clear(renderer) == 0);
	assert(morph_md_kitty_write_text(renderer, "status\n", 7u) == 0);
	assert(output.len == 0u);
	assert(morph_md_kitty_end_frame(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "\033[?2026h") != NULL);
	assert(strstr(output.bytes, "\033_Ga=d,d=A,q=2\033\\") != NULL);
	assert(strstr(output.bytes, "\033[H\033[2J") != NULL);
	assert(strstr(output.bytes, "status\n") != NULL);
	assert(strstr(output.bytes, "\033[?2026l") != NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_incremental_render_preserves_scrollback(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 40u;
	options.terminal_rows = 12u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);

	assert(morph_md_kitty_append(renderer, "first", 5u, 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len == 0u);

	assert(morph_md_kitty_append(renderer, " line\n", 6u, 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len == 0u);

	capture_reset(&output);
	assert(morph_md_kitty_append(renderer, "second line\n", 12u, 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len == 0u);

	capture_reset(&output);
	assert(morph_md_kitty_append(renderer, NULL, 0u, 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "first line") != NULL);
	assert(strstr(output.bytes, "second line") != NULL);
	assert(strstr(output.bytes, "\033[A") == NULL);
	assert(strstr(output.bytes, "\033[J") == NULL);
	assert(strstr(output.bytes, "\033[H") == NULL);
	assert(strstr(output.bytes, "\033[2J") == NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_streaming_table_stays_in_live_tail(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 60u;
	options.terminal_rows = 24u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);

	assert(morph_md_kitty_append(renderer, "| a | b |\n", 10u, 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len == 0u);
	capture_reset(&output);
	assert(morph_md_kitty_append(renderer, "|---|---|\n", 10u, 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len == 0u);

	capture_reset(&output);
	assert(morph_md_kitty_append(renderer, "| 1 | 2 |\n", 10u, 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "│ 1 │ 2 │") != NULL);
	assert(strstr(output.bytes, "\033[A") == NULL);
	assert(strstr(output.bytes, "\033[J") == NULL);
	assert(strstr(output.bytes, "\033[H") == NULL);
	assert(strstr(output.bytes, "\033[2J") == NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_table_wraps_to_viewport(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown =
		"| id | description |\n"
		"|---|---|\n"
		"| 1 | alpha beta gamma delta |\n";

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 24u;
	options.content_padding_right_columns = 2u;
	options.content_padding_left_columns = 2u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "  │ id │ description │\n") != NULL);
	assert(strstr(output.bytes,
		      "  │ 1  │ alpha beta  │\n"
		      "  │    │ gamma delta │\n") != NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_table_cjk_and_long_word_wrapping(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown =
		"| 内容 |\n"
		"|---|\n"
		"| 中文，测试。 |\n"
		"| abcdefghijk |\n";

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 12u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "│ 中文，测 │\n│ 试。     │\n") != NULL);
	assert(strstr(output.bytes, "│ abcdefgh │\n│ ijk      │\n") != NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_table_code_is_atomic_and_tabs_are_stable(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown =
		"| value |\n"
		"|---|\n"
		"| `abcdefghij` |\n"
		"| a\tb |\n";

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 12u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes,
		      "│ \033[2mabcdefghij\033[22m │\n") != NULL);
	assert(strstr(output.bytes, "`abcdefghij`") == NULL);
	assert(strstr(output.bytes, "│ a    b     │\n") != NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_inline_code_uses_terminal_style(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown = "Run `morph --help` now.\n";

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 80u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes,
		      "Run \033[2mmorph --help\033[22m now.") != NULL);
	assert(strstr(output.bytes, "`morph --help`") == NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_inline_code_wraps_with_content_padding(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown = "abcd `01234567890123456789` end";

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 16u;
	options.content_padding_left_columns = 4u;
	options.content_padding_right_columns = 2u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes,
		"\033[2m01234\n    5678901234\n    56789\033[22m") != NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_table_default_emoji_widths(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown =
		"| identity | status |\n"
		"|---|---|\n"
		"| Bot | ✅ ready |\n"
		"| User | ❌ failed |\n";

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 40u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "│ Bot      │ ✅ ready  │") != NULL);
	assert(strstr(output.bytes, "│ User     │ ❌ failed │") != NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_links_show_destination_in_text_and_tables(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown =
		"[site](https://example.com)\n\n"
		"| link |\n"
		"|---|\n"
		"| [site](https://example.com) |\n";

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 60u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes,
		      "site (https://example.com)\n\n") != NULL);
	assert(strstr(output.bytes,
		      "│ site (https://example.com) │\n") != NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_content_padding_and_wrapping(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown = "alpha beta gamma";

	memset(&options, 0, sizeof(options));
	memset(&output, 0, sizeof(output));
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 16u;
	options.content_padding_top_rows = 1u;
	options.content_padding_right_columns = 3u;
	options.content_padding_bottom_rows = 2u;
	options.content_padding_left_columns = 2u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strcmp(output.bytes,
		      "\033[?2026h\n  alpha beta \n  gamma\n\n\n\n"
		      "\033[?2026l") == 0);
	morph_md_kitty_destroy(renderer);
}

static void test_initial_cursor_column_and_wrapping(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown = "alpha beta gamma";

	memset(&options, 0, sizeof(options));
	memset(&output, 0, sizeof(output));
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 16u;
	options.content_padding_right_columns = 3u;
	options.content_padding_left_columns = 2u;
	options.initial_cursor_column = 2u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strcmp(output.bytes,
		      "\033[?2026halpha beta \n  gamma\n\n"
		      "\033[?2026l") == 0);
	morph_md_kitty_destroy(renderer);
}

static void test_initial_cursor_column_preserves_prefix_on_refresh(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 40u;
	options.terminal_rows = 12u;
	options.content_padding_left_columns = 2u;
	options.initial_cursor_column = 2u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);

	assert(morph_md_kitty_append(renderer, "first line\n\n", 12u, 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	capture_reset(&output);
	assert(morph_md_kitty_append(renderer, "second line\n\n", 13u, 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "second line") != NULL);
	assert(strstr(output.bytes, "\033[J") == NULL);
	assert(strstr(output.bytes, "\033[1A\r\033[J") == NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_initial_cursor_column_refreshes_later_rows_from_margin(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *first = "stable line\n\nmutable line\n";
	const char *second = "next line\n\n";

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 40u;
	options.terminal_rows = 12u;
	options.content_padding_left_columns = 2u;
	options.initial_cursor_column = 2u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);

	assert(morph_md_kitty_append(renderer, first, strlen(first), 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	capture_reset(&output);
	assert(morph_md_kitty_append(renderer, second, strlen(second), 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "next line") != NULL);
	assert(strstr(output.bytes, "\033[J") == NULL);
	assert(strstr(output.bytes, "\033[3G\033[J") == NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_heading_underline_excludes_left_padding(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown = "# Title";

	memset(&options, 0, sizeof(options));
	memset(&output, 0, sizeof(output));
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.content_padding_left_columns = 4u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "    \033[1;4;38;5;81mTitle") != NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_local_png_bypasses_media_callback(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	struct media_capture media;
	char path[128];
	char markdown[256];

	snprintf(path, sizeof(path),
		 "/tmp/morph-markdown-kitty-callback-%ld.png", (long)getpid());
	assert(write_test_png(path) == 0);
	snprintf(markdown, sizeof(markdown), "![plot](file://%s)", path);
	memset(&options, 0, sizeof(options));
	memset(&output, 0, sizeof(output));
	memset(&media, 0, sizeof(media));
	options.write = capture_write;
	options.user_data = &output;
	options.media = capture_media;
	options.media_user_data = &media;
	options.terminal_fd = -1;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(media.count == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "\033_Ga=T,f=100,") != NULL);
	morph_md_kitty_destroy(renderer);
	assert(unlink(path) == 0);
}

static void test_media_callbacks_follow_document_order(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown =
		"before\n\n"
		"![plot](file:///tmp/plot.jpg)\n\n"
		"between\n\n"
		"[clip](file:///tmp/demo.mp4)\n\n"
		"after";
	const char *before;
	const char *image;
	const char *between;
	const char *video;
	const char *after;

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.media = capture_ordered_media;
	options.media_user_data = &output;
	options.terminal_fd = -1;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	before = strstr(output.bytes, "before");
	image = strstr(output.bytes, "<image:/tmp/plot.jpg>");
	between = strstr(output.bytes, "between");
	video = strstr(output.bytes, "<video:/tmp/demo.mp4>");
	after = strstr(output.bytes, "after");
	assert(before && image && between && video && after);
	assert(before < image);
	assert(image < between);
	assert(between < video);
	assert(video < after);
	assert(strstr(output.bytes, "[image: ") == NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_streaming_media_callbacks_emit_once_in_place(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *first =
		"before\n\n"
		"![plot](file:///tmp/stream.jpg)\n\n"
		"[clip](file:///tmp/stream.mp4)\n\n";
	const char *second = "after";
	const char *image;
	const char *video;
	const char *after;

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.media = capture_ordered_media;
	options.media_user_data = &output;
	options.terminal_fd = -1;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, first, strlen(first), 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(morph_md_kitty_append(
		       renderer, second, strlen(second), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	image = strstr(output.bytes, "<image:/tmp/stream.jpg>");
	video = strstr(output.bytes, "<video:/tmp/stream.mp4>");
	after = strstr(output.bytes, "after");
	assert(image && video && after);
	assert(image < video);
	assert(video < after);
	assert(substring_count(
		output.bytes, "<image:/tmp/stream.jpg>") == 1);
	assert(substring_count(
		output.bytes, "<video:/tmp/stream.mp4>") == 1);
	morph_md_kitty_destroy(renderer);
}

static void test_local_png_renders_in_blocks_and_tables(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	char path[128];
	char markdown[512];
	const char *next;
	unsigned int first_image_id;
	unsigned int second_image_id;

	snprintf(path, sizeof(path),
		 "/tmp/morph-markdown-kitty-test-%ld.png", (long)getpid());
	assert(write_test_png(path) == 0);
	snprintf(markdown, sizeof(markdown),
		 "![block](file://%s)\n\n"
		 "| image |\n"
		 "|---|\n"
		 "| ![cell](file://%s) |\n",
		 path, path);
	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 40u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(substring_count(output.bytes, "\033_Ga=T,f=100,") == 2);
	assert(strstr(output.bytes, ",U=1,q=2,c=1,r=1,m=0;") != NULL);
	assert(strstr(output.bytes, "\364\216\273\256") != NULL);
	assert(strstr(output.bytes, "[image:") == NULL);
	next = next_kitty_image_id(output.bytes, &first_image_id);
	assert(next != NULL);
	next = next_kitty_image_id(next, &second_image_id);
	assert(next != NULL);
	assert((first_image_id & 0xff000000u) == 0x53000000u);
	assert((second_image_id & 0xff000000u) == 0x53000000u);
	assert(first_image_id != second_image_id);
	morph_md_kitty_destroy(renderer);
	assert(unlink(path) == 0);
}

static void test_streaming_image_is_transmitted_once(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	char path[128];
	char markdown[256];
	unsigned int image_id = 0u;

	snprintf(path, sizeof(path),
		 "/tmp/morph-markdown-kitty-stream-%ld.png", (long)getpid());
	assert(write_test_png(path) == 0);
	snprintf(markdown, sizeof(markdown),
		 "![block](file://%s)\n\n", path);
	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 40u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);

	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(substring_count(output.bytes, "\033_Ga=T,f=100,") == 1);
	assert(sscanf(strstr(output.bytes, ",i="), ",i=%u", &image_id) == 1);
	assert(image_id != 0u);
	assert(strstr(output.bytes, "\364\216\273\256") != NULL);

	capture_reset(&output);
	assert(morph_md_kitty_append(renderer, "next line\n\n", 11u, 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "next line") != NULL);
	assert(strstr(output.bytes, "\033_Ga=T,f=100,") == NULL);
	assert(strstr(output.bytes, "\033[A") == NULL);
	assert(strstr(output.bytes, "\033[J") == NULL);

	capture_reset(&output);
	assert(morph_md_kitty_append(renderer, NULL, 0u, 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "\033_Ga=T,f=100,") == NULL);
	morph_md_kitty_destroy(renderer);
	assert(unlink(path) == 0);
}

/* Follow cursor movement and check every physical table row, including rows
 * where text or a shorter visual contributes only blank padding. */
static void assert_table_border_columns(const char *output)
{
	const char *cursor = output;
	const char *end;
	unsigned int column = 0u;
	unsigned int expected[3] = {0u};
	unsigned int border = 0u;
	unsigned int rows = 0u;
	unsigned int advance;
	size_t len;

	while (*cursor) {
		if (strncmp(cursor, "\0337", 2u) == 0) {
			end = strstr(cursor + 2u, "\0338");
			assert(end != NULL);
			cursor = end + 2u;
			continue;
		}
		if (strncmp(cursor, "\033_G", 3u) == 0) {
			end = strstr(cursor + 3u, "\033\\");
			assert(end != NULL);
			cursor = end + 2u;
			continue;
		}
		if (strncmp(cursor, "\033[", 2u) == 0) {
			end = cursor + 2u;
			while (*end && (*end < '@' || *end > '~'))
				end++;
			assert(*end);
			if (*end == 'C') {
				assert(sscanf(cursor + 2u, "%u", &advance) == 1);
				column += advance;
			}
			cursor = end + 1u;
			continue;
		}
		if (*cursor == '\n') {
			if (border) {
				assert(border == 3u);
				rows++;
			}
			border = 0u;
			column = 0u;
			cursor++;
			continue;
		}
		if (strncmp(cursor, "│", strlen("│")) == 0) {
			assert(border < 3u);
			if (rows == 0u)
				expected[border] = column;
			else
				assert(expected[border] == column);
			border++;
		}
		len = md_utf8_grapheme_len(cursor, strlen(cursor));
		assert(len > 0u);
		column += (unsigned int)md_utf8_display_width_n(cursor, len);
		cursor += len;
	}
	assert(rows > 2u);
}

static void test_table_text_is_centered_beside_tall_formula(void)
{
	static const char placeholder[] = "\364\216\273\256";
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	char path[128];
	char markdown[1024];
	const char *format =
		"| label | content |\n"
		"|---|---|\n"
		"| formula | before $\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}$ after |\n"
		"| mixed | 中文 `code` $x$ ![small](%s) "
		"$\\frac{1}{\\frac{1}{x}}$ 后文 |\n";
	const char *first_placeholder;
	const char *last_placeholder;
	const char *before;
	const char *cursor;

	snprintf(path, sizeof(path), "/tmp/morph-kitty-mixed-%ld.png",
		 (long)getpid());
	assert(write_test_png(path) == 0);
	snprintf(markdown, sizeof(markdown), format, path);
	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM | MORPH_MD_FEATURE_MATH;
	options.font_path = MORPH_TEST_MATH_FONT_PATH;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 100u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	first_placeholder = strstr(output.bytes, placeholder);
	assert(first_placeholder != NULL);
	last_placeholder = first_placeholder;
	for (cursor = first_placeholder + strlen(placeholder);
	     (cursor = strstr(cursor, placeholder)) != NULL;
	     cursor += strlen(placeholder))
		last_placeholder = cursor;
	before = strstr(output.bytes, "before");
	assert(before != NULL);
	assert(first_placeholder < before);
	assert(before < last_placeholder);
	assert_table_border_columns(output.bytes);
	morph_md_kitty_destroy(renderer);
	assert(unlink(path) == 0);
}

static void test_math_uses_native_size_kitty_transfer(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown =
		"123456789 $x^2$ math\n\n"
		"| formula | status |\n"
		"|---|---|\n"
		"| $x^2$ | rendered |\n"
		"| $\\begin{aligned}a&=1 \\\\ b&=2 \\\\ c&=3\\end{aligned}$ | three rows |\n";

	memset(&options, 0, sizeof(options));
	memset(&output, 0, sizeof(output));
	options.font_path = MORPH_TEST_MATH_FONT_PATH;
	options.features = MORPH_MD_FEATURE_GFM | MORPH_MD_FEATURE_MATH;
	options.fg_color = 0xFFFFFFFFu;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 16u;
	options.content_padding_right_columns = 2u;
	options.content_padding_left_columns = 2u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "\033_Ga=T,f=32,s=") != NULL);
	assert(strstr(output.bytes,
		      "  123456789 \n  \033_Ga=T,f=32,s=") != NULL);
	assert(strstr(output.bytes, ",U=1,q=2,c=") != NULL);
	assert(strstr(output.bytes, "\364\216\273\256") != NULL);
	assert(strstr(output.bytes, "│ $x^2$ ") == NULL);
	assert(max_kitty_image_height(output.bytes) > 40u);
	morph_md_kitty_destroy(renderer);
}

static void test_streaming_math_is_append_only(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *first =
		"| formula | value |\n"
		"|---|---|\n"
		"| $x^2$ | one |\n";
	const char *second = "| $y^2$ | two |\n";

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.font_path = MORPH_TEST_MATH_FONT_PATH;
	options.features = MORPH_MD_FEATURE_GFM | MORPH_MD_FEATURE_MATH;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 60u;
	options.terminal_rows = 24u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(renderer, first, strlen(first), 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len == 0u);

	capture_reset(&output);
	assert(morph_md_kitty_append(renderer, second, strlen(second), 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len == 0u);

	assert(morph_md_kitty_append(renderer, NULL, 0u, 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes, "\033_Ga=T,f=32,s=") != NULL);
	assert(strstr(output.bytes, ",U=1,") != NULL);
	assert(strstr(output.bytes, "\033_Ga=d,d=I,i=") == NULL);
	assert(strstr(output.bytes, "\033_Ga=d,d=A") == NULL);
	assert(strstr(output.bytes, "\033[A") == NULL);
	assert(strstr(output.bytes, "\033[J") == NULL);
	assert(strstr(output.bytes, "\033[H") == NULL);
	assert(strstr(output.bytes, "\033[2J") == NULL);
	morph_md_kitty_destroy(renderer);
}

static void test_fenced_code_uses_syntax_highlighting(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	const char *markdown =
		"> quote before code\n\n"
		"```c\n"
		"int main(void) { return 42; }\n"
		"```\n";

	memset(&options, 0, sizeof(options));
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 80u;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(
		       renderer, markdown, strlen(markdown), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(output.len < sizeof(output.bytes));
	output.bytes[output.len] = '\0';
	assert(strstr(output.bytes,
		      "quote before code\n\n"
		      "\033[2;38;5;244m──") != NULL);
	assert(strstr(output.bytes, "quote before code\n\n\n") == NULL);
	assert(strstr(output.bytes,
		      "\033[2;38;5;244m──"
		      "\033[1;38;5;75m c "
		      "\033[2;38;5;244m─") != NULL);
	assert(strstr(output.bytes,
		      "\n\033[36mint") != NULL);
	assert(strstr(output.bytes,
		      "\033[36mint\033[38;5;250m") != NULL);
	assert(strstr(output.bytes,
		      "\033[1;33mreturn\033[38;5;250m") != NULL);
	assert(strstr(output.bytes, "\033[35m42\033[38;5;250m") != NULL);
	assert(strstr(output.bytes,
		      "\033[2;38;5;244m──") != NULL);
	assert(strstr(output.bytes, "```c") == NULL);
	morph_md_kitty_destroy(renderer);
}

struct loader_capture {
	const char *path;
	int loads;
	int releases;
};

static int load_test_image(const char *url, char **path, void *user)
{
	struct loader_capture *capture = user;

	capture->loads++;
	*path = strstr(url, "missing") ? NULL : strdup(capture->path);
	return *path ? 0 : -1;
}

static void release_test_image(char *path, void *user)
{
	struct loader_capture *capture = user;

	capture->releases++;
	free(path);
}

static void test_loaded_images_keep_table_layout_and_stream_cache(void)
{
	struct morph_md_kitty_options options = {0};
	struct morph_md_kitty *renderer;
	struct capture output;
	struct media_capture media = {0};
	char path[] = "/tmp/morph-kitty-loader-XXXXXX";
	int fd = mkstemp(path);
	struct loader_capture loader = {path, 0, 0};
	const char *markdown =
		"| image | text |\n|---|---|\n"
		"| ![](https://example.test/a.jpg) | short |\n"
		"| ![](https://example.test/a.jpg) | `code` |\n"
		"| ![](missing.webp) | missing |\n\n";
	const char *tail = "![](https://example.test/a.jpg)\n\nAfter\n";

	assert(fd >= 0);
	close(fd);
	assert(write_test_png(path) == 0);
	capture_reset(&output);
	options.features = MORPH_MD_FEATURE_GFM;
	options.write = capture_write;
	options.user_data = &output;
	options.terminal_fd = -1;
	options.terminal_columns = 80u;
	options.media = capture_media;
	options.media_user_data = &media;
	options.load_image = load_test_image;
	options.release_image = release_test_image;
	options.image_user_data = &loader;
	renderer = morph_md_kitty_create(&options);
	assert(renderer != NULL);
	assert(morph_md_kitty_append(renderer, markdown, strlen(markdown), 0) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	assert(morph_md_kitty_append(renderer, tail, strlen(tail), 1) == 0);
	assert(morph_md_kitty_render(renderer) == 0);
	output.bytes[output.len] = '\0';
	assert(loader.loads == 2);
	assert(media.count == 0);
	assert(substring_count(output.bytes, "\033_Ga=T,f=100,") == 3);
	assert(strstr(output.bytes, "[image unavailable: missing.webp]") != NULL);
	assert_table_border_columns(output.bytes);
	morph_md_kitty_destroy(renderer);
	assert(loader.releases == 1);
	assert(unlink(path) == 0);
}

int main(void)
{
	test_stream_render_and_final();
	test_loaded_images_keep_table_layout_and_stream_cache();
	test_clear_sequence();
	test_incremental_render_preserves_scrollback();
	test_streaming_table_stays_in_live_tail();
	test_table_wraps_to_viewport();
	test_table_cjk_and_long_word_wrapping();
	test_table_code_is_atomic_and_tabs_are_stable();
	test_inline_code_uses_terminal_style();
	test_inline_code_wraps_with_content_padding();
	test_table_default_emoji_widths();
	test_links_show_destination_in_text_and_tables();
	test_content_padding_and_wrapping();
	test_initial_cursor_column_and_wrapping();
	test_initial_cursor_column_preserves_prefix_on_refresh();
	test_initial_cursor_column_refreshes_later_rows_from_margin();
	test_heading_underline_excludes_left_padding();
	test_local_png_bypasses_media_callback();
	test_media_callbacks_follow_document_order();
	test_streaming_media_callbacks_emit_once_in_place();
	test_local_png_renders_in_blocks_and_tables();
	test_streaming_image_is_transmitted_once();
	test_table_text_is_centered_beside_tall_formula();
	test_math_uses_native_size_kitty_transfer();
	test_streaming_math_is_append_only();
	test_fenced_code_uses_syntax_highlighting();
	return 0;
}
