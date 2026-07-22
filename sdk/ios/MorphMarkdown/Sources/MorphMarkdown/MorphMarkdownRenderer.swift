#if canImport(UIKit)
import UIKit

enum TableCellRole {
	case none
	case header
	case body
}

private struct RenderedBlock {
	let signature: Int
	let viewCount: Int
}

final class MorphMarkdownRenderer {
	var theme: MorphMarkdownTheme = MorphMarkdownThemes.normal
	var mathRenderer: MorphMathRenderer?
	var imageLoader: MorphImageLoader = FileImageLoader()
	var viewportWidthOverride: CGFloat?
	var onLinkClick: MorphMarkdownLinkHandler?
	private var renderedBlocks: [RenderedBlock] = []
	private var progressiveNodes: [MarkdownNode] = []
	private var progressiveIndex = 0

	func reset(parent: UIStackView) {
		parent.removeAllArrangedSubviews()
		renderedBlocks = []
		cancelProgressiveRender()
	}

	func render(json: String, parent: UIStackView) {
		cancelProgressiveRender()
		parent.removeAllArrangedSubviews()
		guard let data = json.data(using: .utf8),
		      let root = try? JSONDecoder().decode(MarkdownNode.self, from: data) else {
			parent.addArrangedSubview(configuredLabel("snapshot failed", size: theme.bodyTextSize, theme: theme))
			return
		}
		renderedBlocks = renderChildren(root.children, from: 0, parent: parent)
	}

	func renderProgressively(json: String, parent: UIStackView, initialBlockCount: Int) -> Bool {
		parent.removeAllArrangedSubviews()
		guard let data = json.data(using: .utf8),
		      let root = try? JSONDecoder().decode(MarkdownNode.self, from: data) else {
			parent.addArrangedSubview(configuredLabel("snapshot failed", size: theme.bodyTextSize, theme: theme))
			return false
		}
		progressiveNodes = root.children
		progressiveIndex = min(max(0, initialBlockCount), progressiveNodes.count)
		renderedBlocks = renderChildren(progressiveNodes, from: 0, to: progressiveIndex, parent: parent)
		return progressiveIndex < progressiveNodes.count
	}

	func renderNextProgressiveBatch(parent: UIStackView, count: Int) -> Bool {
		guard progressiveIndex < progressiveNodes.count else { return false }
		let end = min(progressiveNodes.count, progressiveIndex + max(1, count))
		let batch = renderChildren(progressiveNodes, from: progressiveIndex, to: end, parent: parent)
		renderedBlocks.append(contentsOf: batch)
		progressiveIndex = end
		if progressiveIndex >= progressiveNodes.count {
			cancelProgressiveRender(keepingRenderedBlocks: true)
			return false
		}
		return true
	}

	func cancelProgressiveRender() {
		cancelProgressiveRender(keepingRenderedBlocks: true)
	}

	private func cancelProgressiveRender(keepingRenderedBlocks: Bool) {
		progressiveNodes = []
		progressiveIndex = 0
		if !keepingRenderedBlocks {
			renderedBlocks = []
		}
	}

	func renderReusingStablePrefix(json: String, parent: UIStackView, stableBlockCount: Int) {
		_ = renderReusingStablePrefixProgressively(
			json: json,
			parent: parent,
			stableBlockCount: stableBlockCount,
			initialTailCount: Int.max
		)
	}

	func renderReusingStablePrefixProgressively(
		json: String,
		parent: UIStackView,
		stableBlockCount: Int,
		initialTailCount: Int
	) -> Bool {
		cancelProgressiveRender()
		guard let data = json.data(using: .utf8),
		      let root = try? JSONDecoder().decode(MarkdownNode.self, from: data) else {
			render(json: json, parent: parent)
			return false
		}
		let signatures = root.children.map(blockSignature)
		let limit = min(renderedBlocks.count, signatures.count, max(0, stableBlockCount))
		var blockPrefix = 0
		while blockPrefix < limit, renderedBlocks[blockPrefix].signature == signatures[blockPrefix] {
			blockPrefix += 1
		}
		let viewPrefix = renderedBlocks.prefix(blockPrefix).reduce(0) { $0 + $1.viewCount }
		removeTail(from: viewPrefix, parent: parent)
		let requestedEnd = initialTailCount >= root.children.count ? root.children.count :
			blockPrefix + max(0, initialTailCount)
		let tailEnd = min(root.children.count, requestedEnd)
		let tail = renderChildren(root.children, from: blockPrefix, to: tailEnd, parent: parent)
		renderedBlocks = Array(renderedBlocks.prefix(blockPrefix)) + tail
		guard tailEnd < root.children.count else { return false }
		progressiveNodes = root.children
		progressiveIndex = tailEnd
		return true
	}

	private func renderChildren(_ children: [MarkdownNode], from start: Int, parent: UIStackView) -> [RenderedBlock] {
		return renderChildren(children, from: start, to: children.count, parent: parent)
	}

	private func renderChildren(
		_ children: [MarkdownNode],
		from start: Int,
		to end: Int,
		parent: UIStackView
	) -> [RenderedBlock] {
		guard start < end, start < children.count else { return [] }
		return children[start..<min(end, children.count)].map { node in
			let before = parent.arrangedSubviews.count
			renderBlock(node, parent: parent)
			return RenderedBlock(signature: blockSignature(node), viewCount: parent.arrangedSubviews.count - before)
		}
	}

	private func removeTail(from prefix: Int, parent: UIStackView) {
		guard prefix < parent.arrangedSubviews.count else { return }
		for view in parent.arrangedSubviews[prefix...] {
			parent.removeArrangedSubview(view)
			view.removeFromSuperview()
		}
	}

	private func blockSignature(_ node: MarkdownNode) -> Int {
		var hasher = Hasher()
		hash(node, into: &hasher)
		return hasher.finalize()
	}

	private func hash(_ node: MarkdownNode, into hasher: inout Hasher) {
		hasher.combine(node.kind)
		hasher.combine(node.literal)
		hasher.combine(node.url)
		hasher.combine(node.title)
		hasher.combine(node.info)
		hasher.combine(node.sourcepos)
		hasher.combine(node.level)
		hasher.combine(node.listType)
		hasher.combine(node.start)
		hasher.combine(node.checked)
		hasher.combine(node.children.count)
		for child in node.children {
			hash(child, into: &hasher)
		}
	}

	func renderBlock(_ node: MarkdownNode, parent: UIStackView) {
		switch node.kind {
		case "heading":
			parent.addArrangedSubview(heading(node))
		case "paragraph":
			renderParagraph(node, parent: parent)
		case "list":
			renderList(node, parent: parent, depth: 0)
		case "table":
			parent.addArrangedSubview(table(node))
		case "block_quote":
			parent.addArrangedSubview(blockQuote(node))
		case "math_block":
			parent.addArrangedSubview(mathView(node.literal ?? "", display: true))
		case "image":
			parent.addArrangedSubview(imageView(node))
		case "code_block":
			parent.addArrangedSubview(codeBlock(node.literal ?? ""))
		case "thematic_break":
			parent.addArrangedSubview(rule())
		case "soft_break", "hard_break":
			parent.addArrangedSubview(spacer(height: 4))
		default:
			renderUnknown(node, parent: parent)
		}
	}

	private func renderParagraph(_ node: MarkdownNode, parent: UIStackView) {
		var segment: [MarkdownNode] = []
		for child in node.children {
			if child.kind == "math_block" {
				addInlineSegment(segment, to: parent)
				segment.removeAll(keepingCapacity: true)
				parent.addArrangedSubview(displayMathBlock(child.literal ?? ""))
			} else {
				segment.append(child)
			}
		}
		addInlineSegment(segment, to: parent)
	}

	private func addInlineSegment(_ children: [MarkdownNode], to parent: UIStackView) {
		if !children.isEmpty {
			parent.addArrangedSubview(inlineGroup(children))
		}
	}

	private func displayMathBlock(_ latex: String) -> UIView {
		let view = mathView(latex, display: true)
		return padded(view, top: 4, bottom: theme.paragraphBottomSpacing)
	}

	private func renderUnknown(_ node: MarkdownNode, parent: UIStackView) {
		if node.children.isEmpty {
			parent.addArrangedSubview(cellText(node.literal ?? "", role: .none))
		} else {
			node.children.forEach { renderBlock($0, parent: parent) }
		}
	}

	private func renderList(_ node: MarkdownNode, parent: UIStackView, depth: Int) {
		let ordered = node.listType == "ordered"
		var number = node.start ?? 1
		for item in node.children {
			if item.kind == "tasklist" {
				parent.addArrangedSubview(taskItem(item))
			} else {
				parent.addArrangedSubview(listItem(item, ordered: ordered, number: number, depth: depth))
				if ordered {
					number += 1
				}
			}
		}
	}

	private func listItem(_ item: MarkdownNode, ordered: Bool, number: Int, depth: Int) -> UIView {
		let row = horizontalRow()
		row.addArrangedSubview(ordered ? orderedMarker(number) : unorderedMarker(depth))
		row.addArrangedSubview(listItemContent(item, depth: depth))
		return row
	}

	private func listItemContent(_ item: MarkdownNode, depth: Int) -> UIStackView {
		let group = verticalStack()
		group.addArrangedSubview(inlineGroup(item.inlineChildren, compact: true))
		item.children.filter { $0.kind == "list" }.forEach {
			let nested = verticalStack()
			nested.layoutMargins = UIEdgeInsets(top: 0, left: theme.nestedListIndent, bottom: 0, right: 0)
			nested.isLayoutMarginsRelativeArrangement = true
			renderList($0, parent: nested, depth: depth + 1)
			group.addArrangedSubview(nested)
		}
		return group
	}

	private func horizontalRow() -> UIStackView {
		let row = UIStackView()
		row.axis = .horizontal
		row.alignment = .top
		row.spacing = 0
		row.layoutMargins = UIEdgeInsets(top: 0, left: theme.listIndent,
						 bottom: theme.listItemSpacing, right: 0)
		row.isLayoutMarginsRelativeArrangement = true
		return row
	}

	private func orderedMarker(_ number: Int) -> UILabel {
		let label = configuredLabel("\(number).", size: theme.bodyTextSize, theme: theme)
		label.textAlignment = .center
		label.widthAnchor.constraint(equalToConstant: theme.orderedMarkerWidth).isActive = true
		return label
	}

	private func unorderedMarker(_ depth: Int) -> UIView {
		let markers = theme.unorderedListMarkers
		let style = markers.isEmpty ? MorphListMarkerStyle.disc : markers[depth % markers.count]
		let marker = ListMarkerView(style: style, theme: theme)
		marker.widthAnchor.constraint(equalToConstant: theme.listMarkerWidth).isActive = true
		return marker
	}

	private func taskItem(_ item: MarkdownNode) -> UIView {
		let row = horizontalRow()
		let marker = TaskMarkerView(checked: item.checked ?? false, theme: theme)
		marker.widthAnchor.constraint(equalToConstant: theme.taskBoxSize).isActive = true
		marker.heightAnchor.constraint(equalToConstant: theme.taskBoxSize).isActive = true
		row.addArrangedSubview(marker)
		row.setCustomSpacing(theme.taskBoxTextGap, after: marker)
		row.addArrangedSubview(inlineGroup(item.inlineChildren, compact: true))
		return row
	}
}

private extension UIStackView {
	func removeAllArrangedSubviews() {
		arrangedSubviews.forEach {
			removeArrangedSubview($0)
			$0.removeFromSuperview()
		}
	}
}
#endif
