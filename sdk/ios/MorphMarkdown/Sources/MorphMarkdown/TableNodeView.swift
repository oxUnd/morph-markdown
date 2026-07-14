import SwiftUI

struct TableNodeView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	let node: MorphMarkdownNode
	let theme: MorphMarkdownTheme
	let mathRenderer: Math
	let imageLoader: Images

	var body: some View {
		ScrollView(.horizontal) {
			VStack(alignment: .leading, spacing: 0) {
				ForEach(Array(node.children.enumerated()), id: \.element.id) { index, row in
					tableRow(row, header: index == 0)
				}
			}
			.border(theme.tableStyle.borderColor, width: theme.tableStyle.borderWidth)
		}
	}

	private func tableRow(_ row: MorphMarkdownNode, header: Bool) -> some View {
		HStack(alignment: .top, spacing: 0) {
			ForEach(row.children) { cell in
				cellView(cell, header: header)
			}
		}
	}

	private func cellView(_ cell: MorphMarkdownNode, header: Bool) -> some View {
		InlineNodeView(
			nodes: cell.children,
			theme: tableTheme(header: header),
			mathRenderer: mathRenderer,
			imageLoader: imageLoader
		)
		.frame(maxWidth: theme.tableCellMaxWidth, alignment: .leading)
		.padding(.horizontal, theme.tableCellPaddingHorizontal)
		.padding(.vertical, theme.tableCellPaddingVertical)
		.foregroundColor(header ? theme.tableStyle.headerTextColor : theme.tableStyle.bodyTextColor)
		.background(header ? theme.tableStyle.headerBackgroundColor : theme.tableStyle.bodyBackgroundColor)
		.border(theme.tableStyle.borderColor, width: theme.tableStyle.borderWidth)
	}

	private func tableTheme(header: Bool) -> MorphMarkdownTheme {
		var next = theme
		if header, let font = theme.tableStyle.headerFont {
			next.bodyFont = font
		}
		if !header, let font = theme.tableStyle.bodyFont {
			next.bodyFont = font
		}
		return next
	}
}
