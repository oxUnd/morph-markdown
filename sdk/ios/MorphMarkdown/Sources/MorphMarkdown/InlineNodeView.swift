import SwiftUI

struct InlineNodeView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	let nodes: [MorphMarkdownNode]
	let theme: MorphMarkdownTheme
	let mathRenderer: Math
	let imageLoader: Images
	@State private var contentHeight: CGFloat = 0
	@State private var itemSizes: [String: CGSize] = [:]

	var body: some View {
		if canComposeText {
			composedText
				.lineSpacing(bodyLineSpacing(theme))
				.frame(maxWidth: .infinity, alignment: .leading)
		} else {
			GeometryReader { geometry in
				wrappingContent(width: geometry.size.width, items: renderItems)
			}
			.frame(height: contentHeight)
		}
	}

	private var canComposeText: Bool {
		nodes.allSatisfy { node in
			switch node.kind {
			case "text", "strong", "emphasis", "link":
				return true
			default:
				return false
			}
		}
	}

	private var renderItems: [InlineRenderItem] {
		var out: [InlineRenderItem] = []
		for node in nodes {
			appendItems(for: node, into: &out)
		}
		return out.enumerated().map { index, item in
			var next = item
			next.id = "\(index)-\(item.id)"
			return next
		}
	}

	private func appendItems(for node: MorphMarkdownNode, into out: inout [InlineRenderItem]) {
		switch node.kind {
		case "text":
			appendTextFragments(expandTabs(node.literal, tabSize: theme.tabSize), style: .plain, sourceID: node.id, into: &out)
		case "strong":
			appendTextFragments(plainText(node), style: .strong, sourceID: node.id, into: &out)
		case "emphasis":
			appendTextFragments(plainText(node), style: .emphasis, sourceID: node.id, into: &out)
		case "link":
			appendTextFragments(plainText(node), style: .link, sourceID: node.id, into: &out)
		case "code":
			out.append(.node(node, style: .code))
		default:
			out.append(.node(node, style: .plain))
		}
	}

	private func appendTextFragments(
		_ value: String,
		style: InlineRenderStyle,
		sourceID: Int,
		into out: inout [InlineRenderItem]
	) {
		for fragment in InlineTextFragmenter.fragments(value) {
			out.append(.text(fragment, style: style, sourceID: sourceID))
		}
	}

	private var composedText: Text {
		nodes.reduce(Text("")) { partial, node in
			partial + textFragment(node)
		}
	}

	private func textFragment(_ node: MorphMarkdownNode) -> Text {
		switch node.kind {
		case "code":
			return Text(expandTabs(node.literal, tabSize: theme.tabSize))
				.font(theme.inlineCodeFont)
		case "strong":
			return Text(plainText(node)).font(theme.bodyFont(bold: true))
		case "emphasis":
			return Text(plainText(node)).font(theme.bodyFont()).italic()
		case "link":
			return Text(plainText(node)).font(theme.bodyFont()).foregroundColor(.blue).underline()
		default:
			return Text(expandTabs(node.literal, tabSize: theme.tabSize)).font(theme.bodyFont())
		}
	}

	private func wrappingContent(width maxWidth: CGFloat, items: [InlineRenderItem]) -> some View {
		let layout = inlineLayout(width: maxWidth, items: items)
		return ZStack(alignment: .topLeading) {
			ForEach(items, id: \.id) { item in
				inlineItem(item)
					.fixedSize()
					.background(itemSizeReader(id: item.id))
					.offset(layout.positions[item.id] ?? .zero)
			}
		}
		.frame(height: layout.height, alignment: .topLeading)
		.onPreferenceChange(InlineItemSizeKey.self) { sizes in
			if itemSizes != sizes {
				itemSizes = sizes
			}
		}
		.onChange(of: layout.height) { height in
			if abs(contentHeight - height) > 0.5 {
				contentHeight = height
			}
		}
	}

	private func inlineLayout(width maxWidth: CGFloat, items: [InlineRenderItem]) -> InlineLayoutResult {
		let boundedWidth = max(1, maxWidth)
		let sizes = items.map { itemSize($0) }
		let lines = InlineLineBreaker.breakLines(
			items: sizes.map { InlineItemSize(width: $0.width, height: $0.height) },
			maxWidth: boundedWidth,
			minLineHeight: 0
		)
		var positions: [String: CGSize] = [:]
		var y: CGFloat = 0
		for line in lines {
			var x: CGFloat = 0
			for index in line.start..<line.end {
				let size = sizes[index]
				let itemY = y + max(0, line.height - size.height) / 2
				positions[items[index].id] = CGSize(width: x, height: itemY)
				x += size.width
			}
			y += line.height
		}
		return InlineLayoutResult(positions: positions, height: y)
	}

	private func itemSize(_ item: InlineRenderItem) -> CGSize {
		if let measured = itemSizes[item.id], measured.width > 0 || measured.height > 0 {
			return measured
		}
		switch item.content {
		case .text(let value):
			return textItemSize(value, style: item.style)
		case .node(let node):
			return nodeItemSize(node)
		}
	}

	private func textItemSize(_ value: String, style: InlineRenderStyle) -> CGSize {
		let size: CGFloat
		let monospace: Bool
		let bold: Bool
		switch style {
		case .code:
			size = theme.inlineCodeTextSizeSp
			monospace = true
			bold = false
		case .strong:
			size = theme.bodyTextSizeSp
			monospace = false
			bold = true
		default:
			size = theme.bodyTextSizeSp
			monospace = false
			bold = false
		}
		let width = MarkdownTextMeasurer.width(value, size: size, monospace: monospace, bold: bold)
		let height = size * theme.bodyLineHeightMultiplier
		if style == .code {
			return CGSize(
				width: width + theme.inlineCodePaddingHorizontalDp * 2,
				height: height + theme.inlineCodePaddingVerticalDp * 2
			)
		}
		return CGSize(width: width, height: height)
	}

	private func nodeItemSize(_ node: MorphMarkdownNode) -> CGSize {
		switch node.kind {
		case "code":
			return textItemSize(expandTabs(node.literal, tabSize: theme.tabSize), style: .code)
		case "math_inline":
			return CGSize(
				width: MarkdownTextMeasurer.width(node.literal, size: theme.mathSize(), monospace: true),
				height: theme.mathSize() * theme.bodyLineHeightMultiplier
			)
		case "image":
			return CGSize(width: theme.imageMaxWidthDp, height: theme.imageMaxHeightDp)
		default:
			let value = plainText(node)
			return CGSize(
				width: MarkdownTextMeasurer.width(value, size: theme.bodyTextSizeSp),
				height: theme.bodyTextSizeSp * theme.bodyLineHeightMultiplier
			)
		}
	}

	private func itemSizeReader(id: String) -> some View {
		GeometryReader { geometry in
			Color.clear.preference(key: InlineItemSizeKey.self, value: [id: geometry.size])
		}
	}

	@ViewBuilder
	private func inlineItem(_ item: InlineRenderItem) -> some View {
		switch item.content {
		case .text(let value):
			styledText(value, style: item.style)
		case .node(let node):
			inlineNode(node)
		}
	}

	@ViewBuilder
	private func styledText(_ value: String, style: InlineRenderStyle) -> some View {
		switch style {
		case .plain:
			Text(value).font(theme.bodyFont()).lineSpacing(bodyLineSpacing(theme))
		case .strong:
			Text(value).font(theme.bodyFont(bold: true)).lineSpacing(bodyLineSpacing(theme))
		case .emphasis:
			Text(value).font(theme.bodyFont()).italic().lineSpacing(bodyLineSpacing(theme))
		case .link:
			Text(value).font(theme.bodyFont()).foregroundColor(.blue).underline().lineSpacing(bodyLineSpacing(theme))
		case .code:
			Text(value)
				.font(theme.inlineCodeFont)
				.padding(.horizontal, theme.inlineCodePaddingHorizontalDp)
				.padding(.vertical, theme.inlineCodePaddingVerticalDp)
				.background(Color(red: 0.93, green: 0.93, blue: 0.90))
		}
	}

	@ViewBuilder
	private func inlineNode(_ node: MorphMarkdownNode) -> some View {
		switch node.kind {
		case "text":
			Text(expandTabs(node.literal, tabSize: theme.tabSize))
				.font(theme.bodyFont())
				.lineSpacing(bodyLineSpacing(theme))
		case "code":
			Text(expandTabs(node.literal, tabSize: theme.tabSize))
				.font(theme.inlineCodeFont)
				.padding(.horizontal, theme.inlineCodePaddingHorizontalDp)
				.padding(.vertical, theme.inlineCodePaddingVerticalDp)
				.background(Color(red: 0.93, green: 0.93, blue: 0.90))
		case "strong":
			Text(plainText(node))
				.font(theme.bodyFont(bold: true))
				.lineSpacing(bodyLineSpacing(theme))
		case "emphasis":
			Text(plainText(node))
				.font(theme.bodyFont())
				.italic()
				.lineSpacing(bodyLineSpacing(theme))
		case "link":
			Text(plainText(node))
				.font(theme.bodyFont())
				.foregroundColor(.blue)
				.underline()
				.lineSpacing(bodyLineSpacing(theme))
		case "math_inline":
			mathRenderer.render(latex: node.literal, display: false, theme: theme)
		case "image":
			imageLoader.image(url: node.url, theme: theme)
		default:
			Text(plainText(node)).font(theme.bodyFont()).lineSpacing(bodyLineSpacing(theme))
		}
	}
}

private enum InlineRenderContent {
	case text(String)
	case node(MorphMarkdownNode)
}

private enum InlineRenderStyle {
	case plain
	case strong
	case emphasis
	case link
	case code
}

private struct InlineRenderItem {
	var id: String
	let content: InlineRenderContent
	let style: InlineRenderStyle

	static func text(_ value: String, style: InlineRenderStyle, sourceID: Int) -> InlineRenderItem {
		InlineRenderItem(id: "\(sourceID)-text-\(value)", content: .text(value), style: style)
	}

	static func node(_ node: MorphMarkdownNode, style: InlineRenderStyle) -> InlineRenderItem {
		InlineRenderItem(id: "\(node.id)-node-\(node.kind)", content: .node(node), style: style)
	}
}

private struct InlineLayoutResult {
	let positions: [String: CGSize]
	let height: CGFloat
}

private struct InlineItemSizeKey: PreferenceKey {
	static var defaultValue: [String: CGSize] = [:]

	static func reduce(value: inout [String: CGSize], nextValue: () -> [String: CGSize]) {
		value.merge(nextValue(), uniquingKeysWith: { _, next in next })
	}
}
