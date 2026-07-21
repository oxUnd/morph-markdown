#if canImport(UIKit)
import UIKit

final class ListMarkerView: UIView {
	private let style: MorphListMarkerStyle
	private let theme: MorphMarkdownTheme

	init(style: MorphListMarkerStyle, theme: MorphMarkdownTheme) {
		self.style = style
		self.theme = theme
		super.init(frame: .zero)
		backgroundColor = .clear
	}

	required init?(coder: NSCoder) {
		return nil
	}

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		return CGSize(width: theme.listMarkerWidth, height: theme.bodyTextSize * theme.bodyLineHeightMultiplier)
	}

	override var intrinsicContentSize: CGSize {
		return sizeThatFits(.zero)
	}

	override func systemLayoutSizeFitting(_ targetSize: CGSize) -> CGSize {
		return sizeThatFits(targetSize)
	}

	override func draw(_ rect: CGRect) {
		UIColor(argb: theme.bodyTextColor).setStroke()
		UIColor(argb: theme.bodyTextColor).setFill()
		switch style {
		case .disc:
			drawDisc()
		case .circle:
			drawCircle()
		case .square:
			drawSquare()
		case .hyphen:
			drawHyphen()
		}
	}

	private func drawDisc() {
		UIBezierPath(ovalIn: markerRect()).fill()
	}

	private func drawCircle() {
		let path = UIBezierPath(ovalIn: markerRect())
		path.lineWidth = 1.5
		path.stroke()
	}

	private func drawSquare() {
		UIBezierPath(rect: markerRect()).fill()
	}

	private func drawHyphen() {
		let path = UIBezierPath()
		path.lineWidth = 1.5
		path.move(to: CGPoint(x: bounds.midX - theme.listMarkerSize / 2, y: bounds.midY))
		path.addLine(to: CGPoint(x: bounds.midX + theme.listMarkerSize / 2, y: bounds.midY))
		path.stroke()
	}

	private func markerRect() -> CGRect {
		return CGRect(x: bounds.midX - theme.listMarkerSize / 2,
			      y: bounds.midY - theme.listMarkerSize / 2,
			      width: theme.listMarkerSize,
			      height: theme.listMarkerSize)
	}
}

final class TaskMarkerView: UIView {
	private let checked: Bool
	private let theme: MorphMarkdownTheme

	init(checked: Bool, theme: MorphMarkdownTheme) {
		self.checked = checked
		self.theme = theme
		super.init(frame: .zero)
		backgroundColor = .clear
	}

	required init?(coder: NSCoder) {
		return nil
	}

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		return CGSize(width: theme.taskBoxSize, height: theme.taskBoxSize)
	}

	override var intrinsicContentSize: CGSize {
		return sizeThatFits(.zero)
	}

	override func systemLayoutSizeFitting(_ targetSize: CGSize) -> CGSize {
		return sizeThatFits(targetSize)
	}

	override func draw(_ rect: CGRect) {
		drawBox()
		if checked {
			drawCheck()
		}
	}

	private func drawBox() {
		let rect = bounds.insetBy(dx: 2, dy: 2)
		let path = UIBezierPath(roundedRect: rect, cornerRadius: 2)
		if checked {
			UIColor(argb: 0xffd4d4d0).setFill()
			path.fill()
		} else {
			UIColor(argb: 0xffd4d4d0).setStroke()
			path.lineWidth = 2
			path.stroke()
		}
	}

	private func drawCheck() {
		let path = UIBezierPath()
		path.lineWidth = 2.4
		path.lineCapStyle = .round
		path.lineJoinStyle = .round
		UIColor.white.setStroke()
		path.move(to: CGPoint(x: bounds.width * 0.28, y: bounds.height * 0.52))
		path.addLine(to: CGPoint(x: bounds.width * 0.43, y: bounds.height * 0.68))
		path.addLine(to: CGPoint(x: bounds.width * 0.74, y: bounds.height * 0.34))
		path.stroke()
	}
}
#endif
