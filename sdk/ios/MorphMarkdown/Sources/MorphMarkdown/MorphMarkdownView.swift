import SwiftUI

public struct MorphMarkdownView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	@ObservedObject private var engine: MorphMarkdownEngine
	private let theme: MorphMarkdownTheme
	private let options: MorphMarkdownOptions
	private let mathRenderer: Math
	private let imageLoader: Images

	public init(
		engine: MorphMarkdownEngine,
		options: MorphMarkdownOptions = MorphMarkdownOptions(),
		theme: MorphMarkdownTheme = MorphMarkdownThemes.Normal,
		mathRenderer: Math,
		imageLoader: Images
	) {
		self.engine = engine
		self.options = options
		self.theme = theme
		self.mathRenderer = mathRenderer
		self.imageLoader = imageLoader
	}

	public var body: some View {
		ScrollViewReader { proxy in
			ScrollView {
				LazyVStack(alignment: .leading, spacing: 0) {
					ForEach(engine.nodes) { node in
						BlockNodeView(
							node: node,
							theme: theme,
							mathRenderer: mathRenderer,
							imageLoader: imageLoader
						)
					}
					Color.clear.frame(height: 1).id("__morph_markdown_bottom")
				}
				.frame(maxWidth: .infinity, alignment: .leading)
				.padding(.horizontal, 20)
				.padding(.top, 16)
				.padding(.bottom, 32)
			}
			.onChange(of: engine.nodes.count) { _ in
				if options.autoScrollOnAppend {
					proxy.scrollTo("__morph_markdown_bottom", anchor: .bottom)
				}
			}
		}
	}
}

public extension MorphMarkdownView
where Math == MathJaxMathRenderer, Images == FileImageLoader {
	init(
		engine: MorphMarkdownEngine,
		options: MorphMarkdownOptions = MorphMarkdownOptions(),
		theme: MorphMarkdownTheme = MorphMarkdownThemes.Normal
	) {
		self.init(
			engine: engine,
			options: options,
			theme: theme,
			mathRenderer: MathJaxMathRenderer(),
			imageLoader: FileImageLoader()
		)
	}
}

public extension MorphMarkdownEngine {
	func renderSnapshot() {
		reloadSnapshot()
	}
}
