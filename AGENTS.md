# Agent Notes

This repository is `morph-markdown`, a streaming Markdown renderer core plus
Android/iOS SDK demos. Read this file before making changes in a new session.

## Project Shape

- C core lives in `include/` and `src/`.
- Reusable C foundations live in `src/base/`: arrays, buffers, hash, string maps,
  UTF-8 handling, width helpers, and related low-level utilities.
- Android SDK lives in `sdk/android/morph-markdown/`.
- Android demo lives in `demo/android/` and must only demonstrate SDK
  integration; reusable rendering code belongs in the SDK module.
- iOS SwiftUI SDK lives in `sdk/ios/MorphMarkdown/`.
- MathJax-C is fetched into this checkout at `.third_party/mathjax-c` by
  `scripts/prepare-third-party.sh`; do not add dependencies on directories
  outside this repository.
- Android FreeType/HarfBuzz for MathJax-C are prepared inside
  `sdk/android/morph-markdown/.build/vendor-android/<abi>` by
  `sdk/android/morph-markdown/scripts/prepare-android-deps.sh`.

## Coding Rules

- Keep every function under 200 lines. Split with focused helpers rather than
  long procedural functions.
- Do not build features by ad-hoc string manipulation when a structured parser,
  IR node, or existing helper is available.
- Add generic data structures and reusable algorithms to clear foundation files
  instead of burying them inside feature code.
- Keep directory ownership clean:
  - C core and portable algorithms in `src/` or `src/base/`.
  - Android native UI/rendering in `sdk/android/morph-markdown`.
  - Android sample-only fixtures and controls in `demo/android`.
  - iOS SwiftUI rendering in `sdk/ios/MorphMarkdown`.
- Prefer small, pure algorithm classes for layout decisions so they can be unit
  tested without Android Views.
- Use ASCII in source unless the file already uses non-ASCII or the demo content
  intentionally needs Chinese text.
- Do not revert unrelated user changes. Check `git status --short` before edits.

## Markdown/Rendering Requirements

- Standard Markdown and GFM behavior must stay supported through `cmark-gfm`.
- Math and images are pluggable at SDK level.
- MathJax-C bitmap output must not be scaled or compressed by the SDK. Reserve
  the bitmap's actual width and height, then lay out surrounding text.
- Inline math participates in inline flow:
  - Formula bitmap is one unbreakable inline item.
  - The line height is the max height of all inline items in that line.
  - Text, images, and formulas are vertically centered inside the line box.
  - Only wrap when the line width is insufficient.
- Display math remains block-like with outer spacing.
- Table wrap semantics:
  - `tableCellWrap=true` means prefer fitting the table into the viewport.
  - Use intrinsic sizing: min-content and preferred widths per column, then
    shrink/grow within the available width.
  - Unbreakable content such as a large formula, image, long word, or inline code
    may still force horizontal scrolling.
  - Do not shrink formula bitmaps to satisfy table width.
- Table borders, header/body background, header/body text color and text size
  are theme-configurable.
- Lists and task lists should align markers/checkboxes with the first text line,
  not the visual center of a multi-line item.
- HetiLike Chinese typography is supported in the demo; preserve Chinese fixtures
  when validating text flow, CJK spacing, lists, and tables.

## Important Android Classes

- `MorphMarkdownView`: SDK entry point.
- `MorphMarkdownRenderer`: IR-to-Android View renderer.
- `InlineLayout`: inline line box layout and vertical centering.
- `InlineLineBreaker`: pure line breaking algorithm.
- `InlineTextFragmenter`: splits text into breakable fragments for inline flow.
- `MarkdownTableView`: table ViewGroup with two-pass measurement.
- `TableColumnSizer`: pure intrinsic table column width algorithm.
- `TableIntrinsicMeasurer`: converts child views to min/preferred widths.
- `MarkdownTableScrollView`: passes viewport width to tables inside horizontal
  scroll containers.
- `MathJaxMathRenderer`: calls native MathJax-C and returns fixed-size bitmap
  views.

## Validation Commands

From `demo/android/`:

```sh
../../scripts/prepare-third-party.sh
../../sdk/android/morph-markdown/scripts/prepare-android-deps.sh
gradle :morph-markdown:testDebugUnitTest :morph-markdown:assembleDebug :app:assembleDebug
```

When MathJax-C or native code changes, force a native rebuild:

```sh
gradle --stop
gradle clean :morph-markdown:testDebugUnitTest :morph-markdown:assembleDebug :app:assembleDebug
```

Install and launch Android demo:

```sh
gradle :app:installDebug --console=plain --quiet
adb -s emulator-5554 shell am force-stop com.morph.markdown.demo
adb -s emulator-5554 shell am start -n com.morph.markdown.demo/.MainActivity
```

From repository root, build C core and Kitty demo:

```sh
./scripts/prepare-third-party.sh
cmake -S . -B build -DMORPH_MARKDOWN_BUILD_KITTY=ON -DMORPH_MARKDOWN_BUILD_TESTS=ON
cmake --build build
```

From `sdk/ios/MorphMarkdown/`:

```sh
../../../scripts/prepare-third-party.sh
swift build
```

Before finishing changes:

```sh
git diff --check
python3 - <<'PY'
from pathlib import Path
paths = list(Path('sdk/android/morph-markdown/src/main/java').rglob('*.kt')) + list(Path('demo/android/app/src/main/java').rglob('*.kt'))
violations=[]
for p in paths:
    lines=p.read_text().splitlines(); i=0
    while i < len(lines):
        s=lines[i].lstrip()
        if any(s.startswith(x) for x in ('fun ','private fun ','internal fun ','public fun ','override fun ')):
            start=i; brace=0; seen=False; j=i
            while j < len(lines):
                brace += lines[j].count('{') - lines[j].count('}')
                seen = seen or '{' in lines[j]
                if seen and brace <= 0: break
                j += 1
            if j-start+1 > 200: violations.append((p,start+1,j-start+1,s))
            i=j
        i += 1
for v in violations: print(f'{v[0]}:{v[1]} {v[2]} lines {v[3]}')
if not violations: print('no Kotlin function over 200 lines')
PY
```

## Current Known Gaps

- iOS native C bridge adapter is still pending; SwiftUI SDK currently consumes a
  `MorphMarkdownNativeEngine` protocol.
- iOS table layout should eventually mirror Android intrinsic table sizing.
- Stable-prefix streaming detection is conservative and blank-line/block-boundary
  based.
- Dedicated footnote presentation nodes are not modeled yet.
