# Kitty terminal demo

Build from the repository root:

```sh
./scripts/prepare-third-party.sh
cmake -S . -B build \
  -DMORPH_MARKDOWN_BUILD_KITTY=ON \
  -DMORPH_MARKDOWN_BUILD_KITTY_DEMO=ON
cmake --build build --target morph-md-kitty-demo
```

Run it in Kitty, Ghostty, WezTerm, or another terminal that supports the Kitty
graphics protocol:

```sh
./build/demo/kitty/morph-md-kitty-demo
```

The demo appends the fixture in chunks. Stable Markdown rows remain in terminal
scrollback while only the current mutable tail is refreshed. It uses one row of
vertical padding and four columns of horizontal padding by default. Pass
`--no-delay` to disable animation or `--no-padding` to compare the unpadded
layout; the flags can be combined in any order.
