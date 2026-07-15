// swift-tools-version: 5.9
import PackageDescription

let package = Package(
	name: "MorphMarkdown",
	platforms: [
		.iOS(.v15),
		.macOS(.v14)
	],
	products: [
		.library(name: "MorphMarkdown", targets: ["MorphMarkdown"])
	],
	targets: [
		.binaryTarget(
			name: "MorphMarkdownNativeCore",
			path: ".build/native/MorphMarkdownNativeCore.xcframework"
		),
		.target(
			name: "MorphMarkdownNative",
			dependencies: ["MorphMarkdownNativeCore"],
			path: "Sources/MorphMarkdownNative",
			publicHeadersPath: "include",
			cSettings: [
				.headerSearchPath("include"),
				.headerSearchPath("../../.build/native/include")
			],
			linkerSettings: [
				.linkedLibrary("c++")
			]
		),
		.target(
			name: "MorphMarkdown",
			dependencies: ["MorphMarkdownNative"],
			path: "Sources/MorphMarkdown",
			resources: [
				.copy("../../Resources/STIXTwoMath-Regular.ttf")
			]
		),
		.testTarget(
			name: "MorphMarkdownTests",
			dependencies: ["MorphMarkdown"],
			path: "Tests/MorphMarkdownTests"
		)
	]
)
