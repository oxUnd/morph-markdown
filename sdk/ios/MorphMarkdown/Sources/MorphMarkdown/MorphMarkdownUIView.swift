#if canImport(UIKit)
import UIKit

public final class MorphMarkdownUIView: UIScrollView {
	private var engine: MorphMarkdownEngine
	private let body = UIStackView()
	private let renderer = MorphMarkdownRenderer()

	public var options = MorphMarkdownOptions()
	public var viewportWidthOverride: CGFloat? {
		get { renderer.viewportWidthOverride }
		set {
			let width = Self.validViewportWidth(newValue)
			guard renderer.viewportWidthOverride != width else {
				return
			}
			renderer.viewportWidthOverride = width
			renderSnapshot(autoScroll: false)
		}
	}

	public var theme: MorphMarkdownTheme {
		get { renderer.theme }
		set {
			renderer.theme = newValue
			renderSnapshot(autoScroll: false)
		}
	}

	public var mathRenderer: MorphMathRenderer? {
		get { renderer.mathRenderer }
		set {
			renderer.mathRenderer = newValue
			renderSnapshot(autoScroll: false)
		}
	}

	public var imageLoader: MorphImageLoader {
		get { renderer.imageLoader }
		set {
			renderer.imageLoader = newValue
			renderSnapshot(autoScroll: false)
		}
	}

	public var onLinkClick: MorphMarkdownLinkHandler? {
		get { renderer.onLinkClick }
		set {
			renderer.onLinkClick = newValue
			renderSnapshot(autoScroll: false)
		}
	}

	public override init(frame: CGRect) {
		guard let engine = MorphMarkdownEngine() else {
			fatalError("failed to create MorphMarkdownEngine")
		}
		self.engine = engine
		super.init(frame: frame)
		configure()
	}

	required init?(coder: NSCoder) {
		guard let engine = MorphMarkdownEngine() else {
			return nil
		}
		self.engine = engine
		super.init(coder: coder)
		configure()
	}

	deinit {
		close()
	}

	public func append(_ markdown: String, final: Bool = false) {
		_ = engine.append(markdown, final: final)
		renderSnapshot(autoScroll: options.autoScrollOnAppend)
	}

	public func setMarkdown(_ markdown: String, final: Bool = true) {
		engine.close()
		guard let next = MorphMarkdownEngine() else {
			renderError()
			return
		}
		engine = next
		_ = engine.append(markdown, final: final)
		renderSnapshot(autoScroll: false)
	}

	public override func sizeThatFits(_ size: CGSize) -> CGSize {
		let width = Self.validViewportWidth(size.width) ?? bounds.width
		guard width > 0 else {
			return super.sizeThatFits(size)
		}
		viewportWidthOverride = width
		let target = CGSize(width: width, height: UIView.layoutFittingCompressedSize.height)
		let fit = body.systemLayoutSizeFitting(
			target,
			withHorizontalFittingPriority: .required,
			verticalFittingPriority: .fittingSizeLevel
		)
		return CGSize(width: width, height: ceil(max(1, fit.height)))
	}

	public func reset() {
		engine.close()
		guard let next = MorphMarkdownEngine() else {
			renderError()
			return
		}
		engine = next
		body.removeAllArrangedSubviews()
	}

	public func renderSnapshot(autoScroll: Bool = false) {
		guard let json = engine.snapshotJson() else {
			renderError()
			return
		}
		renderer.render(json: json, parent: body)
		body.setNeedsLayout()
		layoutIfNeeded()
		if autoScroll {
			scrollToBottom()
		}
	}

	public func close() {
		engine.close()
	}

	private func configure() {
		alwaysBounceVertical = false
		isScrollEnabled = false
		addSubview(body)
		body.axis = .vertical
		body.alignment = .fill
		body.distribution = .fill
		body.translatesAutoresizingMaskIntoConstraints = false
		NSLayoutConstraint.activate([
			body.leadingAnchor.constraint(equalTo: contentLayoutGuide.leadingAnchor),
			body.trailingAnchor.constraint(equalTo: contentLayoutGuide.trailingAnchor),
			body.topAnchor.constraint(equalTo: contentLayoutGuide.topAnchor),
			body.bottomAnchor.constraint(equalTo: contentLayoutGuide.bottomAnchor),
			body.widthAnchor.constraint(equalTo: frameLayoutGuide.widthAnchor)
		])
	}

	private func renderError() {
		body.removeAllArrangedSubviews()
		body.addArrangedSubview(configuredLabel("snapshot failed", size: theme.bodyTextSize, theme: theme))
	}

	private func scrollToBottom() {
		let y = max(0, contentSize.height - bounds.height)
		setContentOffset(CGPoint(x: 0, y: y), animated: false)
	}

	private static func validViewportWidth(_ width: CGFloat?) -> CGFloat? {
		guard let width, width > 0, width < CGFloat.greatestFiniteMagnitude else {
			return nil
		}
		return width
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
