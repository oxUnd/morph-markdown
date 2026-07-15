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
- `demo/android/` Android app demonstrating SDK integration.
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
- Both SDKs keep math and image rendering pluggable.

## Current gaps

- iOS native C bridge adapter is still pending; the SwiftUI SDK currently
  consumes a `MorphMarkdownNativeEngine` protocol.
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
./build/morph-md-kitty-demo
```

The demo simulates streamed model chunks, clears and redraws the Markdown view,
and renders LaTeX through `.third_party/mathjax-c` with Kitty graphics protocol.
Formula font size follows the current terminal cell height by default, so
inline math matches Kitty's body text size instead of using a hardcoded display
size.
The table fixture is chunked by header, separator, and data rows so the terminal
view shows the table growing row by row as model output streams in.
The Markdown fixture behind the demo is mirrored in
`demo/streaming_math.md` for manual inspection and future mobile migration.

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
