#include "morph_markdown_html.h"
#include <stdio.h>

int main(void)
{
    struct morph_md_engine_options options = {0};
    options.features = MORPH_MD_FEATURE_GFM | MORPH_MD_FEATURE_MATH;
    options.html_policy = MORPH_MD_HTML_PASSTHROUGH;
    struct morph_md_engine *engine = NULL;
    struct morph_md_doc *doc = NULL;
    char *html = NULL;
    int rc = morph_md_engine_create(&options, &engine);
    char input[4096];
    size_t len;
    while (!rc && (len = fread(input, 1, sizeof(input), stdin)) > 0)
        rc = morph_md_engine_append(engine, input, len, 0);
    if (!rc && ferror(stdin)) rc = -1;
    if (!rc) rc = morph_md_engine_append(engine, NULL, 0, 1);
    if (!rc) rc = morph_md_engine_snapshot(engine, &doc);
    if (!rc) rc = morph_md_doc_to_html(doc, NULL, &html);
    if (!rc) {
        puts("<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>morph-markdown</title><style>");
        puts(morph_md_html_styles());
        puts("</style></head><body>");
        puts(html);
        puts("</body></html>");
        if (ferror(stdout)) rc = -1;
    }
    morph_md_free(html);
    morph_md_doc_release(doc);
    morph_md_engine_destroy(engine);
    if (rc) fprintf(stderr, "HTML export failed: %d\n", rc);
    return rc ? 1 : 0;
}
