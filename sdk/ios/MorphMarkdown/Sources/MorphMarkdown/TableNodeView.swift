import SwiftUI

struct TableNodeView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	let node: MorphMarkdownNode
	let theme: MorphMarkdownTheme
	let mathRenderer: Math
	let imageLoader: Images
	@State private var viewportWidth: CGFloat = 0

	var body: some View {
		Group {
			if theme.tableHorizontalScroll {
				ScrollView(.horizontal, showsIndicators: true) {
					tableContent
						.fixedSize(horizontal: true, vertical: false)
				}
			} else {
				tableContent
			}
		}
		.padding(.top, theme.tableTopSpacingDp)
		.padding(.bottom, theme.tableBottomSpacingDp)
		.background(widthReader)
	}

	private var tableContent: some View {
		tableContent(widths: columnWidths())
	}

	private func tableContent(widths: [CGFloat]) -> some View {
		VStack(alignment: .leading, spacing: 0) {
			ForEach(Array(node.children.enumerated()), id: \.element.id) { index, row in
				tableRow(row, header: index == 0, widths: widths)
			}
		}
		.border(theme.tableStyle.borderColor, width: theme.tableStyle.borderWidthDp)
	}

	private func tableRow(_ row: MorphMarkdownNode, header: Bool, widths: [CGFloat]) -> some View {
		HStack(alignment: .top, spacing: 0) {
			ForEach(Array(row.children.enumerated()), id: \.element.id) { index, cell in
				cellView(cell, header: header, width: widths[safe: index] ?? theme.tableCellMaxWidthDp)
			}
		}
	}

	private func cellView(_ cell: MorphMarkdownNode, header: Bool, width: CGFloat) -> some View {
		let contentWidth = cellContentWidth(width)
		return InlineNodeView(
			nodes: inlineChildren(of: cell),
			theme: tableTheme(header: header, contentWidth: contentWidth),
			mathRenderer: mathRenderer,
			imageLoader: imageLoader
		)
		.frame(width: contentWidth, alignment: .leading)
		.fixedSize(horizontal: false, vertical: true)
		.lineSpacing(theme.tableCellLineSpacingDp)
		.padding(.horizontal, theme.tableCellPaddingHorizontalDp)
		.padding(.vertical, theme.tableCellPaddingVerticalDp)
		.foregroundColor(header ? theme.tableStyle.headerTextColor : theme.tableStyle.bodyTextColor)
		.background(header ? theme.tableStyle.headerBackgroundColor : theme.tableStyle.bodyBackgroundColor)
		.border(theme.tableStyle.borderColor, width: theme.tableStyle.borderWidthDp)
	}

	private func tableTheme(header: Bool, contentWidth: CGFloat) -> MorphMarkdownTheme {
		var next = theme
		if header, let size = theme.tableStyle.headerTextSizeSp {
			next.bodyTextSizeSp = size
		}
		if !header, let size = theme.tableStyle.bodyTextSizeSp {
			next.bodyTextSizeSp = size
		}
		next.bodyLineHeightMultiplier = theme.tableCellLineHeightMultiplier
		next.imageMaxWidthDp = min(theme.imageMaxWidthDp, contentWidth)
		next.mathTextScale = theme.tableMathTextScale
		return next
	}

	private var widthReader: some View {
		GeometryReader { geometry in
			Color.clear.preference(key: TableViewportWidthKey.self, value: geometry.size.width)
		}
		.onPreferenceChange(TableViewportWidthKey.self) { width in
			if abs(width - viewportWidth) > 0.5 {
				viewportWidth = width
			}
		}
	}

	private func columnWidths() -> [CGFloat] {
		let rows = node.children
		let columnCount = rows.map { $0.children.count }.max() ?? 0
		let cells = rows.flatMap { row in
			row.children.enumerated().map { index, cell in
				cellWidth(cell, column: index)
			}
		}
		let available = viewportWidth > 0 ? viewportWidth : nil
		return TableColumnSizer.sizeColumns(
			cells: cells,
			columnCount: columnCount,
			availableWidth: available,
			maxColumnWidth: theme.tableCellMaxWidthDp,
			wrap: theme.tableCellWrap
		)
	}

	private func cellWidth(_ cell: MorphMarkdownNode, column: Int) -> TableCellWidth {
		let nodes = inlineChildren(of: cell)
		let minWidth = cellOuterWidth(intrinsicWidth(nodes, preferred: false))
		let preferredWidth = cellOuterWidth(intrinsicWidth(nodes, preferred: true))
		return TableCellWidth(
			column: column,
			minWidth: readableMinWidth(minWidth: minWidth, preferredWidth: preferredWidth),
			preferredWidth: preferredWidth
		)
	}

	private func cellOuterWidth(_ contentWidth: CGFloat) -> CGFloat {
		contentWidth + theme.tableCellPaddingHorizontalDp * 2
	}

	private func cellContentWidth(_ outerWidth: CGFloat) -> CGFloat {
		max(1, outerWidth - theme.tableCellPaddingHorizontalDp * 2)
	}

	private func readableMinWidth(minWidth: CGFloat, preferredWidth: CGFloat) -> CGFloat {
		let floor = min(preferredWidth, cellOuterWidth(max(72, theme.bodyTextSizeSp * 4)))
		return max(minWidth, floor)
	}

	private func intrinsicWidth(_ nodes: [MorphMarkdownNode], preferred: Bool) -> CGFloat {
		nodes.reduce(CGFloat(0)) { total, node in
			total + intrinsicNodeWidth(node, preferred: preferred)
		}
	}

	private func intrinsicNodeWidth(_ node: MorphMarkdownNode, preferred: Bool) -> CGFloat {
		switch node.kind {
		case "text":
			return textWidth(expandTabs(node.literal, tabSize: theme.tabSize), preferred: preferred)
		case "code":
			let value = expandTabs(node.literal, tabSize: theme.tabSize)
			return textWidth(value, preferred: preferred, monospace: true, size: theme.inlineCodeTextSizeSp) +
				theme.inlineCodePaddingHorizontalDp * 2
		case "strong":
			return textWidth(plainText(node), preferred: preferred, bold: true)
		case "emphasis", "link":
			return textWidth(plainText(node), preferred: preferred)
		case "math_inline":
			return MarkdownTextMeasurer.width(node.literal, size: theme.mathSize(), monospace: true) + 8
		case "image":
			return min(theme.imageMaxWidthDp, theme.tableCellMaxWidthDp)
		default:
			return textWidth(plainText(node), preferred: preferred)
		}
	}

	private func textWidth(
		_ value: String,
		preferred: Bool,
		monospace: Bool = false,
		bold: Bool = false,
		size: CGFloat? = nil
	) -> CGFloat {
		let fontSize = size ?? theme.bodyTextSizeSp
		if preferred {
			return MarkdownTextMeasurer.width(value, size: fontSize, monospace: monospace, bold: bold)
		}
		return MarkdownTextMeasurer.longestUnbreakableWidth(value, size: fontSize, monospace: monospace, bold: bold)
	}

	private func inlineChildren(of cell: MorphMarkdownNode) -> [MorphMarkdownNode] {
		if let paragraph = cell.children.first(where: { $0.kind == "paragraph" }) {
			return paragraph.children
		}
		return cell.children
	}
}

private struct TableViewportWidthKey: PreferenceKey {
	static var defaultValue: CGFloat = 0

	static func reduce(value: inout CGFloat, nextValue: () -> CGFloat) {
		value = max(value, nextValue())
	}
}

private extension Array {
	subscript(safe index: Index) -> Element? {
		indices.contains(index) ? self[index] : nil
	}
}
