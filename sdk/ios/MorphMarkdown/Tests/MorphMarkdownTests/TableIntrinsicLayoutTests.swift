#if canImport(UIKit)
import UIKit
import XCTest
@testable import MorphMarkdown

final class TableIntrinsicLayoutTests: XCTestCase {
	func testAttributedCellPreservesBoldWidthAndInsets() {
		let label = InlineAttributedLabel(contentInsets: UIEdgeInsets(top: 8, left: 16, bottom: 8, right: 16))
		label.attributedText = NSAttributedString(
			string: "Anthropic",
			attributes: [.font: UIFont.boldSystemFont(ofSize: 24)]
		)
		assertMinimumSurvivesLayout(label, viewport: 100)
	}

	func testAttributedCellPreservesWholeInlineCodeWidth() {
		let label = InlineAttributedLabel(contentInsets: UIEdgeInsets(top: 8, left: 16, bottom: 8, right: 16))
		label.attributedText = NSAttributedString(
			string: "--thinking low/medium",
			attributes: [
				.font: UIFont.monospacedSystemFont(ofSize: 20, weight: .regular),
				.backgroundColor: UIColor.lightGray
			]
		)
		assertMinimumSurvivesLayout(label, viewport: 180)
	}

	func testBalancingCannotDonateUnbreakableMinimumWidth() {
		let label = MinimumWidthLabel()
		label.text = "x"
		assertMinimumSurvivesLayout(label, viewport: 320)
	}

	private func assertMinimumSurvivesLayout<T: UILabel & TableIntrinsicOverride>(
		_ label: T, viewport: CGFloat
	) {
		let table = MarkdownTableView(theme: MorphMarkdownTheme())
		table.beginRow(header: false)
		table.addCell(label)
		let neighbor = UILabel()
		neighbor.numberOfLines = 0
		neighbor.text = String(repeating: "wrapping text ", count: 30)
		table.addCell(neighbor)
		table.viewportWidthHint = viewport
		let measured = table.sizeThatFits(CGSize(width: viewport, height: .greatestFiniteMagnitude))
		table.frame = CGRect(origin: .zero, size: measured)
		table.layoutIfNeeded()
		XCTAssertGreaterThanOrEqual(label.frame.width, label.tableMinimumWidth)
		XCTAssertGreaterThanOrEqual(
			label.frame.height,
			label.sizeThatFits(CGSize(width: label.frame.width, height: .greatestFiniteMagnitude)).height
		)
	}
}

private final class MinimumWidthLabel: UILabel, TableIntrinsicOverride {
	var tableMinimumWidth: CGFloat { 140 }
}
#endif
