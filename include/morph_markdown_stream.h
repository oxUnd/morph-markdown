#ifndef MORPH_MARKDOWN_STREAM_H
#define MORPH_MARKDOWN_STREAM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum morph_md_patch_op {
	MORPH_MD_PATCH_INSERT = 1,
	MORPH_MD_PATCH_UPDATE = 2,
	MORPH_MD_PATCH_SEAL = 3,
	MORPH_MD_PATCH_FINISH = 4,
	MORPH_MD_PATCH_ERROR = 5
};

enum morph_md_html_policy {
	MORPH_MD_HTML_PASSTHROUGH = 0,
	MORPH_MD_HTML_STRIP = 1,
	MORPH_MD_HTML_TEXT = 2
};

struct morph_md_options {
	int enable_gfm;
	int enable_math;
	size_t max_tail_bytes;
	int html_policy;
};

struct morph_md_stats {
	size_t sealed_bytes;
	size_t tail_bytes;
	size_t utf8_pending_bytes;
	size_t sealed_blocks;
	int finished;
};

typedef void (*morph_md_patch_cb)(enum morph_md_patch_op op,
				  const char *json,
				  void *user_data);

struct morph_md_stream;

struct morph_md_stream *morph_md_stream_create(
	const struct morph_md_options *options,
	morph_md_patch_cb callback,
	void *user_data);

int morph_md_stream_append(struct morph_md_stream *stream,
			   const char *bytes,
			   size_t len,
			   int is_final);

char *morph_md_stream_snapshot(struct morph_md_stream *stream);

int morph_md_stream_get_stats(struct morph_md_stream *stream,
			      struct morph_md_stats *stats);

void morph_md_stream_destroy(struct morph_md_stream *stream);

void morph_md_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
