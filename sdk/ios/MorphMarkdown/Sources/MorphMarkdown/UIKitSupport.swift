#if canImport(UIKit)
import UIKit

extension UIColor {
	convenience init(argb: UInt32) {
		let alpha = CGFloat((argb >> 24) & 0xff) / 255
		let red = CGFloat((argb >> 16) & 0xff) / 255
		let green = CGFloat((argb >> 8) & 0xff) / 255
		let blue = CGFloat(argb & 0xff) / 255
		self.init(red: red, green: green, blue: blue, alpha: alpha)
	}
}

extension UIView {
	func removeAllSubviews() {
		subviews.forEach { $0.removeFromSuperview() }
	}

	func pinEdges(to view: UIView) {
		translatesAutoresizingMaskIntoConstraints = false
		NSLayoutConstraint.activate([
			leadingAnchor.constraint(equalTo: view.leadingAnchor),
			trailingAnchor.constraint(equalTo: view.trailingAnchor),
			topAnchor.constraint(equalTo: view.topAnchor),
			bottomAnchor.constraint(equalTo: view.bottomAnchor)
		])
	}
}

func morphFont(theme: MorphMarkdownTheme, size: CGFloat, bold: Bool = false) -> UIFont {
	if let name = bold ? theme.boldFontName ?? theme.fontName : theme.fontName,
	   let font = UIFont(name: name, size: size) {
		return font
	}
	switch theme.fontProfile {
	case .hetiLikeHei:
		return UIFont.systemFont(ofSize: size, weight: bold ? .bold : .regular)
	case .hetiLikeSong:
		return UIFont(name: "TimesNewRomanPSMT", size: size) ?? UIFont.systemFont(ofSize: size)
	case .system:
		return UIFont.systemFont(ofSize: size, weight: bold ? .bold : .regular)
	}
}

func configuredLabel(
	_ text: String,
	size: CGFloat,
	theme: MorphMarkdownTheme,
	bold: Bool = false,
	monospace: Bool = false,
	allowCjkSpacing: Bool = false
) -> UILabel {
	let label = UILabel()
	label.numberOfLines = 0
	label.textColor = UIColor(argb: 0xff1b1b1b)
	label.font = monospace ? UIFont.monospacedSystemFont(ofSize: size, weight: .regular) : morphFont(theme: theme, size: size, bold: bold)
	label.attributedText = attributedText(processedText(text, theme: theme, allowCjkSpacing: allowCjkSpacing), label: label, theme: theme)
	return label
}

func attributedText(_ text: String, label: UILabel, theme: MorphMarkdownTheme) -> NSAttributedString {
	let style = NSMutableParagraphStyle()
	style.lineHeightMultiple = theme.bodyLineHeightMultiplier
	return NSAttributedString(string: text, attributes: [
		.font: label.font as Any,
		.foregroundColor: label.textColor as Any,
		.paragraphStyle: style
	])
}

final class InsetLabel: UILabel {
	var contentInsets: UIEdgeInsets = .zero

	override var intrinsicContentSize: CGSize {
		let size = super.intrinsicContentSize
		return CGSize(width: size.width + contentInsets.left + contentInsets.right,
			      height: size.height + contentInsets.top + contentInsets.bottom)
	}

	override func drawText(in rect: CGRect) {
		super.drawText(in: rect.inset(by: contentInsets))
	}

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		let inner = CGSize(width: max(0, size.width - contentInsets.left - contentInsets.right),
				   height: max(0, size.height - contentInsets.top - contentInsets.bottom))
		let fitted = multilineSizeThatFits(inner) ?? super.sizeThatFits(inner)
		return CGSize(width: fitted.width + contentInsets.left + contentInsets.right,
			      height: fitted.height + contentInsets.top + contentInsets.bottom)
	}

	private func multilineSizeThatFits(_ size: CGSize) -> CGSize? {
		guard numberOfLines != 1, size.width > 0, size.width < CGFloat.greatestFiniteMagnitude else {
			return nil
		}
		let text = measuredAttributedText()
		let rect = text.boundingRect(with: size,
					     options: [.usesLineFragmentOrigin, .usesFontLeading],
					     context: nil)
		return CGSize(width: ceil(rect.width), height: ceil(rect.height))
	}

	private func measuredAttributedText() -> NSAttributedString {
		let mutable = NSMutableAttributedString(attributedString: attributedText ?? NSAttributedString(string: text ?? ""))
		guard mutable.length > 0 else {
			return mutable
		}
		let range = NSRange(location: 0, length: mutable.length)
		let paragraph = NSMutableParagraphStyle()
		paragraph.lineBreakMode = lineBreakMode
		mutable.addAttributes([.font: font as Any, .paragraphStyle: paragraph], range: range)
		return mutable
	}
}
#endif
