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

		let link = try XCTUnwrap(firstAttributedTextView(in: parent))
		let linkValue = try XCTUnwrap(link.attributedText.attribute(.link, at: 8, effectiveRange: nil) as? URL)
		XCTAssertEqual(linkValue.absoluteString, "morph://speak?text=hello&lang=en-US")
		link.onLinkClick?(linkValue.absoluteString, nil)
		XCTAssertEqual(clickedURL, "morph://speak?text=hello&lang=en-US")
	}

	func testUIViewSupportsScrollableAndIntrinsicModes() {
		let view = MorphMarkdownUIView(frame: CGRect(x: 0, y: 0, width: 320, height: 480))
		XCTAssertTrue(view.isScrollEnabled)

		view.layoutMode = .intrinsicHeight
		view.setMarkdown("A paragraph that should have an intrinsic height.")
		XCTAssertFalse(view.isScrollEnabled)
		XCTAssertNotEqual(view.intrinsicContentSize.height, UIView.noIntrinsicMetric)
	}

	func testFinalAppendCompletesDeferredRender() {
		let rendered = expectation(description: "final append rendered")
		let view = MorphMarkdownUIView(frame: CGRect(x: 0, y: 0, width: 390, height: 844))
		view.mathRenderer = nil
		view.onRendered = { rendered.fulfill() }
		view.append("Final streaming block.", final: true)
		wait(for: [rendered], timeout: 1)
	}

	func testAttributedParagraphWrapsAtItsAssignedWidth() {
		let view = InlineAttributedTextView(contentInsets: .zero)
		view.attributedText = NSAttributedString(
			string: String(repeating: "wrapping text ", count: 20),
			attributes: [.font: UIFont.systemFont(ofSize: 17)]
		)
		view.frame = CGRect(x: 0, y: 0, width: 140, height: 1)
		view.layoutIfNeeded()

		let wrapped = view.sizeThatFits(CGSize(width: 140, height: CGFloat.greatestFiniteMagnitude))
		let singleLine = view.sizeThatFits(CGSize(width: 10_000, height: CGFloat.greatestFiniteMagnitude))
		XCTAssertGreaterThan(wrapped.height, singleLine.height * 2)
		XCTAssertEqual(view.intrinsicContentSize.width, UIView.noIntrinsicMetric)
	}

	func testAttributedParagraphReusesMeasurementUntilContentChanges() {
		let view = InlineAttributedTextView(contentInsets: .zero)
		view.attributedText = NSAttributedString(
			string: String(repeating: "stable text ", count: 20),
			attributes: [.font: UIFont.systemFont(ofSize: 17)]
		)
		let proposal = CGSize(width: 180, height: CGFloat.greatestFiniteMagnitude)

		let first = view.sizeThatFits(proposal)
		let second = view.sizeThatFits(proposal)

		XCTAssertEqual(first, second)
		XCTAssertEqual(view.measurementComputationCount, 1)

		view.attributedText = NSAttributedString(
			string: "replacement",
			attributes: [.font: UIFont.systemFont(ofSize: 17)]
		)
		_ = view.sizeThatFits(proposal)
		XCTAssertEqual(view.measurementComputationCount, 2)
	}

	func testMarkdownTableReusesMeasurementAtTheSameViewportWidth() {
		let table = MarkdownTableView(theme: MorphMarkdownTheme())
		for row in 0..<2 {
			table.beginRow(header: row == 0)
			for column in 0..<2 {
				let label = UILabel()
				label.numberOfLines = 0
				label.text = "row \(row), column \(column), wrapping content"
				table.addCell(label)
			}
		}
		table.viewportWidthHint = 280
		let proposal = CGSize(width: 280, height: CGFloat.greatestFiniteMagnitude)

		let first = table.sizeThatFits(proposal)
		let second = table.sizeThatFits(proposal)

		XCTAssertEqual(first, second)
		XCTAssertEqual(table.measurementComputationCount, 1)
		_ = table.sizeThatFits(CGSize(width: 320, height: proposal.height))
		XCTAssertEqual(table.measurementComputationCount, 1, "The viewport hint intentionally controls table width")
	}

	func testMarkdownViewReusesMeasurementUntilMarkdownChanges() {
		let view = MorphMarkdownUIView(frame: CGRect(x: 0, y: 0, width: 320, height: 480))
		view.layoutMode = .intrinsicHeight
		view.setMarkdown("A paragraph with **stable** layout.")
		let proposal = CGSize(width: 320, height: CGFloat.greatestFiniteMagnitude)

		let first = view.sizeThatFits(proposal)
		let second = view.sizeThatFits(proposal)

		XCTAssertEqual(first, second)
		XCTAssertEqual(view.measurementComputationCount, 1)

		view.setMarkdown("A changed paragraph with **different** layout and another line.")
		_ = view.sizeThatFits(proposal)
		XCTAssertEqual(view.measurementComputationCount, 2)
	}

	private func firstAttributedTextView(in view: UIView) -> InlineAttributedTextView? {
		if let link = view as? InlineAttributedTextView {
			return link
		}
		for child in view.subviews {
			if let link = firstAttributedTextView(in: child) {
				return link
			}
		}
		return nil
	}
}
#endif
