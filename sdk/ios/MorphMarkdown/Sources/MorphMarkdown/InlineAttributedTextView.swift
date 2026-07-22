#if canImport(UIKit)
import UIKit

final class InlineAttributedTextView: UITextView, UITextViewDelegate, TableIntrinsicOverride {
	var onLinkClick: MorphMarkdownLinkHandler?
	var linkTitles: [String: String] = [:]

	init(contentInsets: UIEdgeInsets) {
		super.init(frame: .zero, textContainer: nil)
		backgroundColor = .clear
		isEditable = false
		isSelectable = true
		isScrollEnabled = false
		textContainerInset = contentInsets
		textContainer.lineFragmentPadding = 0
		adjustsFontForContentSizeCategory = false
		delegate = self
		setContentHuggingPriority(.required, for: .vertical)
		setContentCompressionResistancePriority(.required, for: .vertical)
	}

	required init?(coder: NSCoder) {
		return nil
	}

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		let width = size.width > 0 ? size.width : CGFloat.greatestFiniteMagnitude
		let fit = super.sizeThatFits(CGSize(width: width, height: CGFloat.greatestFiniteMagnitude))
		return CGSize(width: ceil(fit.width), height: ceil(fit.height))
	}

	override var intrinsicContentSize: CGSize {
		let width = bounds.width > 0 ? bounds.width : UIView.noIntrinsicMetric
		return sizeThatFits(CGSize(width: width, height: UIView.noIntrinsicMetric))
	}

	var tableMinimumWidth: CGFloat {
		guard let attributedText, attributedText.length > 0 else {
			return textContainerInset.left + textContainerInset.right
		}
		var widest: CGFloat = 0
		let source = attributedText.string as NSString
		var location = 0
		for fragment in InlineTextFragmenter.fragments(attributedText.string) {
			let length = (fragment as NSString).length
			let range = NSRange(location: location, length: min(length, source.length - location))
			widest = max(widest, measuredWidth(attributedText.attributedSubstring(from: range)))
			location += length
		}
		attributedText.enumerateAttribute(
			.backgroundColor,
			in: NSRange(location: 0, length: attributedText.length)
		) { value, range, _ in
			if value != nil {
				widest = max(widest, measuredWidth(attributedText.attributedSubstring(from: range)))
			}
		}
		return ceil(widest + textContainerInset.left + textContainerInset.right)
	}

	private func measuredWidth(_ value: NSAttributedString) -> CGFloat {
		return ceil(value.boundingRect(
			with: CGSize(width: CGFloat.greatestFiniteMagnitude, height: CGFloat.greatestFiniteMagnitude),
			options: [.usesLineFragmentOrigin, .usesFontLeading],
			context: nil
		).width)
	}

	func textView(
		_ textView: UITextView,
		shouldInteractWith URL: URL,
		in characterRange: NSRange,
		interaction: UITextItemInteraction
	) -> Bool {
		onLinkClick?(URL.absoluteString, linkTitles[URL.absoluteString])
		return false
	}
}
#endif
