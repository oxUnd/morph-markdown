#include "morph_markdown_stream.h"
#include "base/md_strmap.h"

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

static void on_patch(enum morph_md_patch_op op, const char *json, void *user)
{
	struct patch_counter *counter = user;

	(void)json;
	if (op == MORPH_MD_PATCH_INSERT) {
		counter->inserts++;
		snprintf(counter->last_insert, sizeof(counter->last_insert),
			 "%s", json ? json : "");
	} else if (op == MORPH_MD_PATCH_UPDATE) {
		counter->updates++;
	} else if (op == MORPH_MD_PATCH_SEAL) {
		counter->seals++;
	} else if (op == MORPH_MD_PATCH_FINISH) {
		counter->finishes++;
	} else if (op == MORPH_MD_PATCH_ERROR) {
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
	struct morph_md_options opts = {0};
	struct morph_md_stream *stream;
	char *snapshot;
	int rc;

	opts.enable_gfm = 1;
	opts.enable_math = 1;
	opts.max_tail_bytes = 65536u;

	stream = morph_md_stream_create(&opts, on_patch, &counter);
	if (!stream)
		return 1;

	rc = morph_md_stream_append(stream, "| a | b |\n",
				    strlen("| a | b |\n"), 0);
	if (rc != 0)
		return 2;
	rc = morph_md_stream_append(stream, "|---|---|\n| 1 | 2 |\n\nDone\n",
				    strlen("|---|---|\n| 1 | 2 |\n\nDone\n"), 1);
	if (rc != 0)
		return 3;

	snapshot = morph_md_stream_snapshot(stream);
	if (!snapshot)
		return 4;
	if (!contains(snapshot, "\"table\"") || !contains(snapshot, "\"Done\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_stream_destroy(stream);
		return 5;
	}

	morph_md_free(snapshot);
	morph_md_stream_destroy(stream);
	return counter.errors == 0 && counter.finishes == 1 &&
		       counter.seals > 0 ? 0 : 6;
}

static int test_fenced_code_tail(void)
{
	struct patch_counter counter = {0};
	struct morph_md_options opts = {0};
	struct morph_md_stream *stream;
	char *snapshot;
	int rc;

	opts.enable_gfm = 1;
	opts.enable_math = 1;
	opts.max_tail_bytes = 65536u;

	stream = morph_md_stream_create(&opts, on_patch, &counter);
	if (!stream)
		return 10;

	rc = morph_md_stream_append(stream, "```c\nint main(void) {\n",
				    strlen("```c\nint main(void) {\n"), 0);
	if (rc != 0)
		return 11;

	snapshot = morph_md_stream_snapshot(stream);
	if (!snapshot)
		return 12;
	if (!contains(snapshot, "int main")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_stream_destroy(stream);
		return 13;
	}
	morph_md_free(snapshot);

	rc = morph_md_stream_append(stream, "\treturn 0;\n}\n```\n",
				    strlen("\treturn 0;\n}\n```\n"), 1);
	if (rc != 0)
		return 14;
	snapshot = morph_md_stream_snapshot(stream);
	if (!snapshot)
		return 15;
	if (!contains(snapshot, "\"code_block\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_stream_destroy(stream);
		return 16;
	}

	morph_md_free(snapshot);
	morph_md_stream_destroy(stream);
	return counter.errors == 0 ? 0 : 17;
}

static int test_math_toggle(void)
{
	struct patch_counter counter = {0};
	struct morph_md_options opts = {0};
	struct morph_md_stream *stream;
	char *snapshot;
	int rc;

	opts.enable_gfm = 1;
	opts.enable_math = 1;
	stream = morph_md_stream_create(&opts, on_patch, &counter);
	if (!stream)
		return 20;

	rc = morph_md_stream_append(stream, "Euler $e=mc^2$ done\n",
				    strlen("Euler $e=mc^2$ done\n"), 1);
	if (rc != 0)
		return 21;
	snapshot = morph_md_stream_snapshot(stream);
	if (!snapshot)
		return 22;
	if (!contains(snapshot, "\"math_inline\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_stream_destroy(stream);
		return 23;
	}
	morph_md_free(snapshot);
	morph_md_stream_destroy(stream);

	opts.enable_math = 0;
	stream = morph_md_stream_create(&opts, on_patch, &counter);
	if (!stream)
		return 24;
	rc = morph_md_stream_append(stream, "Euler $e=mc^2$ done\n",
				    strlen("Euler $e=mc^2$ done\n"), 1);
	if (rc != 0)
		return 25;
	snapshot = morph_md_stream_snapshot(stream);
	if (!snapshot)
		return 26;
	if (contains(snapshot, "\"math_inline\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_stream_destroy(stream);
		return 27;
	}
	morph_md_free(snapshot);
	morph_md_stream_destroy(stream);
	return 0;
}

static int test_sourcepos_image_and_tasklist(void)
{
	struct patch_counter counter = {0};
	struct morph_md_options opts = {0};
	struct morph_md_stream *stream;
	const char *input;
	char *snapshot;
	int rc;

	opts.enable_gfm = 1;
	opts.enable_math = 1;
	stream = morph_md_stream_create(&opts, on_patch, &counter);
	if (!stream)
		return 30;

	input = "# Title\n\n- [x] done\n\n![alt](file:///tmp/a.png \"pic\")\n";
	rc = morph_md_stream_append(stream, input, strlen(input), 1);
	if (rc != 0)
		return 31;

	snapshot = morph_md_stream_snapshot(stream);
	if (!snapshot)
		return 32;
	if (!contains(snapshot, "\"sourcepos\"") ||
	    !contains(snapshot, "\"image\"") ||
	    !contains(snapshot, "file:///tmp/a.png") ||
	    !contains(snapshot, "\"tasklist\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_stream_destroy(stream);
		return 33;
	}

	morph_md_free(snapshot);
	morph_md_stream_destroy(stream);
	return counter.seals > 0 ? 0 : 34;
}

static int test_display_math(void)
{
	struct patch_counter counter = {0};
	struct morph_md_options opts = {0};
	struct morph_md_stream *stream;
	char *snapshot;
	int rc;

	opts.enable_gfm = 1;
	opts.enable_math = 1;
	stream = morph_md_stream_create(&opts, on_patch, &counter);
	if (!stream)
		return 40;

	rc = morph_md_stream_append(stream, "$$a+b=c$$\n",
				    strlen("$$a+b=c$$\n"), 1);
	if (rc != 0)
		return 41;
	snapshot = morph_md_stream_snapshot(stream);
	if (!snapshot)
		return 42;
	if (!contains(snapshot, "\"math_block\"")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_stream_destroy(stream);
		return 43;
	}
	morph_md_free(snapshot);
	morph_md_stream_destroy(stream);
	return 0;
}

static int test_insert_ids_advance(void)
{
	struct patch_counter counter = {0};
	struct morph_md_options opts = {0};
	struct morph_md_stream *stream;
	int rc;

	opts.enable_gfm = 1;
	stream = morph_md_stream_create(&opts, on_patch, &counter);
	if (!stream)
		return 50;

	rc = morph_md_stream_append(stream, "one\n\n", strlen("one\n\n"), 0);
	if (rc != 0)
		return 51;
	rc = morph_md_stream_append(stream, "two\n\n", strlen("two\n\n"), 1);
	if (rc != 0)
		return 52;

	if (counter.inserts < 2 || contains(counter.last_insert, "\"id\":1")) {
		fprintf(stderr, "%s\n", counter.last_insert);
		morph_md_stream_destroy(stream);
		return 53;
	}

	morph_md_stream_destroy(stream);
	return 0;
}

static int test_utf8_split_chunks(void)
{
	struct patch_counter counter = {0};
	struct morph_md_options opts = {0};
	struct morph_md_stream *stream;
	const char *input;
	char *snapshot;
	int rc;

	opts.enable_gfm = 1;
	stream = morph_md_stream_create(&opts, on_patch, &counter);
	if (!stream)
		return 60;

	input = "你好😀\n";
	rc = morph_md_stream_append(stream, input, 1u, 0);
	if (rc != 0)
		return 61;
	rc = morph_md_stream_append(stream, input + 1u, 4u, 0);
	if (rc != 0)
		return 62;
	rc = morph_md_stream_append(stream, input + 5u, strlen(input) - 5u, 1);
	if (rc != 0)
		return 63;

	snapshot = morph_md_stream_snapshot(stream);
	if (!snapshot)
		return 64;
	if (!contains(snapshot, "你好😀")) {
		fprintf(stderr, "%s\n", snapshot);
		morph_md_free(snapshot);
		morph_md_stream_destroy(stream);
		return 65;
	}
	morph_md_free(snapshot);
	morph_md_stream_destroy(stream);
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

	rc = test_display_math();
	if (rc != 0)
		return rc;

	rc = test_insert_ids_advance();
	if (rc != 0)
		return rc;

	rc = test_utf8_split_chunks();
	if (rc != 0)
		return rc;

	rc = test_strmap_foundation();
	if (rc != 0)
		return rc;

	return 0;
}
