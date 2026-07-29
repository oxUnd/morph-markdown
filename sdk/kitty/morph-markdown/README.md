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
options.initial_cursor_column = 0;

struct morph_md_kitty *renderer = morph_md_kitty_create(&options);
morph_md_kitty_append(renderer, markdown, strlen(markdown), 1);
morph_md_kitty_render(renderer);
morph_md_kitty_destroy(renderer);
```

By default output goes to `stdout`. Set `options.write` and `options.user_data`
to route the UTF-8, ANSI and Kitty protocol byte stream elsewhere. The callback
must return zero on success.

Local PNG images are transmitted directly with the Kitty graphics protocol.
Block images are scaled down only when necessary to fit the content viewport;
table images keep their intrinsic cell size and participate in column and row
measurement. Invalid files, non-PNG formats, remote URLs and videos render a
text placeholder instead.

Set `options.media` and `options.media_user_data` to receive media references
the SDK could not render directly. The SDK removes a leading `file://` prefix
before invoking the callback.

Math bitmaps are transmitted at their native pixel dimensions. The SDK uses
Kitty's no-cursor-movement placement policy and reserves the corresponding
terminal columns without requesting image scaling. Math requires a valid
`font_path`; a renderer created with `NULL` options enables GFM text rendering
without math.

Content padding is measured in terminal rows and columns. Right padding reduces
the width available to normal text and table layout. Tables use intrinsic
column sizing: compact tables keep their preferred width, while wider tables
shrink columns toward their minimum content width and wrap cell text. Wrapping
prefers whitespace and Unicode/CJK break opportunities, then splits an
otherwise unbreakable ordinary word at a grapheme boundary. Tabs in table cells
expand to four spaces for deterministic alignment.

Inline code and native-size formulas remain atomic table items. They are never
split, scaled or compressed, so either may force a table wider than the
available terminal width. A table with more columns than its borders and
minimum cell widths can accommodate may also exceed that boundary.

When the caller writes a visible prefix before rendering, set
`initial_cursor_column` to the cursor column after that prefix. The first
rendered row continues from that column without adding left padding again;
subsequent rows use `content_padding_left_columns`.

Call `morph_md_kitty_render` after each append. Complete rendered rows are
appended at the current cursor position while only the structurally mutable
tail is refreshed. Normal streaming does not clear the viewport or discard
terminal scrollback. `morph_md_kitty_clear` remains available for an explicit
destructive reset; the next render paints the accumulated snapshot again.

Incremental renders use synchronized output internally. Explicit
`morph_md_kitty_begin_frame` and `morph_md_kitty_end_frame` calls may still wrap
multiple SDK operations into one frame. Additional UI text can be composed
with `morph_md_kitty_write_text`.
