#if os(iOS) && canImport(SwiftUI) && canImport(UIKit)
import SwiftUI

public struct MorphMarkdownView: UIViewRepresentable {
	private let chunks: [String]
	private let theme: MorphMarkdownTheme
	private let mathRenderer: MorphMathRenderer?
	private let imageLoader: MorphImageLoader

	public init(
		markdown: String,
		theme: MorphMarkdownTheme = MorphMarkdownThemes.normal,
		mathRenderer: MorphMathRenderer? = MathJaxMathRenderer(),
		imageLoader: MorphImageLoader = FileImageLoader()
	) {
		chunks = [markdown]
		self.theme = theme
		self.mathRenderer = mathRenderer
		self.imageLoader = imageLoader
	}

	public func makeUIView(context: Context) -> MorphMarkdownUIView {
		let view = MorphMarkdownUIView()
		view.theme = theme
		view.mathRenderer = mathRenderer
		view.imageLoader = imageLoader
		return view
	}

	public func updateUIView(_ uiView: MorphMarkdownUIView, context: Context) {
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
