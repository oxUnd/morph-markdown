#if os(iOS) && canImport(SwiftUI) && canImport(UIKit)
import SwiftUI

public struct MorphMarkdownView: UIViewRepresentable {
	private let chunks: [String]
	private let isStreaming: Bool
	private let theme: MorphMarkdownTheme
	private let mathRenderer: MorphMathRenderer?
	private let imageLoader: MorphImageLoader
	private let viewportWidth: CGFloat?
	private let onRendered: (() -> Void)?
	private let onLinkClick: MorphMarkdownLinkHandler?
	private let options: MorphMarkdownOptions
	private let layoutMode: MorphMarkdownLayoutMode
	private let onContentLongPress: MorphMarkdownContentLongPressHandler?

	public init(
		markdown: String,
		isStreaming: Bool = false,
		theme: MorphMarkdownTheme = MorphMarkdownThemes.normal,
		mathRenderer: MorphMathRenderer? = MathJaxMathRenderer.bundled,
		imageLoader: MorphImageLoader = FileImageLoader.shared,
		viewportWidth: CGFloat? = nil,
		options: MorphMarkdownOptions = MorphMarkdownOptions(),
		layoutMode: MorphMarkdownLayoutMode = .intrinsicHeight,
		onRendered: (() -> Void)? = nil,
		onLinkClick: MorphMarkdownLinkHandler? = nil,
		onContentLongPress: MorphMarkdownContentLongPressHandler? = nil
	) {
		chunks = [markdown]
		self.isStreaming = isStreaming
		self.theme = theme
		self.mathRenderer = mathRenderer
		self.imageLoader = imageLoader
		self.viewportWidth = viewportWidth
		self.options = options
		self.layoutMode = layoutMode
		self.onRendered = onRendered
		self.onLinkClick = onLinkClick
		self.onContentLongPress = onContentLongPress
	}

	public func makeUIView(context: Context) -> MorphMarkdownUIView {
		let view = MorphMarkdownUIView()
		view.apply(configuration: configuration)
		return view
	}

	public func updateUIView(_ uiView: MorphMarkdownUIView, context: Context) {
		uiView.apply(configuration: configuration)
		context.coordinator.render(chunks: chunks, isStreaming: isStreaming, into: uiView)
	}

	private var configuration: MorphMarkdownConfiguration {
		MorphMarkdownConfiguration(
			theme: theme,
			options: options,
			layoutMode: layoutMode,
			viewportWidth: viewportWidth,
			mathRenderer: mathRenderer,
			imageLoader: imageLoader,
			onRendered: onRendered,
			onLinkClick: onLinkClick,
			onContentLongPress: onContentLongPress
		)
	}

	@available(iOS 16.0, *)
	public func sizeThatFits(
		_ proposal: ProposedViewSize,
		uiView: MorphMarkdownUIView,
		context: Context
	) -> CGSize? {
		guard let width = proposal.width, width > 0 else {
			return nil
		}
		return uiView.sizeThatFits(CGSize(width: width, height: .greatestFiniteMagnitude))
	}

	public func makeCoordinator() -> Coordinator {
		return Coordinator()
	}

	public final class Coordinator {
		private var lastMarkdown: String?
		private var lastWasStreaming = false

		func render(chunks: [String], isStreaming: Bool, into view: MorphMarkdownUIView) {
			let markdown = chunks.joined()
			if markdown == lastMarkdown, isStreaming == lastWasStreaming {
				return
			}
			if let lastMarkdown, markdown.hasPrefix(lastMarkdown) {
				let delta = String(markdown.dropFirst(lastMarkdown.count))
				view.append(delta, final: !isStreaming)
			} else {
				view.setMarkdown(markdown, final: !isStreaming)
			}
			lastMarkdown = markdown
			lastWasStreaming = isStreaming
		}
	}
}
#endif
