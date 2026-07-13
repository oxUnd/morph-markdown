#include "morph_markdown_stream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct patch_counter {
	int inserts;
	int updates;
	int seals;
	int finishes;
	int errors;
};

static void on_patch(enum morph_md_patch_op op, const char *json, void *user)
{
	struct patch_counter *counter = user;

	(void)json;
	if (op == MORPH_MD_PATCH_INSERT)
		counter->inserts++;
	else if (op == MORPH_MD_PATCH_UPDATE)
		counter->updates++;
	else if (op == MORPH_MD_PATCH_SEAL)
		counter->seals++;
	else if (op == MORPH_MD_PATCH_FINISH)
		counter->finishes++;
	else if (op == MORPH_MD_PATCH_ERROR)
		counter->errors++;
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
	return counter.errors == 0 && counter.finishes == 1 ? 0 : 6;
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

	return 0;
}
