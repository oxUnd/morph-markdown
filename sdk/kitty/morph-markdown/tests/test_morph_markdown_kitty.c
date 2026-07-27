#include "morph_markdown_kitty.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

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
		      "\n  alpha beta \n  gamma\n\n\n\n") == 0);
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
	assert(strncmp(output.bytes, "    \033[1;4;38;5;81mTitle", 25u) == 0);
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
	assert(strstr(output.bytes, ",C=1,q=2,m=") != NULL);
	assert(strstr(output.bytes, ",c=") == NULL);
	assert(strstr(output.bytes, "│ $x^2$ ") == NULL);
	assert(strstr(output.bytes, "│ rendered ") != NULL);
	assert(max_kitty_image_height(output.bytes) > 40u);
	morph_md_kitty_destroy(renderer);
}

int main(void)
{
	test_stream_render_and_final();
	test_clear_sequence();
	test_content_padding_and_wrapping();
	test_heading_underline_excludes_left_padding();
	test_media_callback();
	test_math_uses_native_size_kitty_transfer();
	return 0;
}
