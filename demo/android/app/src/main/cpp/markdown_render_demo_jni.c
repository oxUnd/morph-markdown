#include "morph_markdown_stream.h"
#include "mathjax.h"

#include <android/log.h>
#include <jni.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "MarkdownRenderDemo"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static uint32_t rgba_to_argb(uint32_t rgba)
{
	uint32_t r;
	uint32_t g;
	uint32_t b;
	uint32_t a;

	r = (rgba >> 24) & 0xffu;
	g = (rgba >> 16) & 0xffu;
	b = (rgba >> 8) & 0xffu;
	a = rgba & 0xffu;
	return (a << 24) | (r << 16) | (g << 8) | b;
}

static void release_string(JNIEnv *env, jstring jstr, const char *str)
{
	if (jstr && str)
		(*env)->ReleaseStringUTFChars(env, jstr, str);
}

JNIEXPORT jstring JNICALL
Java_com_morph_markdown_demo_MarkdownNative_snapshot(
	JNIEnv *env,
	jobject thiz,
	jstring markdown)
{
	struct morph_md_options opts;
	struct morph_md_stream *stream = NULL;
	const char *input = NULL;
	char *snapshot = NULL;
	jstring out = NULL;

	(void)thiz;
	if (!markdown)
		return NULL;

	input = (*env)->GetStringUTFChars(env, markdown, NULL);
	if (!input)
		return NULL;

	memset(&opts, 0, sizeof(opts));
	opts.enable_gfm = 1;
	opts.enable_math = 1;
	opts.max_tail_bytes = 65536u;
	opts.html_policy = MORPH_MD_HTML_TEXT;

	stream = morph_md_stream_create(&opts, NULL, NULL);
	if (!stream)
		goto out;
	if (morph_md_stream_append(stream, input, strlen(input), 1) != 0)
		goto out;

	snapshot = morph_md_stream_snapshot(stream);
	if (!snapshot)
		goto out;
	out = (*env)->NewStringUTF(env, snapshot);

out:
	if (!out)
		LOGE("markdown snapshot failed");
	if (snapshot)
		morph_md_free(snapshot);
	if (stream)
		morph_md_stream_destroy(stream);
	release_string(env, markdown, input);
	return out;
}

JNIEXPORT jintArray JNICALL
Java_com_morph_markdown_demo_MarkdownNative_renderLatex(
	JNIEnv *env,
	jobject thiz,
	jstring font_path,
	jstring latex,
	jboolean display,
	jfloat font_size_px)
{
	const char *font = NULL;
	const char *expr = NULL;
	const uint32_t *pixels;
	jintArray out = NULL;
	jint *values = NULL;
	mjx_opts opts;
	mjx_ctx *ctx = NULL;
	mjx_buf *buf = NULL;
	unsigned int width;
	unsigned int height;
	size_t count;
	size_t i;

	(void)thiz;
	if (!font_path || !latex)
		return NULL;

	font = (*env)->GetStringUTFChars(env, font_path, NULL);
	expr = (*env)->GetStringUTFChars(env, latex, NULL);
	if (!font || !expr)
		goto out;

	memset(&opts, 0, sizeof(opts));
	opts.font_path = font;
	opts.font_size = font_size_px > 0.0f ? (double)font_size_px : 18.0;
	opts.fg_color = 0x1b1b1bffu;
	opts.bg_color = 0x00000000u;
	opts.dpi = 72u;

	ctx = mjx_init(&opts);
	if (!ctx)
		goto out;
	buf = mjx_render_latex(ctx, expr,
			       display ? MJX_STYLE_DISPLAY : MJX_STYLE_INLINE);
	if (!buf)
		goto out;

	width = mjx_buf_width(buf);
	height = mjx_buf_height(buf);
	if (width == 0u || height == 0u)
		goto out;
	count = (size_t)width * (size_t)height;
	if (count > ((size_t)INT32_MAX - 2u))
		goto out;

	out = (*env)->NewIntArray(env, (jsize)(count + 2u));
	if (!out)
		goto out;
	values = calloc(count + 2u, sizeof(values[0]));
	if (!values)
		goto out;

	values[0] = (jint)width;
	values[1] = (jint)height;
	pixels = mjx_buf_pixels(buf);
	for (i = 0; i < count; i++)
		values[i + 2u] = (jint)rgba_to_argb(pixels[i]);
	(*env)->SetIntArrayRegion(env, out, 0, (jsize)(count + 2u), values);

out:
	if (!out)
		LOGE("mathjax render failed");
	free(values);
	if (buf)
		mjx_buf_free(buf);
	if (ctx)
		mjx_free(ctx);
	release_string(env, latex, expr);
	release_string(env, font_path, font);
	return out;
}
