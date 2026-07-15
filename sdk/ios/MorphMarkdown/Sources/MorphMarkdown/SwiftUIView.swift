#if os(iOS) && canImport(SwiftUI) && canImport(UIKit)
import SwiftUI

public struct MorphMarkdownView: UIViewRepresentable {
	private let chunks: [String]
	private let theme: MorphMarkdownTheme
	private let mathRenderer: MorphMathRenderer?
	private let imageLoader: MorphImageLoader
	private let viewportWidth: CGFloat?
	private let onLinkClick: MorphMarkdownLinkHandler?

	public init(
		markdown: String,
		theme: MorphMarkdownTheme = MorphMarkdownThemes.normal,
		mathRenderer: MorphMathRenderer? = MathJaxMathRenderer(),
		imageLoader: MorphImageLoader = FileImageLoader(),
		viewportWidth: CGFloat? = nil,
		onLinkClick: MorphMarkdownLinkHandler? = nil
	) {
		chunks = [markdown]
		self.theme = theme
		self.mathRenderer = mathRenderer
		self.imageLoader = imageLoader
		self.viewportWidth = viewportWidth
		self.onLinkClick = onLinkClick
	}

	public func makeUIView(context: Context) -> MorphMarkdownUIView {
		let view = MorphMarkdownUIView()
		view.theme = theme
		view.mathRenderer = mathRenderer
		view.imageLoader = imageLoader
		view.viewportWidthOverride = viewportWidth
		view.onLinkClick = onLinkClick
		return view
	}

	public func updateUIView(_ uiView: MorphMarkdownUIView, context: Context) {
		uiView.viewportWidthOverride = viewportWidth
		uiView.onLinkClick = onLinkClick
		context.coordinator.render(chunks: chunks, into: uiView)
	}

	public func makeCoordinator() -> Coordinator {
		return Coordinator()
	}

	public final class Coordinator {
		private var lastMarkdown: String?

		func render(chunks: [String], into view: MorphMarkdownUIView) {
			let markdown = chunks.joined()
			if markdown == lastMarkdown {
				return
			}
			view.setMarkdown(markdown)
			lastMarkdown = markdown
		}
	}
}
#endif
