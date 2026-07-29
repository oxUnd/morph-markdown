#include "morph_markdown_kitty.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
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
	assert(strstr(output.bytes, "│ `abcdefghij` │\n") != NULL);
	assert(strstr(output.bytes, "│ a    b       │\n") != NULL);
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

static void test_media_callback(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	struct media_capture media;
	const char *markdown = "![plot](file:///tmp/plot.png)";

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
	assert(media.count == 1);
	assert(strcmp(media.type, "image") == 0);
	assert(strcmp(media.path, "/tmp/plot.png") == 0);
	morph_md_kitty_destroy(renderer);
}

static void test_local_png_renders_in_blocks_and_tables(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	struct capture output;
	char path[128];
	char markdown[512];

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
		      "\033[2;38;5;244m╭─") != NULL);
	assert(strstr(output.bytes, "quote before code\n\n\n") == NULL);
	assert(strstr(output.bytes,
		      "\033[2;38;5;244m╭─"
		      "\033[1;38;5;75m c "
		      "\033[2;38;5;244m─") != NULL);
	assert(strstr(output.bytes,
		      "\033[2;38;5;244m│\033[0m "
		      "\033[36mint") != NULL);
	assert(strstr(output.bytes,
		      "\033[36mint\033[38;5;250m") != NULL);
	assert(strstr(output.bytes,
		      "\033[1;33mreturn\033[38;5;250m") != NULL);
	assert(strstr(output.bytes, "\033[35m42\033[38;5;250m") != NULL);
	assert(strstr(output.bytes,
		      "\033[2;38;5;244m╰") != NULL);
	assert(strstr(output.bytes, "```c") == NULL);
	morph_md_kitty_destroy(renderer);
}

int main(void)
{
	test_stream_render_and_final();
	test_clear_sequence();
	test_incremental_render_preserves_scrollback();
	test_streaming_table_stays_in_live_tail();
	test_table_wraps_to_viewport();
	test_table_cjk_and_long_word_wrapping();
	test_table_code_is_atomic_and_tabs_are_stable();
	test_table_default_emoji_widths();
	test_links_show_destination_in_text_and_tables();
	test_content_padding_and_wrapping();
	test_initial_cursor_column_and_wrapping();
	test_initial_cursor_column_preserves_prefix_on_refresh();
	test_initial_cursor_column_refreshes_later_rows_from_margin();
	test_heading_underline_excludes_left_padding();
	test_media_callback();
	test_local_png_renders_in_blocks_and_tables();
	test_streaming_image_is_transmitted_once();
	test_math_uses_native_size_kitty_transfer();
	test_streaming_math_is_append_only();
	test_fenced_code_uses_syntax_highlighting();
	return 0;
}
