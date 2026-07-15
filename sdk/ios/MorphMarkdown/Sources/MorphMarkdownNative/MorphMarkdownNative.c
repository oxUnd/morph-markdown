#include "MorphMarkdownNative.h"
#include "morph_markdown.h"
#include "mathjax.h"

#include <stdlib.h>
#include <string.h>

struct morph_ios_engine {
	struct morph_md_engine *engine;
};

morph_ios_engine *morph_ios_engine_create(void)
{
	struct morph_md_engine_options opts;
	struct morph_ios_engine *wrapper;

	wrapper = calloc(1u, sizeof(*wrapper));
	if (!wrapper)
		return NULL;

	memset(&opts, 0, sizeof(opts));
	opts.features = MORPH_MD_FEATURE_GFM | MORPH_MD_FEATURE_MATH;
	opts.max_tail_bytes = 65536u;
	opts.html_policy = MORPH_MD_HTML_TEXT;
	if (morph_md_engine_create(&opts, &wrapper->engine) != 0) {
		free(wrapper);
		return NULL;
	}
	return wrapper;
}

int morph_ios_engine_append(morph_ios_engine *engine,
			    const char *bytes,
			    size_t len,
			    int final)
{
	if (!engine || !engine->engine || !bytes)
		return -1;
	return morph_md_engine_append(engine->engine, bytes, len, final);
}

char *morph_ios_engine_snapshot_json(morph_ios_engine *engine)
{
	struct morph_md_doc *doc;
	char *json;

	if (!engine || !engine->engine)
		return NULL;
	if (morph_md_engine_snapshot(engine->engine, &doc) != 0)
		return NULL;
	if (morph_md_doc_to_json(doc, &json) != 0)
		json = NULL;
	morph_md_doc_release(doc);
	return json;
}

void morph_ios_engine_destroy(morph_ios_engine *engine)
{
	if (!engine)
		return;
	if (engine->engine)
		morph_md_engine_destroy(engine->engine);
	free(engine);
}

morph_ios_bitmap *morph_ios_render_latex(const char *font_path,
					 const char *latex,
					 int display,
					 double font_size)
{
	morph_ios_bitmap *out;
	mjx_opts opts;
	mjx_ctx *ctx;
	mjx_buf *buf;
	size_t count;

	if (!font_path || !latex)
		return NULL;
	memset(&opts, 0, sizeof(opts));
	opts.font_path = font_path;
	opts.font_size = font_size > 0.0 ? font_size : 18.0;
	opts.fg_color = 0x1b1b1bffu;
	opts.bg_color = 0x00000000u;
	opts.dpi = 72u;

	ctx = mjx_init(&opts);
	if (!ctx)
		return NULL;
	buf = mjx_render_latex(ctx, latex,
			       display ? MJX_STYLE_DISPLAY : MJX_STYLE_INLINE);
	if (!buf) {
		mjx_free(ctx);
		return NULL;
	}
	out = calloc(1u, sizeof(*out));
	if (!out)
		goto fail;
	out->width = mjx_buf_width(buf);
	out->height = mjx_buf_height(buf);
	count = (size_t)out->width * (size_t)out->height;
	if (out->width == 0u || out->height == 0u || count == 0u)
		goto fail;
	out->pixels_rgba = malloc(count * sizeof(out->pixels_rgba[0]));
	if (!out->pixels_rgba)
		goto fail;
	memcpy(out->pixels_rgba, mjx_buf_pixels(buf),
	       count * sizeof(out->pixels_rgba[0]));
	mjx_buf_free(buf);
	mjx_free(ctx);
	return out;

fail:
	if (out)
		morph_ios_bitmap_destroy(out);
	mjx_buf_free(buf);
	mjx_free(ctx);
	return NULL;
}

void morph_ios_bitmap_destroy(morph_ios_bitmap *bitmap)
{
	if (!bitmap)
		return;
	free(bitmap->pixels_rgba);
	free(bitmap);
}

void morph_ios_free(void *ptr)
{
	morph_md_free(ptr);
}
