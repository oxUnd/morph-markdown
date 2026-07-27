# MorphMarkdown Kitty SDK

The Kitty SDK renders streamed CommonMark/GFM as terminal text and sends math
bitmaps with the Kitty graphics protocol. It works in Kitty and other terminal
emulators that implement the protocol.

The SDK is built by the repository's root CMake project:

```sh
./scripts/prepare-third-party.sh
cmake -S . -B build \
  -DMORPH_MARKDOWN_BUILD_KITTY=ON \
  -DMORPH_MARKDOWN_BUILD_KITTY_DEMO=OFF
cmake --build build --target morph-markdown-kitty
```

Link to `MorphMarkdown::Kitty` when this repository is included with
`add_subdirectory`, then include `morph_markdown_kitty.h`.

```c
struct morph_md_kitty_options options = {0};
options.font_path = "/path/to/STIXTwoMath-Regular.ttf";
options.features = MORPH_MD_FEATURE_GFM | MORPH_MD_FEATURE_MATH;
options.terminal_fd = STDOUT_FILENO;
options.content_padding_top_rows = 1;
options.content_padding_right_columns = 4;
options.content_padding_bottom_rows = 1;
options.content_padding_left_columns = 4;

struct morph_md_kitty *renderer = morph_md_kitty_create(&options);
morph_md_kitty_append(renderer, markdown, strlen(markdown), 1);
morph_md_kitty_begin_frame(renderer);
morph_md_kitty_clear(renderer);
morph_md_kitty_render(renderer);
morph_md_kitty_end_frame(renderer);
morph_md_kitty_destroy(renderer);
```

By default output goes to `stdout`. Set `options.write` and `options.user_data`
to route the UTF-8, ANSI and Kitty protocol byte stream elsewhere. The callback
must return zero on success.

Set `options.media` and `options.media_user_data` to receive image and video
references after the text frame has rendered. The SDK removes a leading
`file://` prefix before invoking the callback.

Math bitmaps are transmitted at their native pixel dimensions. The SDK uses
Kitty's no-cursor-movement placement policy and reserves the corresponding
terminal columns without requesting image scaling. Math requires a valid
`font_path`; a renderer created with `NULL` options enables GFM text rendering
without math.

Content padding is measured in terminal rows and columns. Right padding reduces
the width available to normal text wrapping; inline code, native-size formulas
and tables remain unbreakable and may exceed that boundary.

Wrap complete redraws with `morph_md_kitty_begin_frame` and
`morph_md_kitty_end_frame`. Supporting terminals hold the previous frame while
the new text and graphics are produced, which prevents partial redraw flicker.
The SDK buffers bytes between these calls and submits the completed frame at
`end_frame`, so MathJax computation time does not consume the terminal's
synchronized-output timeout. Additional UI text can be composed into the same
frame with `morph_md_kitty_write_text`.
