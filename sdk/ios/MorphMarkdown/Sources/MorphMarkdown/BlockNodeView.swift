import SwiftUI

struct BlockNodeView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	let node: MorphMarkdownNode
	let theme: MorphMarkdownTheme
	let mathRenderer: Math
	let imageLoader: Images

	var body: some View {
		switch node.kind {
		case "heading":
			Text(plainText(node))
				.font(theme.headingFont(level: node.level))
				.bold()
				.lineSpacing(max(0, theme.headingLineHeight(node.level) - theme.headingSize(node.level)))
				.padding(.top, theme.headingTopSpacingDp)
				.padding(.bottom, theme.headingBottomSpacingDp)
		case "paragraph":
			InlineNodeView(nodes: node.children, theme: theme, mathRenderer: mathRenderer, imageLoader: imageLoader)
				.padding(.top, theme.paragraphTopSpacingDp)
				.padding(.bottom, theme.paragraphBottomSpacingDp)
		case "list":
			ListNodeView(node: node, theme: theme, mathRenderer: mathRenderer, imageLoader: imageLoader)
		case "tasklist":
			TaskNodeView(node: node, theme: theme, mathRenderer: mathRenderer, imageLoader: imageLoader)
		case "table":
			TableNodeView(node: node, theme: theme, mathRenderer: mathRenderer, imageLoader: imageLoader)
		case "block_quote":
			quoteView()
		case "code_block":
			codeBlock()
		case "math_block":
			mathRenderer.render(latex: node.literal, display: true, theme: theme)
		case "image":
			imageLoader.image(url: node.url, theme: theme)
		case "thematic_break":
			Divider()
		default:
			Text(plainText(node)).font(theme.bodyFont()).lineSpacing(bodyLineSpacing(theme))
		}
	}

	private func quoteView() -> some View {
		HStack(alignment: .top) {
			Rectangle().fill(Color.secondary).frame(width: 4)
			Text(plainText(node)).font(theme.bodyFont())
				.lineSpacing(bodyLineSpacing(theme))
		}
		.padding(.top, theme.blockquoteVerticalPaddingDp)
		.padding(.bottom, theme.blockquoteBottomSpacingDp)
		.padding(.leading, theme.blockquoteIndentDp)
	}

	private func codeBlock() -> some View {
		Text(expandTabs(node.literal, tabSize: theme.codeBlockTabSize))
			.font(theme.codeFont)
			.padding(.horizontal, theme.codeBlockPaddingHorizontalDp)
			.padding(.vertical, theme.codeBlockPaddingVerticalDp)
			.background(Color(red: 0.93, green: 0.93, blue: 0.90))
	}
}

func plainText(_ node: MorphMarkdownNode) -> String {
	if node.children.isEmpty { return node.literal }
	return node.literal + node.children.map { plainText($0) }.joined()
}

func expandTabs(_ value: String, tabSize: Int) -> String {
	var out = ""
	var col = 0
	for ch in value {
		if ch == "\t" {
			let spaces = tabSize - (col % tabSize)
			out += String(repeating: " ", count: spaces)
			col += spaces
		} else {
			out.append(ch)
			col = ch == "\n" ? 0 : col + 1
		}
	}
	return out
}

func bodyLineSpacing(_ theme: MorphMarkdownTheme) -> CGFloat {
	max(0, theme.bodyTextSizeSp * theme.bodyLineHeightMultiplier - theme.bodyTextSizeSp)
}
