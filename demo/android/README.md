# Android demo for markdown-render

This is a minimal Android project that demonstrates how to embed
the `markdown-render` Android SDK.

Pipeline:

1. The app creates `MorphMarkdownView` from `:morph-markdown`.
2. The SDK owns JNI, `morph_md_engine`, IR snapshot, and native View rendering.
3. The app streams Markdown chunks with `MorphMarkdownView.append()`.
4. The app configures theme controls, `MathJaxMathRenderer`, and `FileImageLoader`.

Build:

```sh
cd ../..
./scripts/prepare-third-party.sh
./sdk/android/morph-markdown/scripts/prepare-android-deps.sh
cd demo/android
./gradlew :morph-markdown:assembleDebug :app:assembleDebug
```

The checked-in wrapper pins Gradle 9.6.1, so a system Gradle installation is
not required.

The demo expects Android FreeType and HarfBuzz libraries. By default CMake
looks for them in:

```text
sdk/android/morph-markdown/.build/vendor-android/<abi>
```

Override with:

```sh
./gradlew :app:assembleDebug \
  -Pandroid.injected.cmake.arguments=-DMORPH_MATHJAX_ANDROID_PREFIX=/path/to/prefix
```
