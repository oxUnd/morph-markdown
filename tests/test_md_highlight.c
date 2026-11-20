#include "md_highlight.h"
#include "morph_markdown_highlight.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static char *highlight(const char *language, const char *code, size_t *out_len)
{
	struct morph_md_sbuf output;
	char *result;

	morph_md_sbuf_init(&output, NULL, 0u);
	morph_md_highlight_code(language, strlen(language), code, strlen(code),
				&output, NULL, NULL);
	result = malloc(output.len + 1u);
	assert(result != NULL);
	morph_md_sbuf_init(&output, result, output.len + 1u);
	morph_md_highlight_code(language, strlen(language), code, strlen(code),
				&output, NULL, NULL);
	result[output.len] = '\0';
	*out_len = output.len;
	return result;
}

static void test_c_tokens_are_colored(void)
{
	const char *code = "int main(void) { return 42; } // done\n";
	size_t len;
	char *result = highlight("c", code, &len);

	assert(len > strlen(code));
	assert(strstr(result, "\033[36mint\033[38;5;250m") != NULL);
	assert(strstr(result, "\033[37mmain\033[38;5;250m") != NULL);
	assert(strstr(result, "\033[1;33mreturn\033[38;5;250m") != NULL);
	assert(strstr(result, "\033[35m42\033[38;5;250m") != NULL);
	assert(strstr(result, "\033[2;36m// done\033[0m\n") != NULL);
	free(result);
}

static void test_alias_and_unknown_language(void)
{
	const char *python = "def answer(): return 42\n";
	const char *plain = "not highlighted\n";
	size_t len;
	char *result = highlight("py", python, &len);

	assert(strstr(result, "\033[1;33mdef\033[38;5;250m") != NULL);
	free(result);
	result = highlight("unknown-language", plain, &len);
	assert(len == strlen(plain));
	assert(memcmp(result, plain, len) == 0);
	free(result);
}

static void test_long_line_is_not_truncated(void)
{
	const size_t code_len = 20000u;
	char *code = malloc(code_len + 1u);
	size_t len;
	char *result;

	assert(code != NULL);
	memset(code, 'x', code_len);
	code[code_len] = '\0';
	result = highlight("c", code, &len);
	assert(len >= code_len);
	assert(strstr(result, code) != NULL);
	free(result);
	assert(morph_md_highlight_ansi("c", code, code_len, &result, &len) == 0);
	assert(len >= code_len);
	assert(strstr(result, code) != NULL);
	free(result);
	free(code);
}

static void test_file_language_detection(void)
{
	static const struct {
		const char *path;
		const char *language;
	} cases[] = {
		{"src/main.c", "c"}, {"dir.with.dot/main", NULL},
		{"src/main.CPP", "cpp"}, {"include/main.hxx", "cpp"},
		{"main.py", "python"}, {"main.tsx", "typescript"},
		{"main.MJS", "javascript"}, {"main.kts", "kotlin"},
		{"config.YML", "yaml"}, {"Cargo.toml", "toml"},
		{"dir/Makefile", "makefile"}, {"GNUmakefile", "makefile"},
		{".zshrc", "shell"}, {"C:\\src\\main.rs", "rust"},
		{"readme.md", NULL}, {"README.MARKDOWN", NULL},
		{"image.png", NULL}, {"notes.txt", NULL},
	};
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		const char *language = morph_md_highlight_language(cases[i].path, NULL, 0u);
		if (cases[i].language)
			assert(language && strcmp(language, cases[i].language) == 0);
		else
			assert(language == NULL);
	}
}

static void test_shebang_detection(void)
{
	static const struct {
		const char *code;
		const char *language;
	} cases[] = {
		{"#!/usr/bin/python3.13\nprint(42)\n", "python"},
		{"#!/usr/bin/env -S python3 -u\n", "python"},
		{"#!/usr/bin/env NODE_ENV=test node\n", "javascript"},
		{"#!/bin/bash\r\n", "bash"}, {"#!/bin/dash\n", "shell"},
		{"#! /usr/bin/lua5.4", "lua"}, {"#!/usr/bin/unknown\n", NULL},
		{"#!/usr/bin/env -u HOME python3\n", NULL},
		{"#", NULL}, {"#!", NULL}, {"plain text", NULL},
	};
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		const char *language = morph_md_highlight_language("script", cases[i].code,
			strlen(cases[i].code));
		if (cases[i].language)
			assert(language && strcmp(language, cases[i].language) == 0);
		else
			assert(language == NULL);
	}

	const char *python = "#!/usr/bin/env python3\n";
	assert(morph_md_highlight_language("README.MD", python, strlen(python)) == NULL);
	assert(morph_md_highlight_language("README.markdown", python, strlen(python)) == NULL);
	assert(strcmp(morph_md_highlight_language("main.c", python, strlen(python)), "c") == 0);
	const char bounded[] = {'#', '!', '/', 'b', 'i', 'n', '/', 's', 'h'};
	assert(strcmp(morph_md_highlight_language(NULL, bounded, sizeof(bounded)), "shell") == 0);
}

static void test_public_api_preserves_source_and_resets_style(void)
{
	const char *code = "\n\n\tint value = 42; /* first\n * \xe4\xb8\xad\xe6\x96\x87 */\n\n";
	char *result = NULL;
	size_t len = 0u;
	assert(morph_md_highlight_ansi("C", code, strlen(code), &result, &len) == 0);
	assert(len == strlen(result));
	assert(memcmp(result + len - 4u, "\033[0m", 4u) == 0);
	assert(strstr(result, "\n\033[2;36m * ") != NULL);
	const char *source = code;
	for (size_t i = 0; i < len; i++) {
		if (result[i] == '\033') {
			assert(result[++i] == '[');
			while (i < len && result[i] != 'm')
				i++;

			assert(i < len);
		} else {
			assert(*source && result[i] == *source++);
		}
	}

	assert(*source == '\0');
	free(result);
	assert(morph_md_highlight_ansi("unknown", code, strlen(code), &result, &len) == 0);
	assert(len == strlen(code) && memcmp(result, code, len + 1u) == 0);
	free(result);
	assert(morph_md_highlight_ansi(NULL, NULL, 0u, &result, NULL) == 0);
	assert(result[0] == '\0');
	free(result);
}

static void test_public_api_rejects_invalid_arguments(void)
{
	char original[] = "unchanged";
	char *result = original;
	size_t len = 99u;
	assert(morph_md_highlight_ansi("c", NULL, 1u, &result, &len) == -EINVAL);
	assert(result == original && len == 99u);
	assert(morph_md_highlight_ansi("c", "code", 4u, NULL, NULL) == -EINVAL);
	assert(morph_md_highlight_language(NULL, NULL, 1u) == NULL);
}

int main(void)
{
	test_c_tokens_are_colored();
	test_alias_and_unknown_language();
	test_long_line_is_not_truncated();
	test_file_language_detection();
	test_shebang_detection();
	test_public_api_preserves_source_and_resets_style();
	test_public_api_rejects_invalid_arguments();
	return 0;
}
