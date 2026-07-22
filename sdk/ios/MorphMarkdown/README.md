# MorphMarkdown for iOS

MorphMarkdown provides UIKit and SwiftUI renderers backed by the shared C
Markdown engine and MathJax-C.

## Local development

From the repository root:

```sh
./scripts/prepare-third-party.sh
sdk/ios/MorphMarkdown/scripts/build-native-ios.sh
cd sdk/ios/MorphMarkdown
swift test
xcodebuild -scheme MorphMarkdown \
  -destination 'platform=iOS Simulator,name=iPhone 17 Pro' test
```

`swift test` covers portable algorithms and the native engine. The
`xcodebuild test` command is also required because it compiles and runs UIKit
tests on iOS.

## UIKit

```swift
let markdownView = MorphMarkdownUIView()
markdownView.apply(configuration: MorphMarkdownConfiguration(
    theme: MorphMarkdownThemes.normal,
    layoutMode: .scrollable,
    onLinkClick: { url, title in /* handle link */ }
))
markdownView.append(chunk, final: false)
```

Use `.scrollable` when the SDK owns vertical scrolling and automatic scrolling
to the latest streamed content. Use `.intrinsicHeight` inside a host-owned
scroll view.

## SwiftUI

```swift
MorphMarkdownView(
    markdown: markdown,
    isStreaming: isStreaming,
    theme: MorphMarkdownThemes.hetiLike,
    layoutMode: .intrinsicHeight
)
```

SwiftUI defaults to intrinsic-height mode. All theme, loader, renderer,
viewport and callback changes are synchronized on update.

## Release binary

The checked-in `Package.swift` uses a locally generated XCFramework for
development. Before publishing a release, generate the archive, checksum and
remote binary manifest:

```sh
scripts/build-native-ios.sh
scripts/prepare-release-package.sh 2.0.0 \
  https://github.com/oxUnd/morph-markdown/releases/download/2.0.0
```

Upload the generated zip and use the generated release manifest in the release
tag.

## Performance budgets

Performance tests are opt-in because their assertions require an iPhone
15-class device and a Release build:

Set `MORPH_RUN_PERFORMANCE_TESTS=1` in the scheme Test action for a physical
device. For a booted simulator, set the test-runner environment first with
`xcrun simctl spawn booted launchctl setenv MORPH_RUN_PERFORMANCE_TESTS 1`.

The 100 KB fixture budgets are 250 ms initial visible render, 20 ms streaming
append work, and 50 MB resident-memory growth. Scrollable mode renders the
visible prefix first and materializes the remaining blocks in small run-loop
batches; intrinsic-height mode renders the full document synchronously.
