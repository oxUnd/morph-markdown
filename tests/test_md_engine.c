#include "morph_markdown.h"
#include "base/md_strmap.h"
#include "base/md_width.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct patch_counter {
	int inserts;
	int updates;
	int seals;
	int finishes;
	int errors;
	char last_insert[4096];
};

static struct morph_md_engine *new_engine(
	struct morph_md_engine_options *opts,
	void (*on_event)(const struct morph_md_event *event, void *user),
	void *user)
{
	struct morph_md_engine *engine;

	opts->on_event = on_event;
	opts->user_data = user;
	if (morph_md_engine_create(opts, &engine) != 0)
		return NULL;
	return engine;
}

static char *snapshot_json(struct morph_md_engine *engine)
{
	struct morph_md_doc *doc;
	char *json;

	if (morph_md_engine_snapshot(engine, &doc) != 0)
		return NULL;
	if (morph_md_doc_to_json(doc, &json) != 0)
		json = NULL;
	morph_md_doc_release(doc);
	return json;
}

static void on_patch(const struct morph_md_event *event, void *user)
{
	struct patch_counter *counter = user;
	const char *json;

	json = event->json;
	if (event->type == MORPH_MD_EVENT_INSERT) {
		counter->inserts++;
		snprintf(counter->last_insert, sizeof(counter->last_insert),
			 "%s", json ? json : "");
	} else if (event->type == MORPH_MD_EVENT_REPLACE) {
		counter->updates++;
	} else if (event->type == MORPH_MD_EVENT_SEAL) {
		counter->seals++;
	} else if (event->type == MORPH_MD_EVENT_FINISH) {
		counter->finishes++;
	} else if (event->type == MORPH_MD_EVENT_ERROR) {
		counter->errors++;
	}
}

static int contains(const char *haystack, const char *needle)
{
	return strstr(haystack, needle) != NULL;
}

static int test_streaming_table(void)
{
	struct patch_counter counter = {0};
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	char *snapshot;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	opts.features |= MORPH_MD_FEATURE_MATH;
	opts.max_tail_bytes = 65536u;

	stream = new_engine(&opts, on_patch, &counter);
	if (!stream)
		return 1;

	rc = morph_md_engine_append(stream, "| a | b |\n",
				    strlen("| a | b |\n"), 0);
	if (rc != 0)
		return 2;
	rc = morph_md_engine_append(stream, "|---|---|\n| 1 | 2 |\n\nDone\n",
				    strlen("|---|---|\n| 1 | 2 |\n\nDone\n"), 1);
	if (rc != 0)
		return 3;

	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 4;
	if (!contains(snapshot, "\"table\"") || !contains(snapshot, "\"Done\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 5;
	}

	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);
	return counter.errors == 0 && counter.finishes == 1 &&
		       counter.seals > 0 ? 0 : 6;
}

static int test_fenced_code_tail(void)
{
	struct patch_counter counter = {0};
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	char *snapshot;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	opts.features |= MORPH_MD_FEATURE_MATH;
	opts.max_tail_bytes = 65536u;

	stream = new_engine(&opts, on_patch, &counter);
	if (!stream)
		return 10;

	rc = morph_md_engine_append(stream, "```c\nint main(void) {\n",
				    strlen("```c\nint main(void) {\n"), 0);
	if (rc != 0)
		return 11;

	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 12;
	if (!contains(snapshot, "int main")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 13;
	}
	morph_md_free(snapshot);

	rc = morph_md_engine_append(stream, "\treturn 0;\n}\n```\n",
				    strlen("\treturn 0;\n}\n```\n"), 1);
	if (rc != 0)
		return 14;
	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 15;
	if (!contains(snapshot, "\"code_block\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 16;
	}

	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);
	return counter.errors == 0 ? 0 : 17;
}

static int test_math_toggle(void)
{
	struct patch_counter counter = {0};
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	char *snapshot;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	opts.features |= MORPH_MD_FEATURE_MATH;
	stream = new_engine(&opts, on_patch, &counter);
	if (!stream)
		return 20;

	rc = morph_md_engine_append(stream, "Euler $e=mc^2$ done\n",
				    strlen("Euler $e=mc^2$ done\n"), 1);
	if (rc != 0)
		return 21;
	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 22;
	if (!contains(snapshot, "\"math_inline\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 23;
	}
	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);

	opts.features &= ~MORPH_MD_FEATURE_MATH;
	stream = new_engine(&opts, on_patch, &counter);
	if (!stream)
		return 24;
	rc = morph_md_engine_append(stream, "Euler $e=mc^2$ done\n",
				    strlen("Euler $e=mc^2$ done\n"), 1);
	if (rc != 0)
		return 25;
	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 26;
	if (contains(snapshot, "\"math_inline\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 27;
	}
	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);
	return 0;
}

static int test_sourcepos_image_and_tasklist(void)
{
	struct patch_counter counter = {0};
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	const char *input;
	char *snapshot;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	opts.features |= MORPH_MD_FEATURE_MATH;
	stream = new_engine(&opts, on_patch, &counter);
	if (!stream)
		return 30;

	input = "# Title\n\n- [x] done\n\n![alt](file:///tmp/a.png \"pic\")\n";
	rc = morph_md_engine_append(stream, input, strlen(input), 1);
	if (rc != 0)
		return 31;

	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 32;
	if (!contains(snapshot, "\"sourcepos\"") ||
	    !contains(snapshot, "\"image\"") ||
	    !contains(snapshot, "file:///tmp/a.png") ||
	    !contains(snapshot, "\"tasklist\"") ||
	    !contains(snapshot, "\"checked\":true")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 33;
	}

	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);
	return counter.seals > 0 ? 0 : 34;
}

static int test_list_metadata(void)
{
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	const char *input;
	char *snapshot;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	opts.features |= MORPH_MD_FEATURE_MATH;
	stream = new_engine(&opts, NULL, NULL);
	if (!stream)
		return 35;

	input = "3. one\n4. two\n";
	rc = morph_md_engine_append(stream, input, strlen(input), 1);
	if (rc != 0)
		return 36;

	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 37;
	if (!contains(snapshot, "\"list_type\":\"ordered\"") ||
	    !contains(snapshot, "\"start\":3")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 38;
	}

	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);
	return 0;
}

static int test_display_math(void)
{
	struct patch_counter counter = {0};
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	char *snapshot;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	opts.features |= MORPH_MD_FEATURE_MATH;
	stream = new_engine(&opts, on_patch, &counter);
	if (!stream)
		return 40;

	rc = morph_md_engine_append(stream, "$$a+b=c$$\n",
				    strlen("$$a+b=c$$\n"), 1);
	if (rc != 0)
		return 41;
	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 42;
	if (!contains(snapshot, "\"math_block\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 43;
	}
	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);
	return 0;
}

static int test_insert_ids_advance(void)
{
	struct patch_counter counter = {0};
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	stream = new_engine(&opts, on_patch, &counter);
	if (!stream)
		return 50;

	rc = morph_md_engine_append(stream, "one\n\n", strlen("one\n\n"), 0);
	if (rc != 0)
		return 51;
	rc = morph_md_engine_append(stream, "two\n\n", strlen("two\n\n"), 1);
	if (rc != 0)
		return 52;

	if (counter.inserts < 2 || contains(counter.last_insert, "\"id\":1")) {
		fprintf(stderr, "%s\n", counter.last_insert);
		morph_md_engine_destroy(stream);
		return 53;
	}

	morph_md_engine_destroy(stream);
	return 0;
}

static int test_per_block_insert_patches(void)
{
	struct patch_counter counter = {0};
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	stream = new_engine(&opts, on_patch, &counter);
	if (!stream)
		return 90;

	rc = morph_md_engine_append(stream, "# A\n\nB\n\n", strlen("# A\n\nB\n\n"), 0);
	if (rc != 0)
		return 91;
	if (counter.inserts != 2 || counter.seals != 1) {
		morph_md_engine_destroy(stream);
		return 92;
	}

	morph_md_engine_destroy(stream);
	return 0;
}

static int test_utf8_split_chunks(void)
{
	struct patch_counter counter = {0};
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	struct morph_md_stats stats;
	const char *input;
	char *snapshot;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	stream = new_engine(&opts, on_patch, &counter);
	if (!stream)
		return 60;

	input = "你好😀\n";
	rc = morph_md_engine_append(stream, input, 1u, 0);
	if (rc != 0)
		return 61;
	if (morph_md_engine_get_stats(stream, &stats) != 0 ||
	    stats.utf8_pending_bytes != 1u)
		return 62;
	rc = morph_md_engine_append(stream, input + 1u, 4u, 0);
	if (rc != 0)
		return 63;
	rc = morph_md_engine_append(stream, input + 5u, strlen(input) - 5u, 1);
	if (rc != 0)
		return 64;
	if (morph_md_engine_get_stats(stream, &stats) != 0 ||
	    stats.utf8_pending_bytes != 0u || !stats.finished)
		return 65;

	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 66;
	if (!contains(snapshot, "你好😀")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 67;
	}
	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);
	return 0;
}

static int test_strmap_foundation(void)
{
	struct md_strmap map;
	int one;
	int two;

	one = 1;
	two = 2;
	md_strmap_init(&map);
	if (md_strmap_set(&map, "one", &one) != 0)
		return 70;
	if (md_strmap_set(&map, "two", &two) != 0)
		return 71;
	if (*(int *)md_strmap_get(&map, "one") != 1)
		return 72;
	if (!md_strmap_contains(&map, "two"))
		return 73;
	md_strmap_clear(&map);
	if (md_strmap_contains(&map, "one"))
		return 74;
	md_strmap_cleanup(&map);
	return 0;
}

static int test_html_policy(void)
{
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	char *snapshot;
	const char *input;
	int rc;

	input = "before <b>x</b> after\n\n<div>raw</div>\n";
	opts.features |= MORPH_MD_FEATURE_GFM;
	opts.html_policy = MORPH_MD_HTML_STRIP;
	stream = new_engine(&opts, NULL, NULL);
	if (!stream)
		return 80;
	rc = morph_md_engine_append(stream, input, strlen(input), 1);
	if (rc != 0)
		return 81;
	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 82;
	if (contains(snapshot, "html") || contains(snapshot, "<div>")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 83;
	}
	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);

	opts.html_policy = MORPH_MD_HTML_TEXT;
	stream = new_engine(&opts, NULL, NULL);
	if (!stream)
		return 84;
	rc = morph_md_engine_append(stream, input, strlen(input), 1);
	if (rc != 0)
		return 85;
	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 86;
	if (contains(snapshot, "\"html_inline\"") ||
	    !contains(snapshot, "<b>") ||
	    !contains(snapshot, "</b>") ||
	    !contains(snapshot, "<div>raw</div>")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 87;
	}
	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);
	return 0;
}

static int test_footnotes_extension(void)
{
	struct morph_md_engine_options opts = {0};
	struct morph_md_engine *stream;
	const char *input;
	char *snapshot;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	input = "note[^a]\n\n[^a]: body\n";
	stream = new_engine(&opts, NULL, NULL);
	if (!stream)
		return 100;
	rc = morph_md_engine_append(stream, input, strlen(input), 1);
	if (rc != 0)
		return 101;
	snapshot = snapshot_json(stream);
	if (!snapshot)
		return 102;
	if (!contains(snapshot, "\"link\"") || !contains(snapshot, "\"url\":\"body\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_engine_destroy(stream);
		return 103;
	}
	morph_md_free(snapshot);
	morph_md_engine_destroy(stream);
	return 0;
}

static int test_tail_limit_does_not_finish_stream(void)
{
	struct morph_md_engine_options opts = {0};
	struct morph_md_stats stats;
	struct morph_md_engine *stream;
	int rc;

	opts.features |= MORPH_MD_FEATURE_GFM;
	opts.max_tail_bytes = 4u;
	stream = new_engine(&opts, NULL, NULL);
	if (!stream)
		return 110;

	rc = morph_md_engine_append(stream, "abcdef", strlen("abcdef"), 0);
	if (rc != 0)
		return 111;
	if (morph_md_engine_get_stats(stream, &stats) != 0 || stats.finished)
		return 112;
	rc = morph_md_engine_append(stream, "\n", strlen("\n"), 1);
	if (rc != 0)
		return 113;
	if (morph_md_engine_get_stats(stream, &stats) != 0 || !stats.finished)
		return 114;

	morph_md_engine_destroy(stream);
	return 0;
}

static int test_display_width_foundation(void)
{
	if (md_utf8_display_width("abc") != 3)
		return 120;
	if (md_utf8_display_width("中文") != 4)
		return 121;
	if (md_utf8_display_width("a😀") != 3)
		return 122;
	return 0;
}

static int test_event_json_api(void)
{
	struct morph_md_event event;
	char *json;

	memset(&event, 0, sizeof(event));
	event.type = MORPH_MD_EVENT_INSERT;
	event.json = "{\"kind\":\"paragraph\"}";
	if (morph_md_event_to_json(&event, &json) != 0)
		return 130;
	if (!contains(json, "\"type\":\"insert\"") ||
	    !contains(json, "\"payload\":{\"kind\":\"paragraph\"}")) {
		fprintf(stderr, "%s\n", json);
		morph_md_free(json);
		return 131;
	}
	morph_md_free(json);
	return 0;
}

int main(void)
{
	int rc;

	rc = test_streaming_table();
	if (rc != 0)
		return rc;

	rc = test_fenced_code_tail();
	if (rc != 0)
		return rc;

	rc = test_math_toggle();
	if (rc != 0)
		return rc;

	rc = test_sourcepos_image_and_tasklist();
	if (rc != 0)
		return rc;

	rc = test_list_metadata();
	if (rc != 0)
		return rc;

	rc = test_display_math();
	if (rc != 0)
		return rc;

	rc = test_insert_ids_advance();
	if (rc != 0)
		return rc;

	rc = test_per_block_insert_patches();
	if (rc != 0)
		return rc;

	rc = test_utf8_split_chunks();
	if (rc != 0)
		return rc;

	rc = test_strmap_foundation();
	if (rc != 0)
		return rc;

	rc = test_html_policy();
	if (rc != 0)
		return rc;

	rc = test_footnotes_extension();
	if (rc != 0)
		return rc;

	rc = test_tail_limit_does_not_finish_stream();
	if (rc != 0)
		return rc;

	rc = test_display_width_foundation();
	if (rc != 0)
		return rc;

	rc = test_event_json_api();
	if (rc != 0)
		return rc;

	return 0;
}
