#if canImport(UIKit)
import UIKit

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
		children.forEach { addInline(row, child: $0, role: role) }
	}

	func addInline(_ row: UIView, child: MarkdownNode, role: TableCellRole) {
		switch child.kind {
		case "text":
			addInlineText(row, value: child.literal ?? "", role: role)
		case "code":
			row.addSubview(inlineCode(child.literal ?? "", role: role))
		case "soft_break", "hard_break":
			addInlineText(row, value: "\n", role: role)
		case "math_inline":
			row.addSubview(mathView(child.literal ?? "", display: false, role: role))
		case "math_block":
			row.addSubview(mathView(child.literal ?? "", display: true, role: role))
		case "image":
			row.addSubview(imageView(child))
		default:
			addInlineText(row, value: child.plainText, role: role)
		}
	}

	func addInlineText(_ row: UIView, value: String, role: TableCellRole) {
		let expanded = expandTabs(value, tabSize: theme.tabSize)
		let spaced = processedText(expanded, theme: theme, allowCjkSpacing: true)
		for fragment in InlineTextFragmenter.fragments(spaced) {
			row.addSubview(cellText(fragment, role: role, expand: false, allowCjkSpacing: false))
		}
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
		label.textColor = UIColor(argb: 0xff1b1b1b)
		label.contentInsets = UIEdgeInsets(top: theme.codeBlockPaddingVertical,
						   left: theme.codeBlockPaddingHorizontal,
						   bottom: theme.codeBlockPaddingVertical,
						   right: theme.codeBlockPaddingHorizontal)
		label.layer.borderColor = UIColor(argb: 0xff454545).cgColor
		label.layer.borderWidth = 1
		return label
	}

	func inlineCode(_ code: String, role: TableCellRole) -> UIView {
		let label = cellText(expandTabs(code, tabSize: theme.tabSize), role: role, expand: false)
		label.font = UIFont.monospacedSystemFont(ofSize: theme.inlineCodeTextSize, weight: .regular)
		label.contentInsets = UIEdgeInsets(top: theme.inlineCodePaddingVertical,
						   left: theme.inlineCodePaddingHorizontal,
						   bottom: theme.inlineCodePaddingVertical,
						   right: theme.inlineCodePaddingHorizontal)
		label.backgroundColor = UIColor(argb: 0xffeeeeea)
		return label
	}

	func cellText(_ value: String, role: TableCellRole, expand: Bool = true, allowCjkSpacing: Bool = true) -> InsetLabel {
		let source = expand ? expandTabs(value, tabSize: theme.tabSize) : value
		let label = InsetLabel()
		label.numberOfLines = 1
		label.font = tableFont(role)
		label.textColor = tableTextColor(role)
		label.text = processedText(source, theme: theme, allowCjkSpacing: allowCjkSpacing)
		return label
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
			return UIColor(argb: 0xff1b1b1b)
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
#endif
