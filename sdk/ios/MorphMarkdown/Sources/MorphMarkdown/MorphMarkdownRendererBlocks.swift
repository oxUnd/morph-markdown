#if canImport(UIKit)
import UIKit

extension MorphMarkdownRenderer {
	func heading(_ node: MarkdownNode) -> UIView {
		let level = min(max(node.level ?? 1, 1), 6)
		let size = theme.headingSize(level)
		let label = InsetLabel()
		label.numberOfLines = 0
		label.font = morphFont(theme: theme, size: size, bold: true)
		label.textColor = UIColor(argb: theme.bodyTextColor)
		let style = NSMutableParagraphStyle()
		style.minimumLineHeight = theme.headingLineHeight(level)
		style.maximumLineHeight = theme.headingLineHeight(level)
		label.attributedText = NSAttributedString(
			string: processedText(node.plainText, theme: theme, allowCjkSpacing: true),
			attributes: [.font: label.font as Any, .foregroundColor: label.textColor as Any, .paragraphStyle: style]
		)
		label.contentInsets = UIEdgeInsets(top: theme.headingTopSpacing, left: 0,
						 bottom: theme.headingBottomSpacing, right: 0)
		label.accessibilityTraits.insert(.header)
		return label
	}

	func blockQuote(_ node: MarkdownNode) -> UIView {
		let row = UIStackView()
		row.axis = .horizontal
		row.alignment = .fill
		row.layoutMargins = UIEdgeInsets(top: theme.blockquoteVerticalPadding, left: 0,
						 bottom: theme.blockquoteBottomSpacing, right: 0)
		row.isLayoutMarginsRelativeArrangement = true
		let bar = UIView()
		bar.backgroundColor = UIColor(argb: theme.blockquoteBarColor)
		bar.widthAnchor.constraint(equalToConstant: 4).isActive = true
		row.addArrangedSubview(bar)
		row.setCustomSpacing(theme.blockquoteIndent, after: bar)
		row.addArrangedSubview(blockQuoteContent(node))
		return row
	}

	private func blockQuoteContent(_ node: MarkdownNode) -> UIView {
		let content = verticalStack()
		if node.children.isEmpty {
			let text = expandTabs(
				node.literal?.trimmingCharacters(in: .whitespacesAndNewlines) ?? "",
				tabSize: theme.tabSize
			)
			content.addArrangedSubview(configuredLabel(text, size: theme.bodyTextSize, theme: theme, allowCjkSpacing: true))
			return content
		}
		for child in node.children {
			if child.kind == "paragraph" {
				content.addArrangedSubview(inlineGroup(child.children, compact: true))
			} else {
				renderBlock(child, parent: content)
			}
		}
		return content
	}

	func codeBlock(_ code: String) -> UIView {
		let label = InsetLabel()
		label.numberOfLines = 0
		label.font = UIFont.monospacedSystemFont(ofSize: theme.codeBlockTextSize, weight: .regular)
		label.textColor = UIColor(argb: theme.bodyTextColor)
		label.text = expandTabs(code, tabSize: theme.codeBlockTabSize)
		label.contentInsets = UIEdgeInsets(top: theme.codeBlockPaddingVertical,
						   left: theme.codeBlockPaddingHorizontal,
						   bottom: theme.codeBlockPaddingVertical,
						   right: theme.codeBlockPaddingHorizontal)
		label.backgroundColor = UIColor(argb: theme.codeBlockBackgroundColor)
		return label
	}

	func rule() -> UIView {
		let view = UIView()
		view.backgroundColor = UIColor(argb: theme.horizontalRuleColor)
		view.heightAnchor.constraint(equalToConstant: 1).isActive = true
		return padded(view, top: 14, bottom: 14)
	}

	func spacer(height: CGFloat) -> UIView {
		let view = UIView()
		view.heightAnchor.constraint(equalToConstant: height).isActive = true
		return view
	}

	func padded(_ view: UIView, top: CGFloat, bottom: CGFloat) -> UIView {
		let stack = verticalStack()
		stack.layoutMargins = UIEdgeInsets(top: top, left: 0, bottom: bottom, right: 0)
		stack.isLayoutMarginsRelativeArrangement = true
		stack.addArrangedSubview(view)
		return stack
	}

	func verticalStack() -> UIStackView {
		let stack = UIStackView()
		stack.axis = .vertical
		stack.alignment = .fill
		stack.distribution = .fill
		return stack
	}
}
#endif
