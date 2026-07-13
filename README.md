# morph markdown-render

Small native Markdown streaming core for iOS and Android.

## Layout

- `include/` public C ABI used by Swift/JNI bindings.
- `src/base/` internal foundations: errors, buffers, arrays, hashing.
- `src/base/md_strmap.*` provides internal string-keyed lookup support.
- `src/base/md_utf8.*` keeps streamed input on complete UTF-8 boundaries.
- `src/md_stream.c` streaming coordinator and cmark-gfm IR conversion.
- `tests/` C tests for streaming, GFM, math toggles, and snapshots.

## Design

The core does not render platform UI. It accepts streamed Markdown bytes and
emits JSON IR patches. iOS and Android render those patches with native views.

- Parser: `cmark-gfm`, with GFM extensions enabled by default.
- Streaming: sealed prefix plus mutable tail buffer. Sealed prefixes emit one
  `INSERT` per top-level block plus a `SEAL` patch with offset, length, and
  content hash metadata.
- Images: emitted as Markdown image/link nodes; platform adapters load images.
- Math: optional IR split into `math_inline` and `math_block` nodes. Rendering
  is a platform plugin responsibility.
- HTML: controlled by `html_policy`: passthrough, strip, or downgrade to text.
- Stats: `morph_md_stream_get_stats` exposes sealed/tail/pending byte counts and
  sealed block count for memory monitoring.
- cmark-gfm `footnotes` is enabled with the GFM extension set; dedicated
  footnote IR nodes are still pending because the exposed AST maps references
  through link-shaped nodes.
- Memory: variable strings use `md_buf`; unbounded collections should use
  `md_array`; string-keyed lookups should use `md_strmap`; hash IDs should use
  `md_hash`.

## Current gaps

- Swift and JNI/Kotlin bindings are not implemented yet.
- Patch payloads are JSON for portability; a zero-copy struct callback path is
  still pending for production.
- Stable-prefix detection is conservative and blank-line/block-boundary based;
  finer line-level stability rules are pending.
- Full CommonMark/GFM corpus automation is not wired into CTest yet.
- Dedicated footnote presentation nodes are not modeled yet.

Functions should stay below 200 lines. Keep new generic data structures in
`src/base/` instead of embedding local one-off containers in feature files.
