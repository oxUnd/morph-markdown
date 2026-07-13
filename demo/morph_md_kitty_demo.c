#include "morph_markdown_kitty.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef MORPH_MATHJAX_FONT_PATH
#define MORPH_MATHJAX_FONT_PATH "fonts/STIXTwoMath-Regular.ttf"
#endif

static const char *demo_chunks[] = {
	"# Streaming Markdown + Kitty Math\n\n",
	"Model text arrives in chunks. Inline formula: ",
	"$e^{i\\pi}+1=0$",
	" and display math follows.\n\n",
	"$$\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}$$\n\n",
	"- GFM task list works\n- [x] parse Markdown\n- [ ] migrate to mobile\n\n",
	"| feature | status |\n",
	"|---|---|\n",
	"| CommonMark/GFM | ok |\n",
	"| MathJax-C Kitty | ok |\n\n",
	"Image placeholder: ![demo](file:///tmp/demo.png)\n"
};

static void clear_screen(void)
{
	fputs("\033[H\033[2J", stdout);
}

int main(void)
{
	struct morph_md_kitty_options options;
	struct morph_md_kitty *renderer;
	size_t i;

	memset(&options, 0, sizeof(options));
	options.font_path = MORPH_MATHJAX_FONT_PATH;
	options.fg_color = 0xFFFFFFu;
	options.bg_color = 0x000000u;
	options.dpi = 72u;
	options.enable_gfm = 1;
	options.enable_math = 1;

	renderer = morph_md_kitty_create(&options);
	if (!renderer) {
		fprintf(stderr, "failed to initialize kitty renderer\n");
		return 1;
	}

	for (i = 0; i < sizeof(demo_chunks) / sizeof(demo_chunks[0]); i++) {
		(void)morph_md_kitty_append(renderer, demo_chunks[i],
					    strlen(demo_chunks[i]), 0);
		clear_screen();
		printf("chunk %zu/%zu\n\n", i + 1u,
		       sizeof(demo_chunks) / sizeof(demo_chunks[0]));
		(void)morph_md_kitty_render(renderer);
		usleep(450000);
	}

	morph_md_kitty_destroy(renderer);
	return 0;
}
