#if canImport(UIKit)
import UIKit

public final class MorphMarkdownUIView: UIScrollView {
	private var engine: MorphMarkdownEngine
	private let body = UIStackView()
	private let renderer = MorphMarkdownRenderer()
	private let renderDebounce = RenderDebounceState()
	private var scheduledRender: DispatchWorkItem?
	private var progressiveRender: DispatchWorkItem?
	private var applyingConfiguration = false
	private lazy var contentLongPressRecognizer = UILongPressGestureRecognizer(
		target: self,
		action: #selector(handleContentLongPress(_:))
	)

	public var options = MorphMarkdownOptions()
	public var layoutMode: MorphMarkdownLayoutMode = .scrollable {
		didSet {
			guard layoutMode != oldValue else { return }
			applyLayoutMode()
			invalidateIntrinsicContentSize()
			if !applyingConfiguration {
				renderSnapshot(autoScroll: false)
			}
		}
	}
	public var onContentLongPress: MorphMarkdownContentLongPressHandler?
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
		}
	}
	public var onRendered: (() -> Void)?
	public var onPlainTextRendered: MorphMarkdownPlainTextHandler?

	public func apply(configuration: MorphMarkdownConfiguration) {
		let nextWidth = Self.validViewportWidth(configuration.viewportWidth)
		let requiresRender = layoutMode != configuration.layoutMode ||
			renderer.theme != configuration.theme ||
			renderer.mathRenderer !== configuration.mathRenderer ||
			renderer.imageLoader !== configuration.imageLoader ||
			renderer.viewportWidthOverride != nextWidth
		options = configuration.options
		applyingConfiguration = true
		layoutMode = configuration.layoutMode
		applyingConfiguration = false
		renderer.theme = configuration.theme
		renderer.mathRenderer = configuration.mathRenderer
		renderer.imageLoader = configuration.imageLoader
		renderer.onLinkClick = configuration.onLinkClick
		renderer.viewportWidthOverride = nextWidth
		onRendered = configuration.onRendered
		onPlainTextRendered = configuration.onPlainTextRendered
		onContentLongPress = configuration.onContentLongPress
		if requiresRender {
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
		switch renderDebounce.onAppend(
			final: final,
			autoScroll: options.autoScrollOnAppend,
			debounceMilliseconds: options.appendRenderDebounceMilliseconds
		) {
		case .none:
			break
		case .renderNow(let autoScroll):
			scheduleRender(afterMilliseconds: 0, autoScroll: autoScroll)
		case .schedule(let delay):
			scheduleRender(afterMilliseconds: delay, autoScroll: nil)
		}
	}

	private func scheduleRender(afterMilliseconds delay: Int, autoScroll: Bool?) {
		scheduledRender?.cancel()
		let work = DispatchWorkItem { [weak self] in
			guard let self else { return }
			self.scheduledRender = nil
			let shouldScroll = autoScroll ?? self.renderDebounce.onScheduledRender()
			self.performRenderSnapshot(autoScroll: shouldScroll, reuseStablePrefix: true)
		}
		scheduledRender = work
		DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(delay), execute: work)
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

	public override var intrinsicContentSize: CGSize {
		guard layoutMode == .intrinsicHeight else {
			return CGSize(width: UIView.noIntrinsicMetric, height: UIView.noIntrinsicMetric)
		}
		let width = Self.validViewportWidth(viewportWidthOverride) ?? bounds.width
		guard width > 0 else {
			return CGSize(width: UIView.noIntrinsicMetric, height: UIView.noIntrinsicMetric)
		}
		return sizeThatFits(CGSize(width: width, height: UIView.noIntrinsicMetric))
	}

	public func reset() {
		cancelScheduledRender()
		engine.close()
		guard let next = MorphMarkdownEngine() else {
			renderError()
			return
		}
		engine = next
		renderer.reset(parent: body)
		invalidateIntrinsicContentSize()
		onRendered?()
		onPlainTextRendered?("")
	}

	public func renderSnapshot(autoScroll: Bool = false, reuseStablePrefix: Bool = false) {
		cancelScheduledRender()
		performRenderSnapshot(autoScroll: autoScroll, reuseStablePrefix: reuseStablePrefix)
	}

	private func performRenderSnapshot(autoScroll: Bool, reuseStablePrefix: Bool) {
		guard let json = engine.snapshotJson() else {
			renderError()
			return
		}
		var hasProgressiveTail = false
		if reuseStablePrefix, layoutMode == .scrollable {
			hasProgressiveTail = renderer.renderReusingStablePrefixProgressively(
				json: json,
				parent: body,
				stableBlockCount: engine.stableBlockCount(),
				initialTailCount: 1
			)
		} else if reuseStablePrefix {
			renderer.renderReusingStablePrefix(json: json, parent: body, stableBlockCount: engine.stableBlockCount())
		} else if layoutMode == .scrollable {
			hasProgressiveTail = renderer.renderProgressively(json: json, parent: body, initialBlockCount: 8)
		} else {
			renderer.render(json: json, parent: body)
		}
		body.setNeedsLayout()
		layoutIfNeeded()
		invalidateIntrinsicContentSize()
		superview?.setNeedsLayout()
		if autoScroll, !hasProgressiveTail {
			scrollToBottom()
		}
		onRendered?()
		onPlainTextRendered?(renderer.renderedPlainText)
		if hasProgressiveTail {
			scheduleProgressiveRender(autoScrollWhenComplete: autoScroll)
		}
	}

	public func close() {
		cancelScheduledRender()
		engine.close()
	}

	private func cancelScheduledRender() {
		scheduledRender?.cancel()
		scheduledRender = nil
		progressiveRender?.cancel()
		progressiveRender = nil
		renderer.cancelProgressiveRender()
		renderDebounce.cancel()
	}

	private func scheduleProgressiveRender(autoScrollWhenComplete: Bool) {
		let work = DispatchWorkItem { [weak self] in
			guard let self else { return }
			self.progressiveRender = nil
			let hasMore = self.renderer.renderNextProgressiveBatch(parent: self.body, count: 1)
			self.body.setNeedsLayout()
			self.setNeedsLayout()
			if hasMore {
				self.scheduleProgressiveRender(autoScrollWhenComplete: autoScrollWhenComplete)
			} else {
				self.invalidateIntrinsicContentSize()
				if autoScrollWhenComplete {
					self.scrollToBottom()
				}
			}
		}
		progressiveRender = work
		DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(1), execute: work)
	}

	private func configure() {
		alwaysBounceVertical = false
		addGestureRecognizer(contentLongPressRecognizer)
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
		applyLayoutMode()
	}

	private func applyLayoutMode() {
		isScrollEnabled = layoutMode == .scrollable
		alwaysBounceVertical = layoutMode == .scrollable
	}

	@objc private func handleContentLongPress(_ recognizer: UILongPressGestureRecognizer) {
		guard recognizer.state == .began else { return }
		let point = recognizer.location(in: self)
		if onContentLongPress?(point) == true {
			UIImpactFeedbackGenerator(style: .medium).impactOccurred()
		}
	}

	private func renderError() {
		renderer.reset(parent: body)
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
