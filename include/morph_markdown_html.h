#ifndef MORPH_MARKDOWN_HTML_H
#define MORPH_MARKDOWN_HTML_H
#include "morph_markdown.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Link morph-markdown-html. Output is a UTF-8 HTML fragment, freed with
 * morph_md_free. Raw HTML follows the engine policy; passthrough is trusted
 * content, not sanitized. Every class/id token is namespaced with morph-.
 * MathJax-C parses TeX into its MathML AST, embedded as native HTML MathML.
 * NULL options use the bundled MathJax-C font for parser initialization.
 * MathML presentation inherits the surrounding HTML font size and color. */
struct morph_md_html_options {
    const char *math_font_path;
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
