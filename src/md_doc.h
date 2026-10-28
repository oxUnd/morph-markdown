#ifndef MORPH_MD_DOC_H
#define MORPH_MD_DOC_H
#include "morph_markdown.h"
#include <cmark-gfm.h>
struct morph_md_doc {
    char *json;
    char *markdown;
    size_t markdown_len;
    uint32_t features;
    enum morph_md_html_policy html_policy;
};
cmark_node *morph_md_parse(const char *md, size_t len, int gfm, int math, int extra_options);
#endif
