#include "morph_markdown_html.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <libxml/HTMLparser.h>

static char *render(const char *md, enum morph_md_html_policy policy)
{
    struct morph_md_engine_options options = {0};
    options.features = MORPH_MD_FEATURE_GFM | MORPH_MD_FEATURE_MATH;
    options.html_policy = policy;
    struct morph_md_engine *engine = NULL;
    struct morph_md_doc *doc = NULL;
    char *html = NULL;
    assert(morph_md_engine_create(&options, &engine) == 0);
    assert(morph_md_engine_append(engine, md, strlen(md), 1) == 0);
    assert(morph_md_engine_snapshot(engine, &doc) == 0);
    morph_md_engine_destroy(engine);
    assert(morph_md_doc_to_html(doc, NULL, &html) == 0);
    morph_md_doc_release(doc);
    return html;
}

static void check_names(xmlNode *node)
{
    for (; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) continue;
        const char *attrs[] = {"id", "class"};
        for (size_t i = 0; i < 2; ++i) {
            xmlChar *value = xmlGetProp(node, BAD_CAST attrs[i]);
            if (!value) continue;
            char *token = strtok((char *)value, " \t\n\r");
            while (token) {
                assert(strncmp(token, "morph-", 6) == 0);
                token = strtok(NULL, " \t\n\r");
            }
            xmlFree(value);
        }
        check_names(node->children);
    }
}

static void test_complete(void)
{
    char *html = render(
        "# Title\n\n**bold** *em* ~~strike~~ `a<b` [link](https://example.com) ![alt](x.png)\n\n"
        "> quote\n\n---\n\n1. first\n2. second\n\n- [x] done\n- [ ] todo\n\n"
        "```c\nint x = 1 < 2;\n```\n\n"
        "| left | right |\n| :--- | ---: |\n| $x^2$ | text |\n\n"
        "Inline $\\frac{1}{2}$ and $x+1$.\n\n$$\\sum_{i=1}^n i$$\n\n"
        "Footnote[^1].\n\n[^1]: note\n\n"
        "<div id=\"anchor\" class=\"custom morph-ready\"><a href=\"#anchor\">raw</a></div>\n",
        MORPH_MD_HTML_PASSTHROUGH);
    assert(strstr(html, "<h1>Title</h1>"));
    assert(strstr(html, "<table>"));
    assert(strstr(html, "<del>strike</del>"));
    assert(strstr(html, "checkbox"));
    assert(strstr(html, "morph-language-c"));
    assert(strstr(html, "int x = 1 &lt; 2;"));
    assert(strstr(html, "<math "));
    assert(strstr(html, "<mfrac>"));
    assert(strstr(html, "application/x-tex"));
    assert(strstr(html, "morph-math-display"));
    assert(strstr(html, "id=\"morph-anchor\""));
    assert(strstr(html, "href=\"#morph-anchor\""));
    assert(strstr(html, "morph-custom morph-ready"));
    assert(strstr(html, "morph-footnote"));
    htmlDocPtr doc = htmlReadMemory(html, (int)strlen(html), NULL, "UTF-8", HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    assert(doc);
    check_names(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);
    morph_md_free(html);
}

static void test_policy(void)
{
    char *html = render("<b id=\"raw\">text</b>\n", MORPH_MD_HTML_TEXT);
    assert(strstr(html, "&lt;b"));
    morph_md_free(html);
    html = render("<b>text</b>\n", MORPH_MD_HTML_STRIP);
    assert(!strstr(html, "<b>"));
    assert(strstr(html, "text"));
    morph_md_free(html);
    html = render("", MORPH_MD_HTML_PASSTHROUGH);
    assert(strstr(html, "morph-markdown"));
    morph_md_free(html);
    assert(morph_md_doc_to_html(NULL, NULL, &html) != 0 && !html);
}

static void test_streaming_and_math(void)
{
    struct morph_md_engine_options options = {0};
    options.features = MORPH_MD_FEATURE_GFM | MORPH_MD_FEATURE_MATH;
    struct morph_md_engine *engine = NULL;
    struct morph_md_doc *before = NULL, *after = NULL;
    assert(morph_md_engine_create(&options, &engine) == 0);
    const char *first = "First $x_1$\n\n";
    assert(morph_md_engine_append(engine, first, strlen(first), 0) == 0);
    assert(morph_md_engine_snapshot(engine, &before) == 0);
    const char *last = "$$\\begin{pmatrix}a&b\\\\c&d\\end{pmatrix}$$\n\n"
        "<label for=\"target\" aria-labelledby=\"one two\">label</label>\n\n"
        "<input id=\"target\" class=\"one  two\">\n";
    assert(morph_md_engine_append(engine, last, strlen(last), 1) == 0);
    assert(morph_md_engine_snapshot(engine, &after) == 0);
    morph_md_engine_destroy(engine);
    char *html = NULL;
    assert(morph_md_doc_to_html(before, NULL, &html) == 0);
    assert(strstr(html, "<msub>"));
    assert(!strstr(html, "<mtable"));
    morph_md_free(html);
    assert(morph_md_doc_to_html(after, NULL, &html) == 0);
    assert(strstr(html, "<mtable"));
    assert(strstr(html, "<mtr>"));
    assert(strstr(html, "columnspacing=\"0.7em\""));
    assert(strstr(html, "stretchy=\"true\">(</mo>"));
    assert(strstr(html, "for=\"morph-target\""));
    assert(strstr(html, "aria-labelledby=\"morph-one morph-two\""));
    morph_md_free(html);
    morph_md_doc_release(before);
    morph_md_doc_release(after);
}

struct resource_state {
    int calls;
    int images;
    int media;
    int links;
    int fail;
};

static int resolve_resource(const struct morph_md_html_resource *resource,
                            const char **out_url, void *user_data)
{
    struct resource_state *state = user_data;
    ++state->calls;
    assert(resource->url[0] != '#');
    assert(*out_url == NULL);
    if (state->fail) return -42;
    if (!strcmp(resource->tag, "img")) {
        ++state->images;
        assert(!strcmp(resource->attribute, "src"));
        assert(!strcmp(resource->url, "original.png"));
        *out_url = "assets/image.png?x=1&label=\"<safe>\"";
    } else if (!strcmp(resource->tag, "a")) {
        ++state->links;
        /* NULL preserves the original URL. */
    } else {
        ++state->media;
        *out_url = "https://cdn.example.com/media";
    }
    return 0;
}

static void test_resources(void)
{
    const char *md = "![alt](original.png) [site](https://example.com) [local](#anchor)\n\n"
        "<video poster=\"poster.png\"><source src=\"movie.mp4\"></video>\n"
        "<audio src=\"audio.mp3\"></audio><object data=\"file.pdf\"></object>\n"
        "<img src=\"original.png\">\n";
    struct morph_md_engine_options engine_options = {0};
    engine_options.features = MORPH_MD_FEATURE_GFM;
    struct morph_md_engine *engine = NULL;
    struct morph_md_doc *doc = NULL;
    assert(morph_md_engine_create(&engine_options, &engine) == 0);
    assert(morph_md_engine_append(engine, md, strlen(md), 1) == 0);
    assert(morph_md_engine_snapshot(engine, &doc) == 0);
    morph_md_engine_destroy(engine);
    struct resource_state state = {0};
    struct morph_md_html_options options = {0};
    options.resolve_resource = resolve_resource;
    options.user_data = &state;
    char *html = NULL;
    assert(morph_md_doc_to_html(doc, &options, &html) == 0);
    assert(state.calls == 7 && state.images == 2 && state.media == 4 && state.links == 1);
    assert(strstr(html, "assets/image.png?x=1&amp;label=%22&lt;safe&gt;%22"));
    assert(strstr(html, "https://cdn.example.com/media"));
    assert(strstr(html, "href=\"https://example.com\""));
    assert(strstr(html, "href=\"#morph-anchor\""));
    morph_md_free(html);
    state.fail = 1;
    html = (char *)&state;
    assert(morph_md_doc_to_html(doc, &options, &html) == -42);
    assert(html == NULL);
    assert(state.calls == 8);
    assert(morph_md_doc_to_html(doc, NULL, &html) == 0);
    assert(strstr(html, "src=\"original.png\""));
    assert(!strstr(html, "cdn.example.com"));
    morph_md_free(html);
    morph_md_doc_release(doc);
}

int main(void)
{
    test_complete();
    test_policy();
    test_resources();
    test_streaming_and_math();
    puts("HTML tests passed");
    return 0;
}
