#ifndef MORPH_MARKDOWN_HTML_H
#define MORPH_MARKDOWN_HTML_H
#include "morph_markdown.h"
#ifdef __cplusplus
extern "C" {
#endif
/* A single URL attribute in the exported DOM (Markdown or passthrough HTML).
 * All strings are borrowed and valid only during the callback. */
struct morph_md_html_resource {
    const char *tag;
    const char *attribute;
    const char *url;
};
/* Return 0 to continue, nonzero to abort export with that error code.
 * Leave *out_url NULL to preserve the URL. Otherwise supply a borrowed string,
 * copied immediately after return and valid until the next callback invocation.
 * No allocation/ownership transfer is required. Values are HTML-escaped by the
 * exporter. Called synchronously once per single-URL attribute per export;
 * repeated exports invoke it again. The exporter never fetches resources.
 * Covers src, poster, object data, href and xlink:href; skips fragment-only
 * references. CSS URLs and srcset lists are outside this callback's scope. */
typedef int (*morph_md_html_resolve_resource_fn)(
    const struct morph_md_html_resource *resource,
    const char **out_url, void *user_data);

/* Link morph-markdown-html. Output is a UTF-8 HTML fragment, freed with
 * morph_md_free. Raw HTML follows the engine policy; passthrough is trusted
 * content, not sanitized. Every class/id token is namespaced with morph-.
 * MathJax-C parses TeX into its MathML AST, embedded as native HTML MathML.
 * NULL options use the bundled MathJax-C font for parser initialization.
 * MathML presentation inherits the surrounding HTML font size and color. */
struct morph_md_html_options {
    const char *math_font_path;
    morph_md_html_resolve_resource_fn resolve_resource;
    void *user_data;
};
int morph_md_doc_to_html(const struct morph_md_doc *doc,
                         const struct morph_md_html_options *options,
                         char **out_html);
/* Optional scoped default styles; static lifetime. */
const char *morph_md_html_styles(void);
#ifdef __cplusplus
}
#endif
#endif
