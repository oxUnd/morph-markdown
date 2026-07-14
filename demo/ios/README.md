# MorphMarkdown iOS Demo

This demo shows how to embed the SwiftUI `MorphMarkdown` SDK from the local
Swift package.

The SDK does not yet include the native C bridge on iOS, so the demo uses a
small fixture implementation of `MorphMarkdownNativeEngine`. Replace
`DemoMarkdownEngine` with the native bridge adapter when it lands.

## Run

Open `MorphMarkdownDemo.xcodeproj` in Xcode and run the `MorphMarkdownDemo`
scheme on an iOS simulator.

Command-line build:

```sh
xcodebuild \
  -project demo/ios/MorphMarkdownDemo.xcodeproj \
  -scheme MorphMarkdownDemo \
  -destination 'platform=iOS Simulator,name=iPhone 16' \
  build
```

