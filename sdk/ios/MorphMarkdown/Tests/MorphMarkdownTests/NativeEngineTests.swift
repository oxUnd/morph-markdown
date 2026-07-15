import XCTest
@testable import MorphMarkdown

final class NativeEngineTests: XCTestCase {
	func testSnapshotParsesGfmAndMath() throws {
		guard let engine = MorphMarkdownEngine() else {
			throw XCTSkip("native engine unavailable")
		}
		defer { engine.close() }

		let markdown = "# Title\n\n- [x] done\n\n| a | b |\n|---|---|\n| $x$ | ![alt](file:///tmp/a.png) |\n"
		XCTAssertEqual(engine.append(markdown, final: true), 0)
		let json = try XCTUnwrap(engine.snapshotJson())

		XCTAssertTrue(json.contains("\"heading\""))
		XCTAssertTrue(json.contains("\"tasklist\""))
		XCTAssertTrue(json.contains("\"table\""))
		XCTAssertTrue(json.contains("\"math_inline\""))
		XCTAssertTrue(json.contains("\"image\""))
	}
}
