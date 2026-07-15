#if canImport(UIKit)
import UIKit

final class MarkdownTableView: UIView {
	private struct Cell {
		let row: Int
		let column: Int
		let header: Bool
		let view: UIView
	}

	private let theme: MorphMarkdownTheme
	private var cells: [Cell] = []
	private var rowHeaders: [Bool] = []
	private var columnWidths: [CGFloat] = []
	private var rowHeights: [CGFloat] = []
	private var currentRow = -1
	private var currentColumn = 0
	var viewportWidthHint: CGFloat = 0

	init(theme: MorphMarkdownTheme) {
		self.theme = theme
		super.init(frame: .zero)
		backgroundColor = .clear
	}

	required init?(coder: NSCoder) {
		return nil
	}

	func beginRow(header: Bool) {
		currentRow += 1
		currentColumn = 0
		rowHeaders.append(header)
	}

	func addCell(_ view: UIView) {
		cells.append(Cell(row: currentRow, column: currentColumn, header: rowHeaders[currentRow], view: view))
		addSubview(view)
		currentColumn += 1
		setNeedsLayout()
	}

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		resetMeasures()
		let widths = resolveColumnWidths(availableWidth: availableWidth(for: size))
		measureCells(widths: widths)
		return CGSize(width: columnWidths.reduce(0, +), height: rowHeights.reduce(0, +))
	}

	override var intrinsicContentSize: CGSize {
		let width = bounds.width > 0 ? bounds.width : UIView.noIntrinsicMetric
		return sizeThatFits(CGSize(width: width, height: UIView.noIntrinsicMetric))
	}

	override func systemLayoutSizeFitting(
		_ targetSize: CGSize,
		withHorizontalFittingPriority horizontalFittingPriority: UILayoutPriority,
		verticalFittingPriority: UILayoutPriority
	) -> CGSize {
		let width = fittingWidth(targetSize.width, priority: horizontalFittingPriority)
		return sizeThatFits(CGSize(width: width, height: targetSize.height))
	}

	override func layoutSubviews() {
		super.layoutSubviews()
		_ = sizeThatFits(bounds.size)
		let rowTops = offsets(rowHeights)
		let columnLefts = offsets(columnWidths)
		for cell in cells {
			cell.view.frame = CGRect(
				x: columnLefts[cell.column],
				y: rowTops[cell.row],
				width: columnWidths[cell.column],
				height: rowHeights[cell.row]
			)
		}
	}

	override func draw(_ rect: CGRect) {
		drawBackgrounds()
		drawGrid()
	}

	private func resetMeasures() {
		let columns = cells.map { $0.column + 1 }.max() ?? 0
		let rows = cells.map { $0.row + 1 }.max() ?? 0
		columnWidths = Array(repeating: 0, count: columns)
		rowHeights = Array(repeating: 0, count: rows)
	}

	private func resolveColumnWidths(availableWidth: CGFloat?) -> [CGFloat] {
		let inputs = cells.map {
			TableCellWidth(column: $0.column,
				       minWidth: $0.view.tableMinIntrinsicWidth(),
				       preferredWidth: $0.view.tablePreferredIntrinsicWidth())
		}
		return TableColumnSizer.sizeColumns(
			cells: inputs,
			columnCount: columnWidths.count,
			availableWidth: availableWidth,
			maxColumnWidth: theme.tableCellMaxWidth,
			wrap: theme.tableCellWrap
		)
	}

	private func measureCells(widths: [CGFloat]) {
		for cell in cells {
			let width = widths[cell.column]
			let size = cell.view.sizeThatFits(CGSize(width: theme.tableCellWrap ? width : CGFloat.greatestFiniteMagnitude,
								 height: CGFloat.greatestFiniteMagnitude))
			columnWidths[cell.column] = max(columnWidths[cell.column], theme.tableCellWrap ? width : ceil(size.width))
			rowHeights[cell.row] = max(rowHeights[cell.row], ceil(size.height))
		}
	}

	private func availableWidth(for size: CGSize) -> CGFloat? {
		if !theme.tableCellWrap {
			return nil
		}
		if viewportWidthHint > 0 {
			return viewportWidthHint
		}
		return size.width > 0 && size.width < CGFloat.greatestFiniteMagnitude ? size.width : nil
	}

	private func fittingWidth(_ width: CGFloat, priority: UILayoutPriority) -> CGFloat {
		if priority == .required, width > 0, width < CGFloat.greatestFiniteMagnitude {
			return width
		}
		if bounds.width > 0 {
			return bounds.width
		}
		return UIView.noIntrinsicMetric
	}

	private func offsets(_ values: [CGFloat]) -> [CGFloat] {
		var out = Array(repeating: CGFloat(0), count: values.count)
		for index in values.indices.dropFirst() {
			out[index] = out[index - 1] + values[index - 1]
		}
		return out
	}

	private func drawBackgrounds() {
		for row in rowHeights.indices {
			let y = rowHeights.prefix(row).reduce(0, +)
			let color = rowHeaders[row] ? theme.tableStyle.headerBackgroundColor : theme.tableStyle.bodyBackgroundColor
			UIColor(argb: color).setFill()
			UIRectFill(CGRect(x: 0, y: y, width: bounds.width, height: rowHeights[row]))
		}
	}

	private func drawGrid() {
		let path = UIBezierPath()
		addVerticalLines(to: path)
		addHorizontalLines(to: path)
		UIColor(argb: theme.tableStyle.borderColor).setStroke()
		path.lineWidth = theme.tableStyle.borderWidth
		path.stroke()
	}

	private func addVerticalLines(to path: UIBezierPath) {
		var x: CGFloat = 0
		path.move(to: CGPoint(x: 0, y: 0))
		path.addLine(to: CGPoint(x: 0, y: bounds.height))
		for width in columnWidths {
			x += width
			path.move(to: CGPoint(x: x, y: 0))
			path.addLine(to: CGPoint(x: x, y: bounds.height))
		}
	}

	private func addHorizontalLines(to path: UIBezierPath) {
		var y: CGFloat = 0
		path.move(to: CGPoint(x: 0, y: 0))
		path.addLine(to: CGPoint(x: bounds.width, y: 0))
		for height in rowHeights {
			y += height
			path.move(to: CGPoint(x: 0, y: y))
			path.addLine(to: CGPoint(x: bounds.width, y: y))
		}
	}
}

private extension UIView {
	func tablePreferredIntrinsicWidth() -> CGFloat {
		return ceil(sizeThatFits(CGSize(width: CGFloat.greatestFiniteMagnitude,
						height: CGFloat.greatestFiniteMagnitude)).width)
	}

	func tableMinIntrinsicWidth() -> CGFloat {
		if let inline = self as? InlineLayoutView {
			return inline.subviews.map { $0.tableMinIntrinsicWidth() }.max() ?? 0
		}
		if let label = self as? UILabel {
			return label.longestUnbreakableWidth()
		}
		return tablePreferredIntrinsicWidth()
	}
}

private extension UILabel {
	func longestUnbreakableWidth() -> CGFloat {
		guard let text = text ?? attributedText?.string else {
			return 0
		}
		let font = self.font ?? UIFont.systemFont(ofSize: 16)
		if lineBreakMode == .byCharWrapping {
			return longestCharacterWidth(text: text, font: font)
		}
		return InlineTextFragmenter.fragments(text).map { fragment in
			if fragment.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
				return CGFloat(0)
			}
			return ceil((fragment as NSString).size(withAttributes: [.font: font]).width)
		}.max() ?? 0
	}

	private func longestCharacterWidth(text: String, font: UIFont) -> CGFloat {
		return text.map { character in
			ceil((String(character) as NSString).size(withAttributes: [.font: font]).width)
		}.max() ?? 0
	}
}
#endif
