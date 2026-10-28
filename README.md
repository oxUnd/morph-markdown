# morph markdown-render

Small native Markdown streaming core for iOS and Android.

## Layout

- `include/` public C ABI used by Swift/JNI bindings.
- `src/base/` internal foundations: errors, buffers, arrays, hashing.
- `src/base/md_strmap.*` provides internal string-keyed lookup support.
- `src/base/md_utf8.*` keeps streamed input on complete UTF-8 boundaries.
- `src/md_stream.c` streaming coordinator and cmark-gfm IR conversion.
- `sdk/android/morph-markdown/` Android View SDK and JNI bridge.
- `sdk/ios/MorphMarkdown/` SwiftUI SDK surface and IR renderer.
- `sdk/kitty/morph-markdown/` terminal SDK using the Kitty graphics protocol.
- `demo/android/` Android app demonstrating SDK integration.
- `demo/kitty/` streaming terminal demo using the Kitty SDK.
- `tests/` C tests for streaming, GFM, math toggles, and snapshots.

## Design

The C core does not render platform UI. It accepts streamed Markdown bytes,
emits typed events, and can snapshot the current Markdown document. Platform
SDKs render the serialized IR with native views.

- Parser: `cmark-gfm`, with GFM extensions enabled by default.
- Streaming: sealed prefix plus mutable tail buffer. Sealed prefixes emit one
  `insert` event per top-level block plus a `seal` event with offset, length, and
  content hash metadata.
- Images: emitted as Markdown image/link nodes; platform adapters load images.
  Kitty hosts can configure `load_image` / `release_image` to resolve any
  supported format or remote URL to a temporary local PNG. The SDK caches the
  result and owns all image placement, including table cells and streaming.
  Failed loads remain placeholders in the original position; the loader must
  never write terminal output. Temporary files are released on renderer destroy.
- Math: optional IR split into `math_inline` and `math_block` nodes. Rendering
  is a platform plugin responsibility.
- HTML: controlled by `html_policy`: passthrough, strip, or downgrade to text.
- API: create `morph_md_engine`, append chunks with `morph_md_engine_append`,
  snapshot with `morph_md_engine_snapshot`, and serialize with
  `morph_md_doc_to_json`.
- Events: `morph_md_engine_options.on_event` receives `morph_md_event`;
  `morph_md_event_to_json` is available for bindings that want JSON patches.
- Stats: `morph_md_engine_get_stats` exposes sealed/tail/pending byte counts and
  sealed block count for memory monitoring.
- cmark-gfm `footnotes` is enabled with the GFM extension set; dedicated
  footnote IR nodes are still pending because the exposed AST maps references
  through link-shaped nodes.
- Memory: variable strings use `md_buf`; unbounded collections should use
  `md_array`; string-keyed lookups should use `md_strmap`; hash IDs should use
  `md_hash`.

## SDKs

- Android SDK exposes `MorphMarkdownView`, `MorphMarkdownEngine`,
  `MorphMarkdownTheme`, `MorphMathRenderer`, and `MorphImageLoader`.
- Android demo depends on the SDK module; it no longer owns reusable Markdown
  rendering code.
- iOS SDK exposes a SwiftUI `MorphMarkdownView`, `MorphMarkdownEngine`,
  `MorphMarkdownTheme`, and math/image plugin protocols.
- Kitty SDK exposes a C streaming renderer with configurable output callbacks;
  math is emitted as native-size RGBA through the Kitty graphics protocol.
  Content padding can be configured independently for all four terminal edges.
- Both SDKs keep math and image rendering pluggable.
- iOS also exposes `MorphMarkdownUIView` with `.scrollable` and
  `.intrinsicHeight` layout modes. SwiftUI defaults to intrinsic-height mode;
  UIKit defaults to internal scrolling.
- iOS native parsing and MathJax rendering use the bundled C bridge. Prepare the
  local XCFramework with `sdk/ios/MorphMarkdown/scripts/build-native-ios.sh`.
- Release maintainers generate a URL/checksum binary manifest with
  `scripts/prepare-release-package.sh <version> <release-base-url>` from the
  iOS package directory.

## Current gaps

- Stable-prefix detection is conservative and blank-line/block-boundary based;
  finer line-level stability rules are pending.
- Dedicated footnote presentation nodes are not modeled yet.

## Kitty Demo

Prepare third-party sources inside this checkout:

```sh
./scripts/prepare-third-party.sh
```

Build:

```sh
cmake -S . -B build
cmake --build build
```

Run inside Kitty or a terminal compatible with Kitty graphics protocol:

```sh
./build/demo/kitty/morph-md-kitty-demo
```

The demo simulates streamed model chunks, appends stable Markdown at the
current cursor position, and renders LaTeX through `.third_party/mathjax-c`
with Kitty graphics protocol. Existing terminal scrollback remains intact
while only the mutable tail is refreshed.
Formula font size follows the current terminal cell height by default, so
inline math matches Kitty's body text size instead of using a hardcoded display
size.
The table fixture is chunked by header, separator, and data rows so the terminal
view shows the table growing row by row as model output streams in.
The Markdown fixture behind the demo is mirrored in
`demo/kitty/streaming_math.md` for manual inspection.

Functions should stay below 200 lines. Keep new generic data structures in
`src/base/` instead of embedding local one-off containers in feature files.

## Third-party Dependencies

This repository must build without paths outside its checkout. MathJax-C is
downloaded to `.third_party/mathjax-c` by `scripts/prepare-third-party.sh`.
Android FreeType/HarfBuzz libraries are generated on demand inside
`sdk/android/morph-markdown/.build/vendor-android/<abi>` by:

```sh
sdk/android/morph-markdown/scripts/prepare-android-deps.sh
```

Both locations are build artifacts and are ignored by git.

## HTML export

Build the `morph-markdown-html` target (enabled by default) after preparing
MathJax-C. It requires FreeType, HarfBuzz and LibXml2. Disable it with
`-DMORPH_MARKDOWN_BUILD_HTML=OFF` for a core-only build.

```sh
./scripts/prepare-third-party.sh
cmake -S . -B build -DMORPH_MARKDOWN_BUILD_HTML=ON
cmake --build build
./build/morph-md-html-demo < article.md > article.html
```

The demo produces a standalone HTML document with scoped default styles.
For embedding, include `morph_markdown_html.h` and link `morph-markdown-html`:

```c
char *html = NULL;
int rc = morph_md_doc_to_html(doc, NULL, &html);
if (rc == 0) {
    /* Embed html; morph_md_html_styles() supplies optional default CSS. */
    morph_md_free(html);
}
```

Export works on intermediate streaming snapshots and finished documents.
Snapshots retain their source independently of the engine. The fragment has a
`morph-markdown` wrapper. Standard Markdown, GFM tables/alignment, task lists,
strikethrough, autolinks, images, links, code blocks/language classes, inline
code and footnotes use cmark-gfm's HTML renderer. Code is HTML-escaped; syntax
coloring is left to the host.

MathJax-C parses inline `$...$` and display `$$...$$` TeX into its structured
MathML AST, which is
serialized directly into native `<math>` HTML, including attributes, variants,
fractions, scripts and matrices. Original TeX is preserved in an annotation.
There is no browser JavaScript dependency or formula bitmap. Presentation uses
the browser's native MathML support and surrounding font size/color. The pinned
MathJax-C has no native HTML renderer; this exporter serializes its public AST.
The default parser font comes from the prepared checkout; set
`morph_md_html_options.math_font_path` when distributing an executable separately.

Every class/id attribute token receives `morph-` unless already prefixed,
including raw HTML, language classes, formula attributes and footnote anchors.
Fragment links, label references, table headers and ARIA ID references are
updated together. Raw HTML follows the engine's passthrough/strip/text policy;
passthrough is for trusted content and is not a sanitizer. Embedded CSS and
JavaScript are not rewritten; author their selectors using prefixed names.

### Resource URL callback

Configure `resolve_resource` and `user_data` in `morph_md_html_options` to map
images and other resource URLs to a CDN, local assets, signed URLs or data URIs:

```c
#include <string.h>

static int resolve_resource(const struct morph_md_html_resource *resource,
                            const char **out_url, void *user_data)
{
    (void)user_data;
    if (strcmp(resource->tag, "img") == 0 &&
        strcmp(resource->attribute, "src") == 0 &&
        strcmp(resource->url, "images/logo.png") == 0)
        *out_url = "https://cdn.example.com/logo.png";
    return 0;
}

struct morph_md_html_options options = {0};
options.resolve_resource = resolve_resource;
options.user_data = NULL;
char *html = NULL;
int rc = morph_md_doc_to_html(doc, &options, &html);
/* Use html on success, then morph_md_free(html). */
```

The callback runs synchronously for each single-URL attribute in emitted
Markdown/passthrough HTML: `src`, `poster`, `object[data]`, `href` and
`xlink:href`. Inspect the tag and attribute to distinguish images, video/audio,
links and other resources. Empty URLs and fragment-only anchors are skipped;
rewritten fragment references still receive the `morph-` namespace.

`*out_url` starts as NULL: leave it NULL to preserve the original URL, or return
a borrowed replacement string. The exporter copies it immediately, escapes it
for HTML and does not free it. A scratch buffer in `user_data` can be reused at
the next invocation. Return a nonzero error code to abort export; `*out_html`
remains NULL. Each snapshot export invokes callbacks again; callers can cache
resource mappings in `user_data`. No callback means existing URLs are preserved.

The exporter does not download resources; fetching, caching and embedding data
URIs belong to the caller. CSS `url(...)` and `srcset` lists are not single-URL
attributes and are not passed to this callback.
