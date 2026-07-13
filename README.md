# morph markdown-render

Small native Markdown streaming core for iOS and Android.

## Layout

- `include/` public C ABI used by Swift/JNI bindings.
- `src/base/` internal foundations: errors, buffers, arrays, hashing.
- `src/md_stream.c` streaming coordinator and cmark-gfm IR conversion.
- `tests/` C tests for streaming, GFM, math toggles, and snapshots.

## Design

The core does not render platform UI. It accepts streamed Markdown bytes and
emits JSON IR patches. iOS and Android render those patches with native views.

- Parser: `cmark-gfm`, with GFM extensions enabled by default.
- Streaming: sealed prefix plus mutable tail buffer.
- Images: emitted as Markdown image/link nodes; platform adapters load images.
- Math: optional IR split into `math_inline` nodes. Rendering is a platform
  plugin responsibility.
- Memory: variable strings use `md_buf`; unbounded collections should use
  `md_array`; hash IDs should use `md_hash`.

Functions should stay below 200 lines. Keep new generic data structures in
`src/base/` instead of embedding local one-off containers in feature files.
