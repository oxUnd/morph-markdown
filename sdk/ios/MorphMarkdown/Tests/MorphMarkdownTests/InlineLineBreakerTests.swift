import XCTest
@testable import MorphMarkdown

final class InlineLineBreakerTests: XCTestCase {
	func testWrapsItemsWhenLineWidthOverflows() {
		let lines = InlineLineBreaker.breakLines(
			items: [
				InlineItemSize(width: 40, height: 12),
				InlineItemSize(width: 45, height: 12),
				InlineItemSize(width: 30, height: 12)
			],
			maxWidth: 90,
			minLineHeight: 16
		)

		XCTAssertEqual(lines.count, 2)
		XCTAssertEqual(lines[0].start, 0)
		XCTAssertEqual(lines[0].end, 2)
		XCTAssertEqual(lines[0].height, 16)
		XCTAssertEqual(lines[1].start, 2)
		XCTAssertEqual(lines[1].end, 3)
	}

	func testTallInlineMathOnlyExpandsItsOwnLine() {
		let lines = InlineLineBreaker.breakLines(
			items: [
				InlineItemSize(width: 40, height: 18),
				InlineItemSize(width: 35, height: 42),
				InlineItemSize(width: 70, height: 18)
			],
			maxWidth: 90,
			minLineHeight: 20
		)

		XCTAssertEqual(lines.count, 2)
		XCTAssertEqual(lines[0].height, 42)
		XCTAssertEqual(lines[1].height, 20)
	}

	func testEmptyInputProducesNoLines() {
		let lines = InlineLineBreaker.breakLines(items: [], maxWidth: 90, minLineHeight: 20)
		XCTAssertTrue(lines.isEmpty)
	}

	func testForcedBreakStartsANewLine() {
		let lines = InlineLineBreaker.breakLines(
			items: [
				InlineItemSize(width: 40, height: 12),
				InlineItemSize(width: 0, height: 0, isLineBreak: true),
				InlineItemSize(width: 35, height: 12)
			],
			maxWidth: 100,
			minLineHeight: 16
		)

		XCTAssertEqual(lines.count, 2)
		XCTAssertEqual(lines[0], InlineLine(start: 0, end: 1, width: 40, height: 16))
		XCTAssertEqual(lines[1], InlineLine(start: 2, end: 3, width: 35, height: 16))
	}
}
