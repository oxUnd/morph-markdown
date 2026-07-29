#include "md_highlight.h"

#include <assert.h>
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
	free(code);
}

int main(void)
{
	test_c_tokens_are_colored();
	test_alias_and_unknown_language();
	test_long_line_is_not_truncated();
	return 0;
}
