# Android demo for markdown-render

This is a minimal Android project that demonstrates how to embed
`markdown-render` from JNI.

Pipeline:

1. Kotlin streams Markdown chunks into a growing buffer.
2. JNI calls `morph_md_stream_append()` and returns the current IR JSON.
3. Kotlin renders the IR with native Android views.
4. Math nodes call `mathjax-c` through JNI and render ARGB bitmaps.

Build:

```sh
cd demo/android
gradle :app:assembleDebug
```

If your shell does not have `gradle`, reuse any Gradle wrapper:

```sh
cd demo/android
../../../android/gradlew :app:assembleDebug
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
