import XCTest
@testable import MorphMarkdown

final class InlineTextFragmenterTests: XCTestCase {
	func testSplitsChineseIntoSingleCharacterFragments() {
		XCTAssertEqual(InlineTextFragmenter.fragments("求根公式"), ["求", "根", "公", "式"])
	}

	func testKeepsEnglishWordsTogetherAndSpacesSeparate() {
		XCTAssertEqual(InlineTextFragmenter.fragments("Morph Markdown SDK"), ["Morph", " ", "Markdown", " ", "SDK"])
	}

	func testKeepsPunctuationAsBreakableFragments() {
		XCTAssertEqual(InlineTextFragmenter.fragments("Android/iOS。"), ["Android", "/", "iOS", "。"])
	}
}
