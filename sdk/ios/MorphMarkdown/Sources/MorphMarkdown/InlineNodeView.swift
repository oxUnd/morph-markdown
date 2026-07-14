import SwiftUI

struct InlineNodeView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	let nodes: [MorphMarkdownNode]
	let theme: MorphMarkdownTheme
	let mathRenderer: Math
	let imageLoader: Images

	var body: some View {
		FlowLayout {
			ForEach(nodes) { node in
				inlineNode(node)
			}
		}
	}

	@ViewBuilder
	private func inlineNode(_ node: MorphMarkdownNode) -> some View {
		switch node.kind {
		case "text":
			Text(expandTabs(node.literal, tabSize: theme.tabSize))
				.font(theme.bodyFont)
				.lineSpacing(theme.bodyLineSpacing)
		case "code":
			Text(expandTabs(node.literal, tabSize: theme.tabSize))
				.font(theme.inlineCodeFont)
				.padding(.horizontal, 4)
				.background(Color.secondary.opacity(0.12))
		case "math_inline":
			mathRenderer.render(latex: node.literal, display: false, theme: theme)
		case "image":
			imageLoader.image(url: node.url, theme: theme)
		default:
			Text(plainText(node)).font(theme.bodyFont).lineSpacing(theme.bodyLineSpacing)
		}
	}
}

struct FlowLayout<Content: View>: View {
	@ViewBuilder let content: Content

	var body: some View {
		HStack(alignment: .firstTextBaseline, spacing: 0) {
			content
		}
	}
}
