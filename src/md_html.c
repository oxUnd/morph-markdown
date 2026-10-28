#include "morph_markdown_html.h"
#include "md_doc.h"
#include "md_math_ext.h"
#include "base/md_buf.h"
#include "base/md_error.h"
#include <mathjax.h>
#include <ast.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static int escape(struct md_buf *out, const char *s)
{
    for (; s && *s; ++s) {
        const char *entity = *s == '&' ? "&amp;" : *s == '<' ? "&lt;" :
            *s == '>' ? "&gt;" : *s == '"' ? "&quot;" : NULL;
        int rc = entity ? md_buf_puts(out, entity) : md_buf_append(out, s, 1);
        if (rc) return rc;
    }
    return 0;
}

static int math_fence(struct md_buf *out, const char *fence)
{
    if (!fence || !*fence || !strcmp(fence, ".")) return 0;
    static const struct { const char *tex; const char *entity; } names[] = {
        {"\\langle", "&#x27e8;"}, {"\\rangle", "&#x27e9;"},
        {"\\lbrace", "{"}, {"\\rbrace", "}"}, {"\\{", "{"}, {"\\}", "}"},
        {"\\lvert", "|"}, {"\\rvert", "|"}, {"\\vert", "|"},
        {"\\Vert", "&#x2016;"}, {"\\|", "&#x2016;"},
        {"\\lfloor", "&#x230a;"}, {"\\rfloor", "&#x230b;"},
        {"\\lceil", "&#x2308;"}, {"\\rceil", "&#x2309;"}
    };
    int rc = md_buf_puts(out, "<mo fence=\"true\" stretchy=\"true\">");
    int found = 0;
    for (size_t i = 0; !rc && i < sizeof(names) / sizeof(names[0]); ++i) {
        if (!strcmp(fence, names[i].tex)) {
            rc = md_buf_puts(out, names[i].entity);
            found = 1;
            break;
        }
    }
    if (!rc && !found) rc = escape(out, fence);
    if (!rc) rc = md_buf_puts(out, "</mo>");
    return rc;
}

static int math_attrs(struct md_buf *out, const mjx_node *node)
{
    int rc = 0;
    for (const mjx_attr *a = node->attrs; a && !rc; a = a->next) {
        const char *name = a->name;
        if (!strcmp(name, "left") || !strcmp(name, "right") || !strcmp(name, "env")) continue;
        if (!strcmp(name, "row_space")) name = "rowspacing";
        if (!strcmp(name, "col_space")) name = "columnspacing";
        int alignment = node->type == MJX_NODE_MTABLE && !strcmp(name, "align");
        if (alignment) name = "columnalign";
        rc = md_buf_printf(out, " %s=\"", name);
        if (!rc && alignment) {
            for (const char *c = a->value; *c && !rc; ++c) {
                if (c != a->value) rc = md_buf_puts(out, " ");
                if (!rc) rc = md_buf_puts(out, *c == 'l' ? "left" : *c == 'r' ? "right" : "center");
            }
        } else if (!rc) rc = escape(out, a->value);
        if (!rc) rc = md_buf_puts(out, "\"");
    }
    return rc;
}

/* MathJax-C exposes its MathML AST; serialize nodes rather than interpreting TeX. */
static int math_node(struct md_buf *out, const mjx_node *node)
{
    static const char *variants[] = {"normal", "bold", "italic", "bold-italic",
        "double-struck", "fraktur", "bold-fraktur", "script", "bold-script",
        "sans-serif", "bold-sans-serif", "sans-serif-italic", "sans-serif-bold-italic",
        "monospace", "initial", "tailed", "looped", "stretched"};
    if (node->type == MJX_NODE_TEXT) return escape(out, node->text);
    const char *tag = mjx_node_type_name(node);
    if (node->type == MJX_NODE_TEXATOM || node->type == MJX_NODE_XML) tag = "mrow";
    int rc = md_buf_printf(out, "<%s", tag);
    if (!rc) rc = math_attrs(out, node);
    if (!rc && node->variant != MJX_VARIANT_NORMAL &&
        (size_t)node->variant < sizeof(variants) / sizeof(variants[0]) &&
        !mjx_node_get_attr(node, "mathvariant"))
        rc = md_buf_printf(out, " mathvariant=\"%s\"", variants[node->variant]);
    if (!rc) rc = md_buf_puts(out, ">");
    int fenced = node->type == MJX_NODE_MROW || node->type == MJX_NODE_TEXATOM;
    if (!rc && fenced) rc = math_fence(out, mjx_node_get_attr(node, "left"));
    if (!rc) rc = escape(out, node->text);
    for (size_t i = 0; !rc && i < mjx_node_child_count(node); ++i)
        rc = math_node(out, mjx_node_child(node, i));
    if (!rc && fenced) rc = math_fence(out, mjx_node_get_attr(node, "right"));
    if (!rc) rc = md_buf_printf(out, "</%s>", tag);
    return rc;
}

static int math_html(mjx_ctx *ctx, cmark_node *node, struct md_buf *out)
{
    const char *latex = morph_md_math_literal(node);
    int display = cmark_node_get_type(node) == MORPH_MD_NODE_MATH_BLOCK;
    mjx_node *math = mjx_parse_latex(ctx, latex, display ? MJX_STYLE_DISPLAY : MJX_STYLE_INLINE);
    if (!math) return MD_ERR_PARSE;
    int rc = md_buf_printf(out,
        "<math xmlns=\"http://www.w3.org/1998/Math/MathML\" class=\"morph-math morph-math-%s\" display=\"%s\"><semantics>",
        display ? "display" : "inline", display ? "block" : "inline");
    if (!rc) rc = math_node(out, math);
    if (!rc) rc = md_buf_puts(out, "<annotation encoding=\"application/x-tex\">");
    if (!rc) rc = escape(out, latex);
    if (!rc) rc = md_buf_puts(out, "</annotation></semantics></math>");
    /* The context owns the parsed tree until the next parse or teardown. */
    return rc;
}

static int prepare_nodes(cmark_node *parent, mjx_ctx **ctx,
                         const struct morph_md_doc *doc, const mjx_opts *opts)
{
    for (cmark_node *node = cmark_node_first_child(parent), *next; node; node = next) {
        next = cmark_node_next(node);
        cmark_node_type type = cmark_node_get_type(node);
        if (type == MORPH_MD_NODE_MATH_INLINE || type == MORPH_MD_NODE_MATH_BLOCK) {
            if (!*ctx) *ctx = mjx_init(opts);
            if (!*ctx) return MD_ERR_PARSE;
            struct md_buf html;
            md_buf_init(&html);
            int rc = math_html(*ctx, node, &html);
            cmark_node *replacement = rc ? NULL : cmark_node_new(CMARK_NODE_HTML_INLINE);
            if (!rc && (!replacement || !cmark_node_set_literal(replacement, html.data))) rc = MD_ERR_NOMEM;
            if (!rc && !cmark_node_replace(node, replacement)) rc = MD_ERR_PARSE;
            md_buf_cleanup(&html);
            if (rc) { cmark_node_free(replacement); return rc; }
            cmark_node_free(node);
        } else if ((type == CMARK_NODE_HTML_BLOCK || type == CMARK_NODE_HTML_INLINE) &&
                   doc->html_policy != MORPH_MD_HTML_PASSTHROUGH) {
            if (doc->html_policy == MORPH_MD_HTML_TEXT) {
                struct md_buf escaped;
                md_buf_init(&escaped);
                int rc = escape(&escaped, cmark_node_get_literal(node));
                if (!rc && !cmark_node_set_literal(node, escaped.data ? escaped.data : "")) rc = MD_ERR_NOMEM;
                md_buf_cleanup(&escaped);
                if (rc) return rc;
            } else { cmark_node_unlink(node); cmark_node_free(node); }
        } else {
            int rc = prepare_nodes(node, ctx, doc, opts);
            if (rc) return rc;
        }
    }
    return 0;
}

static int namespace_value(xmlNode *node, const char *attr, int tokens, int fragment)
{
    xmlChar *value = xmlGetProp(node, BAD_CAST attr);
    if (!value) return 0;
    const char *s = (const char *)value;
    if (fragment && *s != '#') { xmlFree(value); return 0; }
    struct md_buf out;
    md_buf_init(&out);
    int rc = fragment ? md_buf_puts(&out, "#") : 0;
    if (fragment) ++s;
    while (*s && !rc) {
        if (tokens && isspace((unsigned char)*s)) {
            rc = md_buf_append(&out, s++, 1);
            continue;
        }
        const char *start = s;
        while (*s && (!tokens || !isspace((unsigned char)*s))) ++s;
        if (strncmp(start, "morph-", 6)) rc = md_buf_puts(&out, "morph-");
        if (!rc) rc = md_buf_append(&out, start, (size_t)(s - start));
    }
    if (!rc && !xmlSetProp(node, BAD_CAST attr, BAD_CAST (out.data ? out.data : ""))) rc = MD_ERR_NOMEM;
    md_buf_cleanup(&out);
    xmlFree(value);
    return rc;
}

static int namespace_tree(xmlNode *node)
{
    static const char *refs[] = {"for", "headers", "aria-labelledby", "aria-describedby", "aria-controls", "aria-owns",
        "aria-activedescendant", "aria-details", "aria-errormessage", "list", "form", "itemref"};
    for (; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) continue;
        int rc = namespace_value(node, "class", 1, 0);
        if (!rc) rc = namespace_value(node, "id", 0, 0);
        if (!rc) rc = namespace_value(node, "href", 0, 1);
        if (!rc) rc = namespace_value(node, "xlink:href", 0, 1);
        for (size_t i = 0; !rc && i < sizeof(refs) / sizeof(refs[0]); ++i)
            rc = namespace_value(node, refs[i], 1, 0);
        if (!rc) rc = namespace_tree(node->children);
        if (rc) return rc;
    }
    return 0;
}

static int resolve_attribute(xmlNode *node, const char *attribute,
                             const struct morph_md_html_options *options)
{
    xmlChar *url = xmlGetProp(node, BAD_CAST attribute);
    if (!url) return 0;
    if (!url[0] || url[0] == '#') { xmlFree(url); return 0; }
    struct morph_md_html_resource resource = {
        (const char *)node->name, attribute, (const char *)url
    };
    const char *resolved = NULL;
    int rc = options->resolve_resource(&resource, &resolved, options->user_data);
    if (!rc && resolved && !xmlSetProp(node, BAD_CAST attribute, BAD_CAST resolved))
        rc = MD_ERR_NOMEM;
    xmlFree(url);
    return rc;
}

static int resolve_resources(xmlNode *node, const struct morph_md_html_options *options)
{
    if (!options || !options->resolve_resource) return 0;
    static const char *attributes[] = {"src", "poster", "href", "xlink:href"};
    for (; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) continue;
        int rc = 0;
        for (size_t i = 0; !rc && i < sizeof(attributes) / sizeof(attributes[0]); ++i)
            rc = resolve_attribute(node, attributes[i], options);
        if (!rc && xmlStrEqual(node->name, BAD_CAST "object"))
            rc = resolve_attribute(node, "data", options);
        if (!rc) rc = resolve_resources(node->children, options);
        if (rc) return rc;
    }
    return 0;
}

static int namespace_html(const char *html,
                          const struct morph_md_html_options *options, char **result)
{
    size_t len = strlen(html);
    if (len > INT_MAX) return MD_ERR_INVALID;
    if (!len) {
        struct md_buf empty;
        md_buf_init(&empty);
        int rc = md_buf_puts(&empty, "<div class=\"morph-markdown\"></div>\n");
        if (!rc) *result = md_buf_detach(&empty);
        md_buf_cleanup(&empty);
        return rc;
    }
    htmlDocPtr tree = htmlReadMemory(html, (int)len, NULL, "UTF-8",
        HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
    if (!tree) return MD_ERR_PARSE;
    xmlNode *root = xmlDocGetRootElement(tree), *body = root ? root->children : NULL;
    while (body && !xmlStrEqual(body->name, BAD_CAST "body")) body = body->next;
    int rc = body ? resolve_resources(body->children, options) : 0;
    if (!rc) rc = namespace_tree(root);
    xmlBufferPtr buffer = xmlBufferCreate();
    if (!buffer) rc = MD_ERR_NOMEM;
    if (!rc && body) {
        for (xmlNode *n = body->children; n; n = n->next)
            if (htmlNodeDump(buffer, tree, n) < 0) { rc = MD_ERR_NOMEM; break; }
    }
    struct md_buf out;
    md_buf_init(&out);
    if (!rc) rc = md_buf_puts(&out, "<div class=\"morph-markdown\">");
    if (!rc) rc = md_buf_append(&out, (const char *)xmlBufferContent(buffer), (size_t)xmlBufferLength(buffer));
    if (!rc) rc = md_buf_puts(&out, "</div>\n");
    if (!rc) *result = md_buf_detach(&out);
    md_buf_cleanup(&out);
    if (buffer) xmlBufferFree(buffer);
    xmlFreeDoc(tree);
    return rc;
}

int morph_md_doc_to_html(const struct morph_md_doc *doc,
                         const struct morph_md_html_options *options, char **out_html)
{
    if (!out_html) return MD_ERR_INVALID;
    *out_html = NULL;
    if (!doc) return MD_ERR_INVALID;
    mjx_opts opts = {0};
    opts.font_path = options && options->math_font_path ? options->math_font_path : MORPH_HTML_MATH_FONT;
    opts.font_size = 18;
    opts.dpi = 96;
    cmark_node *tree = morph_md_parse(doc->markdown, doc->markdown_len,
        (doc->features & MORPH_MD_FEATURE_GFM) != 0, (doc->features & MORPH_MD_FEATURE_MATH) != 0,
        (doc->features & MORPH_MD_FEATURE_GFM) ? CMARK_OPT_FOOTNOTES : 0);
    if (!tree) return MD_ERR_NOMEM;
    mjx_ctx *ctx = NULL;
    int rc = prepare_nodes(tree, &ctx, doc, &opts);
    cmark_llist *extensions = NULL;
    cmark_mem *mem = cmark_get_default_mem_allocator();
    if (doc->features & MORPH_MD_FEATURE_GFM) {
        cmark_syntax_extension *filter = cmark_find_syntax_extension("tagfilter");
        if (filter) extensions = cmark_llist_append(mem, NULL, filter);
    }
    char *html = rc ? NULL : cmark_render_html(tree, CMARK_OPT_UNSAFE, extensions);
    cmark_llist_free(mem, extensions);
    if (!rc) rc = html ? namespace_html(html, options, out_html) : MD_ERR_NOMEM;
    free(html);
    if (ctx) mjx_free(ctx);
    cmark_node_free(tree);
    return rc;
}

const char *morph_md_html_styles(void)
{
    return ".morph-markdown{line-height:1.6;overflow-wrap:break-word}"
        ".morph-markdown pre{overflow:auto;padding:1em;background:#f6f8fa}"
        ".morph-markdown code{font-family:monospace}"
        ".morph-markdown table{border-collapse:collapse;display:block;overflow:auto}"
        ".morph-markdown th,.morph-markdown td{border:1px solid #d0d7de;padding:.4em .8em}"
        ".morph-markdown blockquote{margin-left:0;padding-left:1em;border-left:3px solid #d0d7de}"
        ".morph-markdown .morph-math-inline{vertical-align:middle;white-space:nowrap}"
        ".morph-markdown .morph-math-display{text-align:center;margin:1em 0;overflow:auto}"
        ".morph-markdown .morph-math{font-size:1em}";
}
