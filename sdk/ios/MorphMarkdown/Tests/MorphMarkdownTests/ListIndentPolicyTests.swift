import XCTest
@testable import MorphMarkdown

final class ListIndentPolicyTests: XCTestCase {
	func testDefaultThemesAlignTopLevelMarkersWithContentEdge() {
		XCTAssertEqual(MorphMarkdownThemes.normal.listIndent, 0)
		XCTAssertEqual(MorphMarkdownThemes.hetiLike.listIndent, 0)
	}

	func testCustomIndentOnlyAppliesToTopLevelRows() {
		XCTAssertEqual(ListIndentPolicy.rowIndent(depth: 0, topLevelIndent: 20), 20)
		XCTAssertEqual(ListIndentPolicy.rowIndent(depth: 1, topLevelIndent: 20), 0)
		XCTAssertEqual(ListIndentPolicy.rowIndent(depth: 3, topLevelIndent: 20), 0)
	}
}
