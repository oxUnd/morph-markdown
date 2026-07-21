#if canImport(UIKit)
import UIKit

private struct LinkTarget {
	let url: String
	let title: String?
}

extension MorphMarkdownRenderer {
	func inlineGroup(_ children: [MarkdownNode], compact: Bool = false) -> UIView {
		let row = InlineLayoutView()
		row.contentInsets = compact ? .zero : UIEdgeInsets(top: theme.paragraphTopSpacing, left: 0,
								   bottom: theme.paragraphBottomSpacing, right: 0)
		row.setContentHuggingPriority(.required, for: .vertical)
		row.setContentCompressionResistancePriority(.required, for: .vertical)
		populateInline(row, children: children, role: .none)
		return row
	}

	func populateInline(_ row: UIView, children: [MarkdownNode], role: TableCellRole) {
		populateInline(row, children: children, role: role, link: nil)
	}

	private func populateInline(_ row: UIView, children: [MarkdownNode], role: TableCellRole, link: LinkTarget?) {
		children.forEach { addInline(row, child: $0, role: role, link: link) }
	}

	func addInline(_ row: UIView, child: MarkdownNode, role: TableCellRole) {
		addInline(row, child: child, role: role, link: nil)
	}

	private func addInline(_ row: UIView, child: MarkdownNode, role: TableCellRole, link: LinkTarget?) {
		switch child.kind {
		case "text":
			addInlineText(row, value: child.literal ?? "", role: role, link: link)
		case "code":
			row.addSubview(linkedView(inlineCode(child.literal ?? "", role: role), link: link))
		case "soft_break", "hard_break":
			addInlineText(row, value: "\n", role: role, link: link)
		case "math_inline":
			row.addSubview(linkedView(mathView(child.literal ?? "", display: false, role: role), link: link))
		case "math_block":
			row.addSubview(linkedView(mathView(child.literal ?? "", display: true, role: role), link: link))
		case "image":
			row.addSubview(linkedView(imageView(child), link: link))
		case "link":
			addLinkInline(row, child: child, role: role)
		default:
			addInlineText(row, value: child.plainText, role: role, link: link)
		}
	}

	func addInlineText(_ row: UIView, value: String, role: TableCellRole) {
		addInlineText(row, value: value, role: role, link: nil)
	}

	private func addLinkInline(_ row: UIView, child: MarkdownNode, role: TableCellRole) {
		guard let url = child.url, !url.isEmpty else {
			addInlineText(row, value: child.plainText, role: role, link: nil)
			return
		}
		let link = LinkTarget(url: url, title: child.title)
		if child.children.isEmpty {
			addInlineText(row, value: url, role: role, link: link)
			return
		}
		populateInline(row, children: child.children, role: role, link: link)
	}

	private func addInlineText(_ row: UIView, value: String, role: TableCellRole, link: LinkTarget?) {
		let expanded = expandTabs(value, tabSize: theme.tabSize)
		let spaced = processedText(expanded, theme: theme, allowCjkSpacing: true)
		for fragment in inlineTextFragments(spaced, role: role) {
			row.addSubview(linkedText(fragment, role: role, link: link))
		}
	}

	private func linkedText(_ value: String, role: TableCellRole, link: LinkTarget?) -> UIView {
		guard let link else {
			return cellText(value, role: role, expand: false, allowCjkSpacing: false)
		}
		let label = linkLabel(value, role: role, link: link)
		return label
	}

	private func linkedView(_ view: UIView, link: LinkTarget?) -> UIView {
		guard let link else {
			return view
		}
		view.isUserInteractionEnabled = true
		view.addGestureRecognizer(LinkTapGestureRecognizer(link: link, onLinkClick: onLinkClick))
		return view
	}

	func mathView(_ latex: String, display: Bool, role: TableCellRole = .none) -> UIView {
		var renderTheme = theme
		if role != .none {
			renderTheme.mathTextScale = theme.tableMathTextScale
		}
		return mathRenderer?.render(latex: latex, display: display, theme: renderTheme) ??
			configuredLabel(latex, size: theme.bodyTextSize, theme: theme)
	}

	func imageView(_ node: MarkdownNode) -> UIView {
		let url = node.url ?? node.literal ?? ""
		if let loaded = imageLoader.load(url: url, theme: theme) {
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
		label.backgroundColor = UIColor(argb: 0xffeeeeea)
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
		row.backgroundColor = UIColor(argb: 0xffeeeeea)
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

	private func linkLabel(_ value: String, role: TableCellRole, link: LinkTarget) -> LinkLabel {
		let label = LinkLabel()
		label.numberOfLines = role != .none && theme.tableCellWrap ? 0 : 1
		label.lineBreakMode = role != .none && theme.tableCellWrap ? .byCharWrapping : .byClipping
		label.font = tableFont(role)
		label.textColor = UIColor(argb: theme.linkTextColor)
		label.linkColor = UIColor(argb: theme.linkTextColor)
		label.drawLinkUnderline = theme.linkUnderline
		label.url = link.url
		label.title = link.title
		label.onLinkClick = onLinkClick
		label.attributedText = linkAttributedText(value, label: label, role: role)
		return label
	}

	private func linkAttributedText(_ text: String, label: UILabel, role: TableCellRole) -> NSAttributedString {
		let style = NSMutableParagraphStyle()
		style.lineHeightMultiple = role == .none ? theme.bodyLineHeightMultiplier : theme.tableCellLineHeightMultiplier
		var attributes: [NSAttributedString.Key: Any] = [
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
