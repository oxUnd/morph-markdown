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
cd demo/android
gradle :morph-markdown:assembleDebug :app:assembleDebug
```

If your shell does not have `gradle`, reuse any Gradle wrapper:

```sh
cd demo/android
../../../android/gradlew :morph-markdown:assembleDebug :app:assembleDebug
```

The demo expects Android FreeType and HarfBuzz libraries. By default CMake
looks for them in:

```text
../android/third_party/media/<abi>
```

Override with:

```sh
gradle :app:assembleDebug \
  -Pandroid.injected.cmake.arguments=-DMORPH_MATHJAX_ANDROID_PREFIX=/path/to/prefix
```
