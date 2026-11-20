#ifndef MORPH_MARKDOWN_HIGHLIGHT_H
#define MORPH_MARKDOWN_HIGHLIGHT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Detect a supported language from a path's basename/extension, then a shebang.
 * Returns a borrowed static name, or NULL for unknown files (including Markdown).
 * code may be NULL when code_len is zero. No file I/O is performed. */
const char *morph_md_highlight_language(const char *path,
				      const char *code, size_t code_len);

/* Highlight a complete source buffer without Markdown parsing or layout.
 * Returns 0, -EINVAL for invalid arguments, or -ENOMEM on allocation failure.
 * On success, *out is malloc-owned and NUL-terminated; release it with free().
 * out_len, if non-NULL, excludes the NUL. Unknown/NULL languages pass through.
 * Recognized languages end with an SGR reset. Source bytes and whitespace are
 * preserved; callers must sanitize untrusted terminal controls BEFORE this call.
 * code may be NULL when code_len is zero. Outputs are unchanged on failure. */
int morph_md_highlight_ansi(const char *language,
			    const char *code, size_t code_len,
			    char **out, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif
