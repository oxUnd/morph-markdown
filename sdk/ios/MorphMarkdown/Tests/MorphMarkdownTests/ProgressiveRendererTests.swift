#if canImport(UIKit)
import UIKit
import XCTest
@testable import MorphMarkdown

final class ProgressiveRendererTests: XCTestCase {
	func testProgressiveRendererMaterializesEveryBlockInBatches() {
		let renderer = MorphMarkdownRenderer()
		let parent = UIStackView()
		let json = documentJSON(blockCount: 11)

		XCTAssertTrue(renderer.renderProgressively(json: json, parent: parent, initialBlockCount: 3))
		XCTAssertEqual(parent.arrangedSubviews.count, 3)
		while renderer.renderNextProgressiveBatch(parent: parent, count: 2) {}
		XCTAssertEqual(parent.arrangedSubviews.count, 11)
	}

	func testStablePrefixCanKeepProgressiveTail() {
		let renderer = MorphMarkdownRenderer()
		let parent = UIStackView()
		renderer.render(json: documentJSON(blockCount: 3), parent: parent)
		let reused = parent.arrangedSubviews[0]

		XCTAssertTrue(renderer.renderReusingStablePrefixProgressively(
			json: documentJSON(blockCount: 9),
			parent: parent,
			stableBlockCount: 3,
			initialTailCount: 2
		))
		XCTAssertTrue(parent.arrangedSubviews[0] === reused)
		XCTAssertEqual(parent.arrangedSubviews.count, 5)
	}

	private func documentJSON(blockCount: Int) -> String {
		let blocks = (0..<blockCount).map { index in
			#"{"kind":"paragraph","children":[{"kind":"text","literal":"block \#(index)"}]}"#
		}.joined(separator: ",")
		return #"{"kind":"document","children":[\#(blocks)]}"#
	}
}
#endif
