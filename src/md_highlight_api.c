#include "morph_markdown_highlight.h"

#include "md_highlight.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char *path_basename(const char *path)
{
	const char *name = path;

	for (; *path; path++) {
		if (*path == '/' || *path == '\\')
			name = path + 1;
	}

	return name;
}

static int lowercase_name(char *lower, size_t capacity, const char *name, size_t len)
{
	if (len >= capacity)
		return 0;

	for (size_t i = 0; i < len; i++)
		lower[i] = name[i] >= 'A' && name[i] <= 'Z'
			? (char)(name[i] + ('a' - 'A')) : name[i];

	lower[len] = '\0';
	return 1;
}

static const char *normalized_language(const char *name, size_t len)
{
	char lower[32];

	if (!lowercase_name(lower, sizeof(lower), name, len))
		return NULL;

	return morph_md_highlight_find_language(lower, len);
}

static const char *filename_language(const char *path)
{
	const char *name = path_basename(path);
	static const struct {
		const char *name;
		const char *language;
	} filenames[] = {
		{"Makefile", "makefile"}, {"makefile", "makefile"},
		{"GNUmakefile", "makefile"}, {".bashrc", "bash"},
		{".bash_profile", "bash"}, {".zshrc", "shell"},
		{".zshenv", "shell"}, {".profile", "shell"},
	};

	for (size_t i = 0; i < sizeof(filenames) / sizeof(filenames[0]); i++) {
		if (strcmp(name, filenames[i].name) == 0)
			return filenames[i].language;
	}

	const char *extension = strrchr(name, '.');
	if (extension) {
		static const struct {
			const char *extension;
			const char *language;
		} aliases[] = {
			{"hh", "cpp"}, {"hxx", "cpp"}, {"pyi", "python"},
			{"mjs", "javascript"}, {"cjs", "javascript"},
			{"mts", "typescript"}, {"cts", "typescript"}, {"kts", "kotlin"},
		};
		char lower[32];
		if (!lowercase_name(lower, sizeof(lower), extension + 1, strlen(extension + 1)))
			return NULL;

		for (size_t i = 0; i < sizeof(aliases) / sizeof(aliases[0]); i++) {
			if (strcmp(lower, aliases[i].extension) == 0)
				return aliases[i].language;
		}
	}

	return extension ? normalized_language(extension + 1, strlen(extension + 1)) : NULL;
}

static int next_shebang_word(const char **cursor, const char *end,
			     const char **word, size_t *len)
{
	while (*cursor < end && (**cursor == ' ' || **cursor == '\t'))
		(*cursor)++;

	*word = *cursor;
	while (*cursor < end && **cursor != ' ' && **cursor != '\t')
		(*cursor)++;

	*len = (size_t)(*cursor - *word);
	return *len != 0u;
}

static const char *interpreter_language(const char *word, size_t len)
{
	const char *name = word;
	for (size_t i = 0; i < len; i++) {
		if (word[i] == '/')
			name = word + i + 1u;
	}

	len -= (size_t)(name - word);
	word = name;
	if ((len == 4u && memcmp(word, "node", 4u) == 0) ||
	    (len == 6u && memcmp(word, "nodejs", 6u) == 0))
		return "javascript";

	if ((len == 4u && memcmp(word, "dash", 4u) == 0) ||
	    (len == 3u && memcmp(word, "ksh", 3u) == 0))
		return "shell";

	/* Versioned interpreters, such as python3.13 and lua5.4. */
	size_t name_len = 0u;
	while (name_len < len && (word[name_len] < '0' || word[name_len] > '9'))
		name_len++;

	for (size_t i = name_len; i < len; i++) {
		if ((word[i] < '0' || word[i] > '9') && word[i] != '.')
			return NULL;
	}

	return normalized_language(word, name_len);
}

static const char *shebang_language(const char *code, size_t len)
{
	if (len < 2u || code[0] != '#' || code[1] != '!')
		return NULL;

	const char *cursor = code + 2;
	const char *end = cursor;
	while ((size_t)(end - code) < len && *end != '\n' && *end != '\r')
		end++;

	const char *word;
	size_t word_len;
	if (!next_shebang_word(&cursor, end, &word, &word_len))
		return NULL;

	int env = word_len >= 3u && memcmp(word + word_len - 3u, "env", 3u) == 0 &&
		(word_len == 3u || word[word_len - 4u] == '/');
	if (!env)
		return interpreter_language(word, word_len);

	while (next_shebang_word(&cursor, end, &word, &word_len)) {
		if ((word_len == 2u && memcmp(word, "-S", 2u) == 0) ||
		    (word_len == 2u && memcmp(word, "--", 2u) == 0) ||
		    memchr(word, '=', word_len))
			continue;

		return word[0] == '-' ? NULL : interpreter_language(word, word_len);
	}

	return NULL;
}

const char *morph_md_highlight_language(const char *path,
				      const char *code, size_t code_len)
{
	const char *language = path ? filename_language(path) : NULL;

	if (language || (!code && code_len))
		return language;

	/* Markdown keeps its own rendering/source path, even with a shebang. */
	const char *extension = path ? strrchr(path_basename(path), '.') : NULL;
	char lower[32];
	if (extension && lowercase_name(lower, sizeof(lower), extension, strlen(extension)) &&
	    (strcmp(lower, ".md") == 0 || strcmp(lower, ".markdown") == 0))
		return NULL;

	return shebang_language(code, code_len);
}

int morph_md_highlight_ansi(const char *language,
			    const char *code, size_t code_len,
			    char **out, size_t *out_len)
{
	if (!out || (!code && code_len))
		return -EINVAL;

	const char *name = language ? normalized_language(language, strlen(language)) : NULL;
	const char *source = code ? code : "";
	struct morph_md_sbuf output;
	morph_md_sbuf_init(&output, NULL, 0u);
	morph_md_highlight_code(name ? name : "", name ? strlen(name) : 0u,
		source, code_len, &output, NULL, NULL);

	const size_t reset_len = name ? 4u : 0u;
	if (output.len > SIZE_MAX - reset_len - 1u)
		return -ENOMEM;

	char *result = malloc(output.len + reset_len + 1u);
	if (!result)
		return -ENOMEM;

	morph_md_sbuf_init(&output, result, output.len + reset_len + 1u);
	morph_md_highlight_code(name ? name : "", name ? strlen(name) : 0u,
		source, code_len, &output, NULL, NULL);
	if (name)
		morph_md_sbuf_append(&output, "\033[0m");

	result[output.len] = '\0';
	*out = result;
	if (out_len)
		*out_len = output.len;

	return 0;
}
