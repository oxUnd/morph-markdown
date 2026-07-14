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
		VStack(alignment: .leading, spacing: theme.listItemSpacingDp) {
			ForEach(Array(node.children.enumerated()), id: \.element.id) { index, child in
				HStack(alignment: .top, spacing: 0) {
					marker(index: index)
					listItem(child)
				}
			}
		}
		.padding(.leading, indent)
	}

	@ViewBuilder
	private func listItem(_ child: MorphMarkdownNode) -> some View {
		if child.kind == "list_item" {
			VStack(alignment: .leading, spacing: 4) {
				ForEach(child.children) { itemChild in
					if itemChild.kind == "list" {
						ListNodeView(
							node: itemChild,
							theme: theme,
							mathRenderer: mathRenderer,
							imageLoader: imageLoader,
							depth: depth + 1
						)
					} else {
						BlockNodeView(
							node: itemChild,
							theme: theme,
							mathRenderer: mathRenderer,
							imageLoader: imageLoader
						)
					}
				}
			}
		} else {
			BlockNodeView(node: child, theme: theme, mathRenderer: mathRenderer, imageLoader: imageLoader)
		}
	}

	private var indent: CGFloat {
		return depth == 0 ? theme.listIndentDp : theme.nestedListIndentDp
	}

	@ViewBuilder
	private func marker(index: Int) -> some View {
		if node.listType == "ordered" {
			Text("\(node.start + index).")
				.font(theme.bodyFont())
				.frame(width: theme.orderedMarkerWidthDp, alignment: .center)
				.padding(.top, markerTopOffset)
		} else {
			ListMarkerShape(style: unorderedMarker(index: index), theme: theme)
				.padding(.top, markerTopOffset + (theme.bodyTextSizeSp - theme.listMarkerSizeDp) / 2)
		}
	}

	private var markerTopOffset: CGFloat {
		max(0, (theme.bodyTextSizeSp * theme.bodyLineHeightMultiplier - theme.bodyTextSizeSp) / 2)
	}

	private func unorderedMarker(index: Int) -> MorphListMarkerStyle {
		if theme.unorderedListMarkers.isEmpty { return .disc }
		return theme.unorderedListMarkers[depth % theme.unorderedListMarkers.count]
	}
}

struct TaskNodeView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	let node: MorphMarkdownNode
	let theme: MorphMarkdownTheme
	let mathRenderer: Math
	let imageLoader: Images

	var body: some View {
		HStack(alignment: .top, spacing: 0) {
			Image(systemName: node.checked ? "checkmark.square.fill" : "square")
				.frame(width: theme.taskBoxSizeDp, height: theme.taskBoxSizeDp, alignment: .center)
				.padding(.trailing, theme.taskBoxTextGapDp)
				.padding(.top, taskTopOffset)
			InlineNodeView(nodes: inlineChildren, theme: theme, mathRenderer: mathRenderer, imageLoader: imageLoader)
		}
	}

	private var taskTopOffset: CGFloat {
		max(0, (theme.bodyTextSizeSp * theme.bodyLineHeightMultiplier - theme.taskBoxSizeDp) / 2)
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
				Circle().fill(Color.primary).frame(width: theme.listMarkerSizeDp, height: theme.listMarkerSizeDp)
			case .circle:
				Circle().stroke(Color.primary, lineWidth: 1.5).frame(width: theme.listMarkerSizeDp, height: theme.listMarkerSizeDp)
			case .square:
				Rectangle().fill(Color.primary).frame(width: theme.listMarkerSizeDp, height: theme.listMarkerSizeDp)
			case .hyphen:
				Rectangle().fill(Color.primary).frame(width: theme.listMarkerSizeDp, height: 1.5)
			}
		}
		.frame(width: theme.listMarkerWidthDp, alignment: .center)
	}
}
