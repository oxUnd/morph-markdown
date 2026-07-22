#if canImport(UIKit)
import UIKit
import XCTest
@testable import MorphMarkdown

final class AgentUILinkRenderingTests: XCTestCase {
	func testMorphSpeakLinkInsideBlockquoteRemainsClickable() throws {
		let parent = UIStackView()
		let renderer = MorphMarkdownRenderer()
		var clickedURL: String?
		renderer.onLinkClick = { url, _ in clickedURL = url }

		renderer.render(
			json: #"{"kind":"document","children":[{"kind":"block_quote","children":[{"kind":"paragraph","children":[{"kind":"text","literal":"hello · "},{"kind":"link","url":"morph://speak?text=hello&lang=en-US","children":[{"kind":"text","literal":"Pronounce"}]}]}]}]}"#,
			parent: parent
		)

		let link = try XCTUnwrap(firstLinkLabel(in: parent))
		XCTAssertEqual(link.url, "morph://speak?text=hello&lang=en-US")
		link.onLinkClick?(link.url, link.title)
		XCTAssertEqual(clickedURL, "morph://speak?text=hello&lang=en-US")
	}

	private func firstLinkLabel(in view: UIView) -> LinkLabel? {
		if let link = view as? LinkLabel {
			return link
		}
		for child in view.subviews {
			if let link = firstLinkLabel(in: child) {
				return link
			}
		}
		return nil
	}
}
#endif
