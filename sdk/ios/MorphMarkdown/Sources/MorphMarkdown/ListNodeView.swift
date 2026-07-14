import SwiftUI

struct ListNodeView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	let node: MorphMarkdownNode
	let theme: MorphMarkdownTheme
	let mathRenderer: Math
	let imageLoader: Images
	let depth: Int

	init(
		node: MorphMarkdownNode,
		theme: MorphMarkdownTheme,
		mathRenderer: Math,
		imageLoader: Images,
		depth: Int = 0
	) {
		self.node = node
		self.theme = theme
		self.mathRenderer = mathRenderer
		self.imageLoader = imageLoader
		self.depth = depth
	}

	var body: some View {
		VStack(alignment: .leading, spacing: 4) {
			ForEach(Array(node.children.enumerated()), id: \.element.id) { index, child in
				HStack(alignment: .firstTextBaseline) {
					marker(index: index)
					BlockNodeView(node: child, theme: theme, mathRenderer: mathRenderer, imageLoader: imageLoader)
				}
			}
		}
		.padding(.leading, indent)
	}

	private var indent: CGFloat {
		return depth == 0 ? theme.listIndent : theme.nestedListIndent
	}

	@ViewBuilder
	private func marker(index: Int) -> some View {
		if node.listType == "ordered" {
			Text("\(node.start + index).")
				.frame(width: theme.listMarkerWidth, alignment: .center)
		} else {
			ListMarkerShape(style: unorderedMarker(index: index), theme: theme)
		}
	}

	private func unorderedMarker(index: Int) -> MorphListMarkerStyle {
		if theme.unorderedListMarkers.isEmpty { return .disc }
		return theme.unorderedListMarkers[index % theme.unorderedListMarkers.count]
	}
}

struct TaskNodeView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	let node: MorphMarkdownNode
	let theme: MorphMarkdownTheme
	let mathRenderer: Math
	let imageLoader: Images

	var body: some View {
		HStack(alignment: .firstTextBaseline) {
			Image(systemName: node.checked ? "checkmark.square.fill" : "square")
				.frame(width: theme.listMarkerWidth, alignment: .center)
			InlineNodeView(nodes: inlineChildren, theme: theme, mathRenderer: mathRenderer, imageLoader: imageLoader)
		}
	}

	private var inlineChildren: [MorphMarkdownNode] {
		return node.children.first(where: { $0.kind == "paragraph" })?.children ?? node.children
	}
}

struct ListMarkerShape: View {
	let style: MorphListMarkerStyle
	let theme: MorphMarkdownTheme

	var body: some View {
		ZStack {
			switch style {
			case .disc:
				Circle().fill(Color.primary).frame(width: theme.listMarkerSize, height: theme.listMarkerSize)
			case .circle:
				Circle().stroke(Color.primary, lineWidth: 1.5).frame(width: theme.listMarkerSize, height: theme.listMarkerSize)
			case .square:
				Rectangle().fill(Color.primary).frame(width: theme.listMarkerSize, height: theme.listMarkerSize)
			case .hyphen:
				Rectangle().fill(Color.primary).frame(width: theme.listMarkerSize, height: 1.5)
			}
		}
		.frame(width: theme.listMarkerWidth, alignment: .center)
	}
}
