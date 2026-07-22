#if canImport(UIKit)
import UIKit

private struct LinkTarget {
	let url: String
	let title: String?
}

private struct InlineTextStyle {
	var bold = false
	var italic = false
	var code = false
	var link: LinkTarget?
}

extension MorphMarkdownRenderer {
	func inlineGroup(_ children: [MarkdownNode], compact: Bool = false) -> UIView {
		if let text = attributedInlineGroup(children, compact: compact, role: .none) {
			return text
		}
		let row = InlineLayoutView()
		row.contentInsets = compact ? .zero : UIEdgeInsets(top: theme.paragraphTopSpacing, left: 0,
								   bottom: theme.paragraphBottomSpacing, right: 0)
		row.setContentHuggingPriority(.required, for: .vertical)
		row.setContentCompressionResistancePriority(.required, for: .vertical)
		populateInline(row, children: children, role: .none)
		return row
	}

	func attributedInlineGroup(
		_ children: [MarkdownNode],
		compact: Bool,
		role: TableCellRole
	) -> UIView? {
		let output = NSMutableAttributedString()
		var titles: [String: String] = [:]
		var baseStyle = InlineTextStyle()
		baseStyle.bold = role == .header && theme.tableStyle.headerBold
		guard appendAttributed(children, to: output, style: baseStyle, role: role, titles: &titles) else {
			return nil
		}
		let insets = attributedInsets(compact: compact, role: role)
		let view = InlineAttributedTextView(contentInsets: insets)
		view.attributedText = output
		view.linkTextAttributes = [
			.foregroundColor: UIColor(argb: theme.linkTextColor),
			.underlineStyle: theme.linkUnderline ? NSUnderlineStyle.single.rawValue : 0
		]
		view.linkTitles = titles
		view.onLinkClick = { [weak self] url, title in self?.onLinkClick?(url, title) }
		return view
	}

	private func attributedInsets(compact: Bool, role: TableCellRole) -> UIEdgeInsets {
		if role != .none {
			return UIEdgeInsets(top: theme.tableCellPaddingVertical,
						 left: theme.tableCellPaddingHorizontal,
						 bottom: theme.tableCellPaddingVertical,
						 right: theme.tableCellPaddingHorizontal)
		}
		return compact ? .zero : UIEdgeInsets(top: theme.paragraphTopSpacing, left: 0,
									 bottom: theme.paragraphBottomSpacing, right: 0)
	}

	private func appendAttributed(
		_ nodes: [MarkdownNode],
		to output: NSMutableAttributedString,
		style: InlineTextStyle,
		role: TableCellRole,
		titles: inout [String: String]
	) -> Bool {
		for node in nodes {
			var next = style
			switch node.kind {
			case "text":
				appendAttributedText(node.literal ?? "", to: output, style: next, role: role)
			case "code":
				next.code = true
				appendAttributedText(node.literal ?? "", to: output, style: next, role: role)
			case "soft_break", "hard_break":
				appendAttributedText("\n", to: output, style: next, role: role)
			case "strong":
				next.bold = true
				guard appendAttributed(node.children, to: output, style: next, role: role, titles: &titles) else { return false }
			case "emph":
				next.italic = true
				guard appendAttributed(node.children, to: output, style: next, role: role, titles: &titles) else { return false }
			case "link":
				guard let url = node.url, !url.isEmpty else {
					appendAttributedText(node.plainText, to: output, style: next, role: role)
					continue
				}
				next.link = LinkTarget(url: url, title: node.title)
				titles[url] = node.title
				if node.children.isEmpty {
					appendAttributedText(url, to: output, style: next, role: role)
				} else if !appendAttributed(node.children, to: output, style: next, role: role, titles: &titles) {
					return false
				}
			default:
				return false
			}
		}
		return true
	}

	private func appendAttributedText(
		_ value: String,
		to output: NSMutableAttributedString,
		style: InlineTextStyle,
		role: TableCellRole
	) {
		let expanded = expandTabs(value, tabSize: theme.tabSize)
		let text = processedText(expanded, theme: theme, allowCjkSpacing: !style.code)
		let paragraph = NSMutableParagraphStyle()
		paragraph.lineHeightMultiple = role == .none ? theme.bodyLineHeightMultiplier : theme.tableCellLineHeightMultiplier
		var attributes: [NSAttributedString.Key: Any] = [
			.font: attributedFont(style, role: role),
			.foregroundColor: style.link == nil ? tableTextColor(role) : UIColor(argb: theme.linkTextColor),
			.paragraphStyle: paragraph
		]
		if style.code {
			attributes[.backgroundColor] = UIColor(argb: theme.inlineCodeBackgroundColor)
		}
		if let link = style.link, let url = URL(string: link.url) {
			attributes[.link] = url
			attributes[.underlineStyle] = theme.linkUnderline ? NSUnderlineStyle.single.rawValue : 0
		}
		output.append(NSAttributedString(string: text, attributes: attributes))
	}

	private func attributedFont(_ style: InlineTextStyle, role: TableCellRole) -> UIFont {
		if style.code {
			return UIFont.monospacedSystemFont(ofSize: theme.inlineCodeTextSize, weight: style.bold ? .bold : .regular)
		}
		let size = tableTextSize(role)
		let base = morphFont(theme: theme, size: size, bold: style.bold)
		let traits = base.fontDescriptor.symbolicTraits.union(.traitItalic)
		guard style.italic, let descriptor = base.fontDescriptor.withSymbolicTraits(traits) else {
			return base
		}
		return UIFont(descriptor: descriptor, size: size)
	}

	func populateInline(_ row: UIView, children: [MarkdownNode], role: TableCellRole) {
		var style = InlineTextStyle()
		style.bold = role == .header && theme.tableStyle.headerBold
		populateInline(row, children: children, role: role, link: nil, style: style)
	}

	private func populateInline(
		_ row: UIView,
		children: [MarkdownNode],
		role: TableCellRole,
		link: LinkTarget?,
		style: InlineTextStyle
	) {
		children.forEach { addInline(row, child: $0, role: role, link: link, style: style) }
	}

	func addInline(_ row: UIView, child: MarkdownNode, role: TableCellRole) {
		var style = InlineTextStyle()
		style.bold = role == .header && theme.tableStyle.headerBold
		addInline(row, child: child, role: role, link: nil, style: style)
	}

	private func addInline(
		_ row: UIView,
		child: MarkdownNode,
		role: TableCellRole,
		link: LinkTarget?,
		style: InlineTextStyle
	) {
		switch child.kind {
		case "text":
			addInlineText(row, value: child.literal ?? "", role: role, link: link, style: style)
		case "code":
			row.addSubview(linkedView(inlineCode(child.literal ?? "", role: role), link: link))
		case "soft_break", "hard_break":
			row.addSubview(InlineLineBreakView())
		case "math_inline":
			row.addSubview(linkedView(mathView(child.literal ?? "", display: false, role: role), link: link))
		case "math_block":
			row.addSubview(linkedView(mathView(child.literal ?? "", display: true, role: role), link: link))
		case "image":
			row.addSubview(linkedView(imageView(child), link: link))
		case "link":
			addLinkInline(row, child: child, role: role, style: style)
		case "strong":
			var next = style
			next.bold = true
			populateInline(row, children: child.children, role: role, link: link, style: next)
		case "emph":
			var next = style
			next.italic = true
			populateInline(row, children: child.children, role: role, link: link, style: next)
		default:
			addInlineText(row, value: child.plainText, role: role, link: link, style: style)
		}
	}

	func addInlineText(_ row: UIView, value: String, role: TableCellRole) {
		addInlineText(row, value: value, role: role, link: nil, style: InlineTextStyle())
	}

	private func addLinkInline(_ row: UIView, child: MarkdownNode, role: TableCellRole, style: InlineTextStyle) {
		guard let url = child.url, !url.isEmpty else {
			addInlineText(row, value: child.plainText, role: role, link: nil, style: style)
			return
		}
		let link = LinkTarget(url: url, title: child.title)
		if child.children.isEmpty {
			addInlineText(row, value: url, role: role, link: link, style: style)
			return
		}
		populateInline(row, children: child.children, role: role, link: link, style: style)
	}

	private func addInlineText(
		_ row: UIView,
		value: String,
		role: TableCellRole,
		link: LinkTarget?,
		style: InlineTextStyle
	) {
		let expanded = expandTabs(value, tabSize: theme.tabSize)
		let spaced = processedText(expanded, theme: theme, allowCjkSpacing: true)
		for fragment in inlineTextFragments(spaced, role: role) {
			row.addSubview(linkedText(fragment, role: role, link: link, style: style))
		}
	}

	private func linkedText(_ value: String, role: TableCellRole, link: LinkTarget?, style: InlineTextStyle) -> UIView {
		guard let link else {
			let label = cellText(value, role: role, expand: false, allowCjkSpacing: false)
			label.font = attributedFont(style, role: role)
			return label
		}
		let label = linkLabel(value, role: role, link: link, style: style)
		return label
	}

	private func linkedView(_ view: UIView, link: LinkTarget?) -> UIView {
		guard let link else {
			return view
		}
		view.isUserInteractionEnabled = true
		view.addGestureRecognizer(LinkTapGestureRecognizer(link: link) { [weak self] url, title in
			self?.onLinkClick?(url, title)
		})
		return view
	}

	func mathView(_ latex: String, display: Bool, role: TableCellRole = .none) -> UIView {
		var renderTheme = theme
		if role != .none {
			renderTheme.mathTextScale = theme.tableMathTextScale
			renderTheme.bodyTextColor = tableTextColor(role).argb
		}
		let view = mathRenderer?.render(latex: latex, display: display, theme: renderTheme) ??
			configuredLabel(latex, size: theme.bodyTextSize, theme: theme)
		view.isAccessibilityElement = true
		view.accessibilityLabel = latex
		return view
	}

	func imageView(_ node: MarkdownNode) -> UIView {
		let url = node.url ?? node.literal ?? ""
		if let loaded = imageLoader.load(url: url, theme: theme) {
			loaded.isAccessibilityElement = true
			loaded.accessibilityLabel = node.plainText.isEmpty ? "image" : node.plainText
			return loaded
		}
		let label = InsetLabel()
		label.numberOfLines = 0
		label.lineBreakMode = .byCharWrapping
		label.font = UIFont.monospacedSystemFont(ofSize: theme.inlineCodeTextSize, weight: .regular)
		label.text = "invalid image: \(url)"
		label.textColor = UIColor(argb: theme.bodyTextColor)
		label.contentInsets = UIEdgeInsets(top: theme.codeBlockPaddingVertical,
						   left: theme.codeBlockPaddingHorizontal,
						   bottom: theme.codeBlockPaddingVertical,
						   right: theme.codeBlockPaddingHorizontal)
		label.layer.borderColor = UIColor(argb: 0xff454545).cgColor
		label.layer.borderWidth = 1
		label.backgroundColor = UIColor(argb: theme.imageErrorBackgroundColor)
		label.layer.borderColor = UIColor(argb: theme.imageErrorBorderColor).cgColor
		return label
	}

	func inlineCode(_ code: String, role: TableCellRole) -> UIView {
		if role != .none, theme.tableCellWrap {
			return wrappingInlineCode(code, role: role)
		}
		let label = cellText(expandTabs(code, tabSize: theme.tabSize), role: role, expand: false)
		label.font = UIFont.monospacedSystemFont(ofSize: theme.inlineCodeTextSize, weight: .regular)
		label.contentInsets = UIEdgeInsets(top: theme.inlineCodePaddingVertical,
						   left: theme.inlineCodePaddingHorizontal,
						   bottom: theme.inlineCodePaddingVertical,
						   right: theme.inlineCodePaddingHorizontal)
		label.backgroundColor = UIColor(argb: theme.inlineCodeBackgroundColor)
		return label
	}

	private func inlineTextFragments(_ text: String, role: TableCellRole) -> [String] {
		if role != .none, theme.tableCellWrap {
			return text.map { String($0) }
		}
		return InlineTextFragmenter.fragments(text)
	}

	private func wrappingInlineCode(_ code: String, role: TableCellRole) -> UIView {
		let row = InlineLayoutView()
		row.contentInsets = UIEdgeInsets(top: theme.inlineCodePaddingVertical,
						 left: theme.inlineCodePaddingHorizontal,
						 bottom: theme.inlineCodePaddingVertical,
						 right: theme.inlineCodePaddingHorizontal)
		row.backgroundColor = UIColor(argb: theme.inlineCodeBackgroundColor)
		for fragment in expandTabs(code, tabSize: theme.tabSize).map({ String($0) }) {
			let label = cellText(fragment, role: role, expand: false, allowCjkSpacing: false)
			label.font = UIFont.monospacedSystemFont(ofSize: theme.inlineCodeTextSize, weight: .regular)
			row.addSubview(label)
		}
		return row
	}

	func cellText(_ value: String, role: TableCellRole, expand: Bool = true, allowCjkSpacing: Bool = true) -> InsetLabel {
		let source = expand ? expandTabs(value, tabSize: theme.tabSize) : value
		let label = InsetLabel()
		label.numberOfLines = role != .none && theme.tableCellWrap ? 0 : 1
		label.lineBreakMode = role != .none && theme.tableCellWrap ? .byCharWrapping : .byClipping
		label.font = tableFont(role)
		label.textColor = tableTextColor(role)
		label.text = processedText(source, theme: theme, allowCjkSpacing: allowCjkSpacing)
		return label
	}

	private func linkLabel(
		_ value: String,
		role: TableCellRole,
		link: LinkTarget,
		style: InlineTextStyle
	) -> LinkLabel {
		let label = LinkLabel()
		label.numberOfLines = role != .none && theme.tableCellWrap ? 0 : 1
		label.lineBreakMode = role != .none && theme.tableCellWrap ? .byCharWrapping : .byClipping
		label.font = attributedFont(style, role: role)
		label.textColor = UIColor(argb: theme.linkTextColor)
		label.linkColor = UIColor(argb: theme.linkTextColor)
		label.drawLinkUnderline = theme.linkUnderline
		label.url = link.url
		label.title = link.title
		label.onLinkClick = { [weak self] url, title in self?.onLinkClick?(url, title) }
		label.attributedText = linkAttributedText(value, label: label, role: role)
		return label
	}

	private func linkAttributedText(_ text: String, label: UILabel, role: TableCellRole) -> NSAttributedString {
		let style = NSMutableParagraphStyle()
		style.lineHeightMultiple = role == .none ? theme.bodyLineHeightMultiplier : theme.tableCellLineHeightMultiplier
		let attributes: [NSAttributedString.Key: Any] = [
			.font: label.font as Any,
			.foregroundColor: UIColor(argb: theme.linkTextColor),
			.paragraphStyle: style
		]
		return NSAttributedString(string: text, attributes: attributes)
	}

	private func tableFont(_ role: TableCellRole) -> UIFont {
		let size = tableTextSize(role)
		let bold = role == .header && theme.tableStyle.headerBold
		return morphFont(theme: theme, size: size, bold: bold)
	}

	private func tableTextColor(_ role: TableCellRole) -> UIColor {
		switch role {
		case .header:
			return UIColor(argb: theme.tableStyle.headerTextColor)
		case .body:
			return UIColor(argb: theme.tableStyle.bodyTextColor)
		case .none:
			return UIColor(argb: theme.bodyTextColor)
		}
	}

	private func tableTextSize(_ role: TableCellRole) -> CGFloat {
		switch role {
		case .header:
			return theme.tableStyle.headerTextSize ?? theme.bodyTextSize
		case .body:
			return theme.tableStyle.bodyTextSize ?? theme.bodyTextSize
		case .none:
			return theme.bodyTextSize
		}
	}
}

private final class LinkTapGestureRecognizer: UITapGestureRecognizer {
	private let link: LinkTarget
	private let onLinkClick: MorphMarkdownLinkHandler?

	init(link: LinkTarget, onLinkClick: MorphMarkdownLinkHandler?) {
		self.link = link
		self.onLinkClick = onLinkClick
		super.init(target: nil, action: nil)
		addTarget(self, action: #selector(didTap))
	}

	@objc private func didTap() {
		onLinkClick?(link.url, link.title)
	}
}
#endif
