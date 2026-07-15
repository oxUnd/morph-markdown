#include "morph_markdown.h"
#include "mathjax.h"

#include <android/log.h>
#include <jni.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "MorphMarkdown"
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

JNIEXPORT jlong JNICALL
Java_com_morph_markdown_MarkdownNative_createEngine(
	JNIEnv *env,
	jobject thiz)
{
	struct morph_md_engine_options opts;
	struct morph_md_engine *engine = NULL;

	(void)env;
	(void)thiz;
	memset(&opts, 0, sizeof(opts));
	opts.features = MORPH_MD_FEATURE_GFM | MORPH_MD_FEATURE_MATH;
	opts.max_tail_bytes = 65536u;
	opts.html_policy = MORPH_MD_HTML_TEXT;
	if (morph_md_engine_create(&opts, &engine) != 0)
		return 0;
	return (jlong)(intptr_t)engine;
}

JNIEXPORT jint JNICALL
Java_com_morph_markdown_MarkdownNative_append(
	JNIEnv *env,
	jobject thiz,
	jlong handle,
	jstring markdown,
	jboolean final)
{
	struct morph_md_engine *engine;
	const char *input = NULL;
	int rc;

	(void)thiz;
	engine = (struct morph_md_engine *)(intptr_t)handle;
	if (!engine || !markdown)
		return -1;

	input = (*env)->GetStringUTFChars(env, markdown, NULL);
	if (!input)
		return -1;
	rc = morph_md_engine_append(engine, input, strlen(input), final);
	release_string(env, markdown, input);
	return rc;
}

JNIEXPORT jstring JNICALL
Java_com_morph_markdown_MarkdownNative_snapshotJson(
	JNIEnv *env,
	jobject thiz,
	jlong handle)
{
	struct morph_md_engine *engine;
	struct morph_md_doc *doc = NULL;
	char *json = NULL;
	jstring out = NULL;

	(void)thiz;
	engine = (struct morph_md_engine *)(intptr_t)handle;
	if (!engine)
		return NULL;

	if (morph_md_engine_snapshot(engine, &doc) != 0)
		goto out;
	if (morph_md_doc_to_json(doc, &json) != 0)
		goto out;
	out = (*env)->NewStringUTF(env, json);

out:
	if (!out)
		LOGE("markdown snapshot failed");
	if (json)
		morph_md_free(json);
	if (doc)
		morph_md_doc_release(doc);
	return out;
}

JNIEXPORT jint JNICALL
Java_com_morph_markdown_MarkdownNative_stableBlockCount(
	JNIEnv *env,
	jobject thiz,
	jlong handle)
{
	struct morph_md_engine *engine;
	size_t count = 0;

	(void)env;
	(void)thiz;
	engine = (struct morph_md_engine *)(intptr_t)handle;
	if (!engine)
		return 0;
	if (morph_md_engine_stable_block_count(engine, &count) != 0)
		return 0;
	if (count > INT32_MAX)
		return INT32_MAX;
	return (jint)count;
}

JNIEXPORT void JNICALL
Java_com_morph_markdown_MarkdownNative_destroyEngine(
	JNIEnv *env,
	jobject thiz,
	jlong handle)
{
	struct morph_md_engine *engine;

	(void)env;
	(void)thiz;
	engine = (struct morph_md_engine *)(intptr_t)handle;
	if (engine)
		morph_md_engine_destroy(engine);
}

JNIEXPORT jintArray JNICALL
Java_com_morph_markdown_MarkdownNative_renderLatex(
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
