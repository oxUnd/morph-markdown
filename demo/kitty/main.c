#include "morph_markdown_kitty.h"
#include "markdown_fixtures.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef MORPH_MATHJAX_FONT_PATH
#define MORPH_MATHJAX_FONT_PATH "fonts/STIXTwoMath-Regular.ttf"
#endif

static void pause_between_chunks(int enabled)
{
	struct timespec delay = { 0, 450000000L };

	if (enabled)
		(void)nanosleep(&delay, NULL);
}

static int has_argument(int argc, char **argv, const char *argument)
{
	int i;

	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], argument) == 0)
			return 1;
	return 0;
}

int main(int argc, char **argv)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	size_t i;
	size_t chunk_count;
	int animate;
	int rc;

	memset(&options, 0, sizeof(options));
	options.font_path = MORPH_MATHJAX_FONT_PATH;
	options.fg_color = 0xFFFFFFFFu;
	options.bg_color = 0x000000u;
	options.dpi = 72u;
	options.features = MORPH_MD_FEATURE_GFM | MORPH_MD_FEATURE_MATH;
	options.terminal_fd = STDOUT_FILENO;
	if (!has_argument(argc, argv, "--no-padding")) {
		options.content_padding_top_rows = 1u;
		options.content_padding_right_columns = 4u;
		options.content_padding_bottom_rows = 1u;
		options.content_padding_left_columns = 4u;
	}
	animate = isatty(STDOUT_FILENO) &&
		  !has_argument(argc, argv, "--no-delay");

	renderer = morph_md_kitty_create(&options);
	if (!renderer) {
		fprintf(stderr, "failed to initialize kitty renderer\n");
		return 1;
	}

	chunk_count = morph_demo_chunk_count;
	for (i = 0; i < chunk_count; i++) {
		rc = morph_md_kitty_append(renderer, morph_demo_chunks[i],
					   strlen(morph_demo_chunks[i]),
					   i + 1u == chunk_count);
		if (rc != 0)
			break;
		rc = morph_md_kitty_render(renderer);
		if (rc != 0)
			break;
		pause_between_chunks(animate);
	}

	morph_md_kitty_destroy(renderer);
	if (rc != 0)
		fprintf(stderr, "kitty rendering failed: %d\n", rc);
	return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
