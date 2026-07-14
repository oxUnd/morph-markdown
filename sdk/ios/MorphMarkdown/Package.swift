// swift-tools-version: 5.9

import PackageDescription

let package = Package(
	name: "MorphMarkdown",
	platforms: [
		.iOS(.v15),
		.macOS(.v12)
	],
	products: [
		.library(name: "MorphMarkdown", targets: ["MorphMarkdown"])
	],
	targets: [
		.target(name: "MorphMarkdown")
	]
)
