#ifndef MORPH_MARKDOWN_H
#define MORPH_MARKDOWN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum morph_md_feature {
	MORPH_MD_FEATURE_GFM = 1u << 0,
	MORPH_MD_FEATURE_MATH = 1u << 1
};

enum morph_md_html_policy {
	MORPH_MD_HTML_PASSTHROUGH = 0,
	MORPH_MD_HTML_STRIP = 1,
	MORPH_MD_HTML_TEXT = 2
};

enum morph_md_event_type {
	MORPH_MD_EVENT_INSERT = 1,
	MORPH_MD_EVENT_REPLACE = 2,
	MORPH_MD_EVENT_SEAL = 3,
	MORPH_MD_EVENT_FINISH = 4,
	MORPH_MD_EVENT_ERROR = 5
};

struct morph_md_event {
	enum morph_md_event_type type;
	const char *json;
	const char *message;
	size_t offset;
	size_t len;
	uint64_t hash;
};

struct morph_md_engine_options {
	uint32_t features;
	size_t max_tail_bytes;
	enum morph_md_html_policy html_policy;
	void *user_data;
	void (*on_event)(const struct morph_md_event *event, void *user_data);
};

struct morph_md_stats {
	size_t sealed_bytes;
	size_t tail_bytes;
	size_t utf8_pending_bytes;
	size_t sealed_blocks;
	int finished;
};

struct morph_md_engine;
struct morph_md_doc;

int morph_md_engine_create(const struct morph_md_engine_options *options,
			   struct morph_md_engine **out);

int morph_md_engine_append(struct morph_md_engine *engine,
			   const char *bytes,
			   size_t len,
			   int final);

int morph_md_engine_snapshot(struct morph_md_engine *engine,
			     struct morph_md_doc **out);

int morph_md_engine_get_stats(struct morph_md_engine *engine,
			      struct morph_md_stats *stats);

int morph_md_engine_stable_block_count(struct morph_md_engine *engine,
				       size_t *out_count);

int morph_md_doc_to_json(const struct morph_md_doc *doc, char **out_json);

int morph_md_event_to_json(const struct morph_md_event *event,
			   char **out_json);

void morph_md_doc_release(struct morph_md_doc *doc);

void morph_md_engine_destroy(struct morph_md_engine *engine);

void morph_md_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
