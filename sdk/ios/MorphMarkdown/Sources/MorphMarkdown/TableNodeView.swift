import SwiftUI

struct TableNodeView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	let node: MorphMarkdownNode
	let theme: MorphMarkdownTheme
	let mathRenderer: Math
	let imageLoader: Images
	@State private var viewportWidth: CGFloat = 0
	@State private var measuredCellHeights: [TableCellKey: CGFloat] = [:]

	var body: some View {
		tableContainer
			.padding(.top, theme.tableTopSpacingDp)
			.padding(.bottom, theme.tableBottomSpacingDp)
			.background(widthReader)
	}

	@ViewBuilder
	private var tableContainer: some View {
		if theme.tableHorizontalScroll {
			ScrollViewReader { proxy in
				ScrollView(.horizontal, showsIndicators: true) {
					HStack(spacing: 0) {
						Color.clear.frame(width: 0, height: 1).id(tableLeadingID)
						tableSurface(layout: tableLayout())
					}
					.frame(maxWidth: .infinity, alignment: .leading)
				}
				.onAppear {
					proxy.scrollTo(tableLeadingID, anchor: .leading)
				}
				.onChange(of: viewportWidth) { _ in
					proxy.scrollTo(tableLeadingID, anchor: .leading)
				}
			}
		} else {
			tableSurface(layout: tableLayout())
		}
	}

	private var tableLeadingID: String {
		"morph-table-leading-\(node.id)"
	}

	private func tableSurface(layout: MarkdownTableLayout) -> some View {
		ZStack(alignment: .topLeading) {
			tableBackground(layout: layout)
			ForEach(layout.cells) { cell in
				cellView(cell)
			}
			tableGrid(layout: layout)
		}
		.frame(width: layout.width, height: layout.height, alignment: .topLeading)
		.onPreferenceChange(TableCellHeightKey.self) { heights in
			if measuredCellHeights != heights {
				measuredCellHeights = heights
			}
		}
	}

	private func cellView(_ cell: MarkdownTableCellLayout) -> some View {
		InlineNodeView(
			nodes: inlineChildren(of: cell.node),
			theme: tableTheme(header: cell.header, contentWidth: cell.contentRect.width),
			mathRenderer: mathRenderer,
			imageLoader: imageLoader
		)
		.frame(width: cell.contentRect.width, alignment: .leading)
		.fixedSize(horizontal: false, vertical: true)
		.foregroundColor(cell.header ? theme.tableStyle.headerTextColor : theme.tableStyle.bodyTextColor)
		.background(cellHeightReader(key: cell.key))
		.offset(x: cell.contentRect.minX, y: cell.contentRect.minY)
	}

	private func tableBackground(layout: MarkdownTableLayout) -> some View {
		ZStack(alignment: .topLeading) {
			ForEach(layout.rows) { row in
				(row.header ? theme.tableStyle.headerBackgroundColor : theme.tableStyle.bodyBackgroundColor)
					.frame(width: layout.width, height: row.height)
					.offset(x: 0, y: row.y)
			}
		}
	}

	private func tableGrid(layout: MarkdownTableLayout) -> some View {
		Path { path in
			for x in layout.verticalLines {
				path.move(to: CGPoint(x: x, y: 0))
				path.addLine(to: CGPoint(x: x, y: layout.height))
			}
			for y in layout.horizontalLines {
				path.move(to: CGPoint(x: 0, y: y))
				path.addLine(to: CGPoint(x: layout.width, y: y))
			}
		}
		.stroke(theme.tableStyle.borderColor, lineWidth: theme.tableStyle.borderWidthDp)
		.allowsHitTesting(false)
	}

	private func tableLayout() -> MarkdownTableLayout {
		MarkdownTableLayoutBuilder(
			node: node,
			theme: theme,
			viewportWidth: viewportWidth,
			measuredCellHeights: measuredCellHeights
		).layout()
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

	private func cellHeightReader(key: TableCellKey) -> some View {
		GeometryReader { geometry in
			Color.clear.preference(key: TableCellHeightKey.self, value: [key: geometry.size.height])
		}
	}

	private func inlineChildren(of cell: MorphMarkdownNode) -> [MorphMarkdownNode] {
		if let paragraph = cell.children.first(where: { $0.kind == "paragraph" }) {
			return paragraph.children
		}
		return cell.children
	}
}

private struct MarkdownTableLayout {
	let width: CGFloat
	let height: CGFloat
	let rows: [MarkdownTableRowLayout]
	let cells: [MarkdownTableCellLayout]
	let verticalLines: [CGFloat]
	let horizontalLines: [CGFloat]
}

private struct MarkdownTableRowLayout: Identifiable {
	let id: Int
	let y: CGFloat
	let height: CGFloat
	let header: Bool
}

private struct MarkdownTableCellLayout: Identifiable {
	let id: TableCellKey
	let key: TableCellKey
	let node: MorphMarkdownNode
	let header: Bool
	let contentRect: CGRect
}

private struct TableCellKey: Hashable {
	let row: Int
	let column: Int
}

private struct MarkdownTableLayoutBuilder {
	let node: MorphMarkdownNode
	let theme: MorphMarkdownTheme
	let viewportWidth: CGFloat
	let measuredCellHeights: [TableCellKey: CGFloat]

	func layout() -> MarkdownTableLayout {
		let widths = columnWidths()
		let heights = rowHeights(widths: widths)
		return tableLayout(widths: widths, heights: heights)
	}

	private func tableLayout(widths: [CGFloat], heights: [CGFloat]) -> MarkdownTableLayout {
		let rowOrigins = offsets(heights)
		let columnOrigins = offsets(widths)
		let tableWidth = widths.reduce(0, +)
		let tableHeight = heights.reduce(0, +)
		return MarkdownTableLayout(
			width: tableWidth,
			height: tableHeight,
			rows: rows(rowOrigins: rowOrigins, heights: heights),
			cells: cells(rowOrigins: rowOrigins, columnOrigins: columnOrigins, widths: widths, heights: heights),
			verticalLines: linePositions(sizes: widths),
			horizontalLines: linePositions(sizes: heights)
		)
	}

	private func rows(rowOrigins: [CGFloat], heights: [CGFloat]) -> [MarkdownTableRowLayout] {
		node.children.indices.map { row in
			MarkdownTableRowLayout(id: row, y: rowOrigins[safe: row] ?? 0, height: heights[safe: row] ?? 0, header: row == 0)
		}
	}

	private func cells(
		rowOrigins: [CGFloat],
		columnOrigins: [CGFloat],
		widths: [CGFloat],
		heights: [CGFloat]
	) -> [MarkdownTableCellLayout] {
		node.children.enumerated().flatMap { row, rowNode in
			rowNode.children.enumerated().map { column, cellNode in
				let key = TableCellKey(row: row, column: column)
				return MarkdownTableCellLayout(
					id: key,
					key: key,
					node: cellNode,
					header: row == 0,
					contentRect: contentRect(row: row, column: column, rowOrigins: rowOrigins, columnOrigins: columnOrigins, widths: widths)
				)
			}
		}
	}

	private func contentRect(
		row: Int,
		column: Int,
		rowOrigins: [CGFloat],
		columnOrigins: [CGFloat],
		widths: [CGFloat]
	) -> CGRect {
		let x = (columnOrigins[safe: column] ?? 0) + theme.tableCellPaddingHorizontalDp
		let y = (rowOrigins[safe: row] ?? 0) + theme.tableCellPaddingVerticalDp
		let width = cellContentWidth(widths[safe: column] ?? theme.tableCellMaxWidthDp)
		return CGRect(x: x, y: y, width: width, height: 0)
	}

	private func columnWidths() -> [CGFloat] {
		let columnCount = node.children.map { $0.children.count }.max() ?? 0
		let inputs = node.children.flatMap { row in
			row.children.enumerated().map { index, cell in
				cellWidth(cell, column: index)
			}
		}
		let available = viewportWidth > 0 ? viewportWidth : nil
		return TableColumnSizer.sizeColumns(
			cells: inputs,
			columnCount: columnCount,
			availableWidth: available,
			maxColumnWidth: theme.tableCellMaxWidthDp,
			wrap: theme.tableCellWrap
		)
	}

	private func rowHeights(widths: [CGFloat]) -> [CGFloat] {
		node.children.enumerated().map { row, rowNode in
			let header = row == 0
			let cells = rowNode.children.enumerated().map { column, cell in
				cellHeight(cell, row: row, column: column, header: header, width: widths[safe: column] ?? theme.tableCellMaxWidthDp)
			}
			return cells.max() ?? defaultCellHeight(header: header)
		}
	}

	private func cellHeight(_ cell: MorphMarkdownNode, row: Int, column: Int, header: Bool, width: CGFloat) -> CGFloat {
		let key = TableCellKey(row: row, column: column)
		let estimated = estimatedContentHeight(cell, header: header, width: cellContentWidth(width))
		let contentHeight = measuredCellHeights[key].flatMap { $0 > 0 ? $0 : nil } ?? estimated
		return contentHeight + theme.tableCellPaddingVerticalDp * 2
	}

	private func cellWidth(_ cell: MorphMarkdownNode, column: Int) -> TableCellWidth {
		let nodes = inlineChildren(of: cell)
		let minWidth = cellOuterWidth(intrinsicWidth(nodes, preferred: false))
		let preferredWidth = cellOuterWidth(intrinsicWidth(nodes, preferred: true))
		return TableCellWidth(column: column, minWidth: minWidth, preferredWidth: preferredWidth)
	}

	private func estimatedContentHeight(_ cell: MorphMarkdownNode, header: Bool, width: CGFloat) -> CGFloat {
		let itemSizes = inlineChildren(of: cell).flatMap { inlineItemSizes(node: $0, header: header) }
		let lineHeight = tableLineHeight(header: header)
		let lines = InlineLineBreaker.breakLines(items: itemSizes, maxWidth: max(1, width), minLineHeight: lineHeight)
		return max(lineHeight, lines.map(\.height).reduce(0, +) + theme.tableCellLineSpacingDp * CGFloat(max(0, lines.count - 1)))
	}

	private func inlineItemSizes(node: MorphMarkdownNode, header: Bool) -> [InlineItemSize] {
		switch node.kind {
		case "text":
			return textItemSizes(expandTabs(node.literal, tabSize: theme.tabSize), header: header)
		case "strong":
			return textItemSizes(plainText(node), header: header, bold: true)
		case "emphasis", "link":
			return textItemSizes(plainText(node), header: header)
		case "code":
			return [codeItemSize(node.literal)]
		case "math_inline":
			return [mathItemSize(node.literal)]
		case "image":
			return [InlineItemSize(width: theme.imageMaxWidthDp, height: theme.imageMaxHeightDp + 12)]
		default:
			return textItemSizes(plainText(node), header: header)
		}
	}

	private func textItemSizes(_ value: String, header: Bool, bold: Bool = false) -> [InlineItemSize] {
		InlineTextFragmenter.fragments(value).map { fragment in
			InlineItemSize(width: textWidth(fragment, header: header, bold: bold), height: tableLineHeight(header: header))
		}
	}

	private func codeItemSize(_ value: String) -> InlineItemSize {
		let text = expandTabs(value, tabSize: theme.tabSize)
		let width = MarkdownTextMeasurer.width(text, size: theme.inlineCodeTextSizeSp, monospace: true) +
			theme.inlineCodePaddingHorizontalDp * 2
		let capped = theme.tableCellWrap ? min(width, theme.tableCellMaxWidthDp * 0.78) : width
		return InlineItemSize(width: capped, height: tableLineHeight(header: false) + theme.inlineCodePaddingVerticalDp * 2)
	}

	private func mathItemSize(_ value: String) -> InlineItemSize {
		InlineItemSize(
			width: MarkdownTextMeasurer.width(value, size: theme.mathSize(), monospace: true),
			height: theme.mathSize() * theme.tableCellLineHeightMultiplier
		)
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
			let width = textWidth(expandTabs(node.literal, tabSize: theme.tabSize), preferred: preferred, monospace: true, size: theme.inlineCodeTextSizeSp) +
				theme.inlineCodePaddingHorizontalDp * 2
			return theme.tableCellWrap ? min(width, theme.tableCellMaxWidthDp * 0.78) : width
		case "strong":
			return textWidth(plainText(node), preferred: preferred, bold: true)
		case "emphasis", "link":
			return textWidth(plainText(node), preferred: preferred)
		case "math_inline":
			return MarkdownTextMeasurer.width(node.literal, size: theme.mathSize(), monospace: true)
		case "image":
			return theme.imageMaxWidthDp
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

	private func textWidth(_ value: String, header: Bool, bold: Bool = false) -> CGFloat {
		let size = tableTextSize(header: header)
		let isBold = bold || (header && theme.tableStyle.headerBold)
		return MarkdownTextMeasurer.width(value, size: size, bold: isBold)
	}

	private func tableTextSize(header: Bool) -> CGFloat {
		header ? (theme.tableStyle.headerTextSizeSp ?? theme.bodyTextSizeSp) :
			(theme.tableStyle.bodyTextSizeSp ?? theme.bodyTextSizeSp)
	}

	private func tableLineHeight(header: Bool) -> CGFloat {
		tableTextSize(header: header) * theme.tableCellLineHeightMultiplier
	}

	private func defaultCellHeight(header: Bool) -> CGFloat {
		tableLineHeight(header: header) + theme.tableCellPaddingVerticalDp * 2
	}

	private func cellOuterWidth(_ contentWidth: CGFloat) -> CGFloat {
		contentWidth + theme.tableCellPaddingHorizontalDp * 2
	}

	private func cellContentWidth(_ outerWidth: CGFloat) -> CGFloat {
		max(1, outerWidth - theme.tableCellPaddingHorizontalDp * 2)
	}

	private func inlineChildren(of cell: MorphMarkdownNode) -> [MorphMarkdownNode] {
		if let paragraph = cell.children.first(where: { $0.kind == "paragraph" }) {
			return paragraph.children
		}
		return cell.children
	}

	private func offsets(_ values: [CGFloat]) -> [CGFloat] {
		var total: CGFloat = 0
		return values.map { value in
			defer { total += value }
			return total
		}
	}

	private func linePositions(sizes: [CGFloat]) -> [CGFloat] {
		var positions: [CGFloat] = [0]
		var total: CGFloat = 0
		for size in sizes {
			total += size
			positions.append(total)
		}
		return positions
	}
}

private struct TableViewportWidthKey: PreferenceKey {
	static var defaultValue: CGFloat = 0

	static func reduce(value: inout CGFloat, nextValue: () -> CGFloat) {
		value = max(value, nextValue())
	}
}

private struct TableCellHeightKey: PreferenceKey {
	static var defaultValue: [TableCellKey: CGFloat] = [:]

	static func reduce(value: inout [TableCellKey: CGFloat], nextValue: () -> [TableCellKey: CGFloat]) {
		value.merge(nextValue(), uniquingKeysWith: { _, next in next })
	}
}

private extension Array {
	subscript(safe index: Index) -> Element? {
		indices.contains(index) ? self[index] : nil
	}
}
