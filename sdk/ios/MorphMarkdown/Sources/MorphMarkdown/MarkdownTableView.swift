#if canImport(UIKit)
import UIKit

protocol TableIntrinsicOverride {
	var tableMinimumWidth: CGFloat { get }
}

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
		let availableWidth = availableWidth(for: size)
		let inputs = tableCellWidths()
		let resolved = resolveColumnWidths(inputs: inputs, availableWidth: availableWidth)
		let widths = balancedColumnWidths(resolved, inputs: inputs, availableWidth: availableWidth)
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

	private func tableCellWidths() -> [TableCellWidth] {
		return cells.map {
			TableCellWidth(column: $0.column,
				       minWidth: $0.view.tableMinIntrinsicWidth(),
				       preferredWidth: $0.view.tablePreferredIntrinsicWidth())
		}
	}

	private func resolveColumnWidths(inputs: [TableCellWidth], availableWidth: CGFloat?) -> [CGFloat] {
		return TableColumnSizer.sizeColumns(
			cells: inputs,
			columnCount: columnWidths.count,
			availableWidth: availableWidth,
			maxColumnWidth: theme.tableCellMaxWidth,
			wrap: theme.tableCellWrap
		)
	}

	private func balancedColumnWidths(_ widths: [CGFloat], inputs: [TableCellWidth], availableWidth: CGFloat?) -> [CGFloat] {
		guard theme.tableCellWrap, let availableWidth, availableWidth > 0, widths.count > 1 else {
			return widths
		}
		var balanced = widths
		let slack = columnSlack(widths: balanced)
		let deficits = columnDeficits(widths: balanced, inputs: inputs)
		transferColumnWidth(widths: &balanced, slack: slack, deficits: deficits)
		return normalizedWidths(balanced, target: availableWidth)
	}

	private func columnSlack(widths: [CGFloat]) -> [CGFloat] {
		let actual = actualColumnWidths(widths: widths)
		return widths.indices.map { max(0, widths[$0] - actual[$0]) }
	}

	private func actualColumnWidths(widths: [CGFloat]) -> [CGFloat] {
		var actual = Array(repeating: CGFloat(0), count: widths.count)
		for cell in cells {
			let width = widths[cell.column]
			let size = cell.view.sizeThatFits(CGSize(width: width, height: CGFloat.greatestFiniteMagnitude))
			actual[cell.column] = max(actual[cell.column], min(width, ceil(size.width)))
		}
		return actual
	}

	private func columnDeficits(widths: [CGFloat], inputs: [TableCellWidth]) -> [CGFloat] {
		let preferred = preferredColumnWidths(inputs: inputs, count: widths.count)
		return widths.indices.map { max(0, preferred[$0] - widths[$0]) }
	}

	private func preferredColumnWidths(inputs: [TableCellWidth], count: Int) -> [CGFloat] {
		var preferred = Array(repeating: CGFloat(0), count: count)
		for input in inputs {
			let capped = min(input.preferredWidth, theme.tableCellMaxWidth)
			preferred[input.column] = max(preferred[input.column], capped)
		}
		return preferred
	}

	private func transferColumnWidth(widths: inout [CGFloat], slack: [CGFloat], deficits: [CGFloat]) {
		var remainingSlack = slack
		var remainingDeficits = deficits
		for receiver in receiverOrder(deficits: deficits) {
			for donor in donorOrder(slack: slack) where receiver != donor {
				let amount = min(remainingDeficits[receiver], remainingSlack[donor])
				guard amount > 0 else {
					continue
				}
				widths[receiver] += amount
				widths[donor] -= amount
				remainingDeficits[receiver] -= amount
				remainingSlack[donor] -= amount
			}
		}
	}

	private func receiverOrder(deficits: [CGFloat]) -> [Int] {
		return deficits.indices.sorted { deficits[$0] > deficits[$1] }
	}

	private func donorOrder(slack: [CGFloat]) -> [Int] {
		return slack.indices.sorted { slack[$0] > slack[$1] }
	}

	private func normalizedWidths(_ widths: [CGFloat], target: CGFloat) -> [CGFloat] {
		let delta = target - widths.reduce(0, +)
		guard abs(delta) > 0.5, let index = widths.indices.max(by: { widths[$0] < widths[$1] }) else {
			return widths
		}
		var out = widths
		out[index] = max(0, out[index] + delta)
		return out
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
		let inset = theme.tableStyle.borderWidth / 2
		path.move(to: CGPoint(x: inset, y: inset))
		path.addLine(to: CGPoint(x: inset, y: bounds.height - inset))
		for width in columnWidths {
			x += width
			let alignedX = min(max(x - inset, inset), bounds.width - inset)
			path.move(to: CGPoint(x: alignedX, y: inset))
			path.addLine(to: CGPoint(x: alignedX, y: bounds.height - inset))
		}
	}

	private func addHorizontalLines(to path: UIBezierPath) {
		var y: CGFloat = 0
		let inset = theme.tableStyle.borderWidth / 2
		path.move(to: CGPoint(x: inset, y: inset))
		path.addLine(to: CGPoint(x: bounds.width - inset, y: inset))
		for height in rowHeights {
			y += height
			let alignedY = min(max(y - inset, inset), bounds.height - inset)
			path.move(to: CGPoint(x: inset, y: alignedY))
			path.addLine(to: CGPoint(x: bounds.width - inset, y: alignedY))
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
			return inline.contentInsets.left +
				(inline.subviews.map { $0.tableMinIntrinsicWidth() }.max() ?? 0) +
				inline.contentInsets.right
		}
		if let label = self as? UILabel {
			return label.longestUnbreakableWidth()
		}
		if let override = self as? TableIntrinsicOverride {
			return override.tableMinimumWidth
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
		return TextIntrinsicMeasurer.longestUnbreakableWidth(text: text, font: font)
	}

	private func longestCharacterWidth(text: String, font: UIFont) -> CGFloat {
		return text.map { character in
			ceil((String(character) as NSString).size(withAttributes: [.font: font]).width)
		}.max() ?? 0
	}
}

private enum TextIntrinsicMeasurer {
	static func longestUnbreakableWidth(text: String, font: UIFont) -> CGFloat {
		var maxWidth = CGFloat(0)
		var token = ""
		for character in text {
			if isBreakable(character) {
				maxWidth = max(maxWidth, tokenWidth(token, font: font), characterWidth(character, font: font))
				token.removeAll(keepingCapacity: true)
			} else {
				token.append(character)
			}
		}
		return max(maxWidth, tokenWidth(token, font: font))
	}

	private static func tokenWidth(_ token: String, font: UIFont) -> CGFloat {
		if token.isEmpty {
			return 0
		}
		return ceil((token as NSString).size(withAttributes: [.font: font]).width)
	}

	private static func characterWidth(_ character: Character, font: UIFont) -> CGFloat {
		if String(character).trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
			return 0
		}
		return ceil((String(character) as NSString).size(withAttributes: [.font: font]).width)
	}

	private static func isBreakable(_ character: Character) -> Bool {
		return character.unicodeScalars.allSatisfy { scalar in
			CharacterSet.whitespacesAndNewlines.contains(scalar) ||
				isCjk(scalar) ||
				isBreakPunctuation(scalar)
		}
	}

	private static func isCjk(_ scalar: UnicodeScalar) -> Bool {
		let value = scalar.value
		return (0x4e00...0x9fff).contains(value) ||
			(0x3400...0x4dbf).contains(value) ||
			(0xf900...0xfaff).contains(value) ||
			(0x3040...0x309f).contains(value) ||
			(0x30a0...0x30ff).contains(value) ||
			(0xac00...0xd7af).contains(value)
	}

	private static func isBreakPunctuation(_ scalar: UnicodeScalar) -> Bool {
		return [",", ".", ";", ":", "，", "。", "；", "：", "/", "-", "_"].contains(String(scalar))
	}
}
#endif
