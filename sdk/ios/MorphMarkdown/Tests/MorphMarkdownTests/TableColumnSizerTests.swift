import XCTest
@testable import MorphMarkdown

final class TableColumnSizerTests: XCTestCase {
	func testWrapsColumnsIntoAvailableWidthWhenPreferredIsTooWide() {
		let widths = TableColumnSizer.sizeColumns(
			cells: [
				TableCellWidth(column: 0, minWidth: 80, preferredWidth: 220),
				TableCellWidth(column: 1, minWidth: 90, preferredWidth: 260)
			],
			columnCount: 2,
			availableWidth: 360,
			maxColumnWidth: 280,
			wrap: true
		)

		XCTAssertEqual(widths.reduce(0, +), 360)
		XCTAssertGreaterThanOrEqual(widths[0], 80)
		XCTAssertGreaterThanOrEqual(widths[1], 90)
	}

	func testPreservesMinimumWidthsWhenTheyExceedAvailableWidth() {
		let widths = TableColumnSizer.sizeColumns(
			cells: [
				TableCellWidth(column: 0, minWidth: 260, preferredWidth: 320),
				TableCellWidth(column: 1, minWidth: 220, preferredWidth: 260)
			],
			columnCount: 2,
			availableWidth: 360,
			maxColumnWidth: 280,
			wrap: true
		)

		XCTAssertEqual(widths, [260, 220])
	}

	func testGrowsPreferredColumnsToUseAvailableWidth() {
		let widths = TableColumnSizer.sizeColumns(
			cells: [
				TableCellWidth(column: 0, minWidth: 40, preferredWidth: 80),
				TableCellWidth(column: 1, minWidth: 60, preferredWidth: 120)
			],
			columnCount: 2,
			availableWidth: 300,
			maxColumnWidth: 280,
			wrap: true
		)

		XCTAssertEqual(widths.reduce(0, +), 300)
		XCTAssertGreaterThan(widths[1], widths[0])
	}

	func testLeavesNaturalWidthsWhenWrappingIsDisabled() {
		let widths = TableColumnSizer.sizeColumns(
			cells: [
				TableCellWidth(column: 0, minWidth: 40, preferredWidth: 420),
				TableCellWidth(column: 1, minWidth: 60, preferredWidth: 380)
			],
			columnCount: 2,
			availableWidth: 300,
			maxColumnWidth: 280,
			wrap: false
		)

		XCTAssertEqual(widths, [420, 380])
	}
}
