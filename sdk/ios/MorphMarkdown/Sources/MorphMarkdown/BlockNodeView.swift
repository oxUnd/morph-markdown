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
				.padding(.top, theme.headingTopSpacing)
				.padding(.bottom, theme.headingBottomSpacing)
		case "paragraph":
			InlineNodeView(nodes: node.children, theme: theme, mathRenderer: mathRenderer, imageLoader: imageLoader)
				.padding(.vertical, theme.paragraphVerticalSpacing * 0.5)
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
			Text(plainText(node)).font(theme.bodyFont).lineSpacing(theme.bodyLineSpacing)
		}
	}

	private func quoteView() -> some View {
		HStack(alignment: .top) {
			Rectangle().fill(Color.secondary).frame(width: 4)
			Text(plainText(node)).font(theme.bodyFont)
				.lineSpacing(theme.bodyLineSpacing)
		}
		.padding(.leading, theme.blockquoteIndent)
	}

	private func codeBlock() -> some View {
		Text(expandTabs(node.literal, tabSize: theme.tabSize))
			.font(theme.codeFont)
			.padding(10)
			.background(Color.secondary.opacity(0.12))
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
