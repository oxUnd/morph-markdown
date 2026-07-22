#if canImport(UIKit)
import UIKit

final class InlineLayoutView: UIView {
	var lineSpacing: CGFloat = 0
	var minLineHeight: CGFloat = 0
	var contentInsets: UIEdgeInsets = .zero
	private var measuredLines: [InlineLine] = []
	private var measuredSizes: [InlineItemSize] = []
	private var lastMeasuredWidth: CGFloat = UIView.noIntrinsicMetric

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		let widthLimit = contentWidthLimit(size.width)
		lastMeasuredWidth = size.width
		measureChildren(maxWidth: widthLimit)
		measuredLines = InlineLineBreaker.breakLines(
			items: measuredSizes,
			maxWidth: widthLimit,
			minLineHeight: minLineHeight
		)
		return CGSize(width: contentInsets.left + contentWidth() + contentInsets.right,
			      height: contentInsets.top + contentHeight() + contentInsets.bottom)
	}

	override var intrinsicContentSize: CGSize {
		let width = effectiveIntrinsicWidth()
		return sizeThatFits(CGSize(width: width, height: UIView.noIntrinsicMetric))
	}

	override func systemLayoutSizeFitting(
		_ targetSize: CGSize,
		withHorizontalFittingPriority horizontalFittingPriority: UILayoutPriority,
		verticalFittingPriority: UILayoutPriority
	) -> CGSize {
		let width = fittingWidth(targetSize.width, priority: horizontalFittingPriority)
		return sizeThatFits(CGSize(width: width, height: targetSize.height))
	}

	override func layoutSubviews() {
		invalidateIfWidthChanged()
		super.layoutSubviews()
		_ = sizeThatFits(bounds.size)
		var y = contentInsets.top
		for line in measuredLines {
			layout(line: line, top: y)
			y += line.height + lineSpacing
		}
	}

	private func invalidateIfWidthChanged() {
		guard bounds.width > 0, bounds.width != lastMeasuredWidth else {
			return
		}
		invalidateIntrinsicContentSize()
	}

	private func effectiveIntrinsicWidth() -> CGFloat {
		if bounds.width > 0 {
			return bounds.width
		}
		return UIView.noIntrinsicMetric
	}

	private func fittingWidth(_ width: CGFloat, priority: UILayoutPriority) -> CGFloat {
		if priority == .required, width > 0, width < CGFloat.greatestFiniteMagnitude {
			return width
		}
		return UIView.noIntrinsicMetric
	}

	private func measureChildren(maxWidth: CGFloat) {
		measuredSizes = subviews.map { child in
			if child is InlineLineBreakView {
				return InlineItemSize(width: 0, height: 0, isLineBreak: true)
			}
			let fit = child.sizeThatFits(CGSize(width: maxWidth, height: CGFloat.greatestFiniteMagnitude))
			return InlineItemSize(width: ceil(fit.width), height: ceil(fit.height))
		}
	}

	private func contentWidthLimit(_ width: CGFloat) -> CGFloat {
		if width <= 0 || width == UIView.noIntrinsicMetric {
			return CGFloat.greatestFiniteMagnitude / 4
		}
		return max(0, width - contentInsets.left - contentInsets.right)
	}

	private func contentHeight() -> CGFloat {
		if measuredLines.isEmpty {
			return 0
		}
		return measuredLines.map(\.height).reduce(0, +) + lineSpacing * CGFloat(measuredLines.count - 1)
	}

	private func contentWidth() -> CGFloat {
		return measuredLines.map(\.width).max() ?? 0
	}

	private func layout(line: InlineLine, top: CGFloat) {
		var x = contentInsets.left
		for index in line.start..<line.end {
			let child = subviews[index]
			let size = measuredSizes[index]
			let y = top + max(0, line.height - size.height) / 2
			child.frame = CGRect(x: x, y: y, width: size.width, height: size.height)
			x += size.width
		}
	}
}

final class InlineLineBreakView: UIView {
	override func sizeThatFits(_ size: CGSize) -> CGSize { .zero }
	override var intrinsicContentSize: CGSize { .zero }
}
#endif
