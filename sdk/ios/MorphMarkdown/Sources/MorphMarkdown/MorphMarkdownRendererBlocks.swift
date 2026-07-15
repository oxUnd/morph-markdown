#if canImport(UIKit)
import UIKit

extension MorphMarkdownRenderer {
	func heading(_ node: MarkdownNode) -> UIView {
		let level = min(max(node.level ?? 1, 1), 6)
		let size = theme.headingSize(level)
		let label = configuredLabel(node.plainText, size: size, theme: theme, bold: true, allowCjkSpacing: true)
		label.layoutMargins = UIEdgeInsets(top: theme.headingTopSpacing, left: 0, bottom: theme.headingBottomSpacing, right: 0)
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
		bar.backgroundColor = UIColor(argb: 0xff767676)
		bar.widthAnchor.constraint(equalToConstant: 4).isActive = true
		row.addArrangedSubview(bar)
		let label = configuredLabel(expandTabs(node.plainText.trimmingCharacters(in: .whitespacesAndNewlines), tabSize: theme.tabSize),
					    size: theme.bodyTextSize, theme: theme, allowCjkSpacing: true)
		row.setCustomSpacing(theme.blockquoteIndent, after: bar)
		row.addArrangedSubview(label)
		return row
	}

	func codeBlock(_ code: String) -> UIView {
		let label = InsetLabel()
		label.numberOfLines = 0
		label.font = UIFont.monospacedSystemFont(ofSize: theme.codeBlockTextSize, weight: .regular)
		label.textColor = UIColor(argb: 0xff1b1b1b)
		label.text = expandTabs(code, tabSize: theme.codeBlockTabSize)
		label.contentInsets = UIEdgeInsets(top: theme.codeBlockPaddingVertical,
						   left: theme.codeBlockPaddingHorizontal,
						   bottom: theme.codeBlockPaddingVertical,
						   right: theme.codeBlockPaddingHorizontal)
		label.backgroundColor = UIColor(argb: 0xffeeeeea)
		return label
	}

	func rule() -> UIView {
		let view = UIView()
		view.backgroundColor = UIColor(argb: 0xffb7b7b0)
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
