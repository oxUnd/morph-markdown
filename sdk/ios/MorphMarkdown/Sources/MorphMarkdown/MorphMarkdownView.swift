import SwiftUI

public struct MorphMarkdownView<Math: MorphMathRenderer, Images: MorphImageLoader>: View {
	@ObservedObject private var engine: MorphMarkdownEngine
	private let theme: MorphMarkdownTheme
	private let mathRenderer: Math
	private let imageLoader: Images

	public init(
		engine: MorphMarkdownEngine,
		theme: MorphMarkdownTheme = MorphMarkdownTheme(),
		mathRenderer: Math,
		imageLoader: Images
	) {
		self.engine = engine
		self.theme = theme
		self.mathRenderer = mathRenderer
		self.imageLoader = imageLoader
	}

	public var body: some View {
		ScrollView {
			LazyVStack(alignment: .leading, spacing: 8) {
				ForEach(engine.nodes) { node in
					BlockNodeView(
						node: node,
						theme: theme,
						mathRenderer: mathRenderer,
						imageLoader: imageLoader
					)
				}
			}
			.frame(maxWidth: .infinity, alignment: .leading)
			.padding()
		}
	}
}

public extension MorphMarkdownView
where Math == PlaceholderMathRenderer, Images == PlaceholderImageLoader {
	init(engine: MorphMarkdownEngine, theme: MorphMarkdownTheme = MorphMarkdownTheme()) {
		self.init(
			engine: engine,
			theme: theme,
			mathRenderer: PlaceholderMathRenderer(),
			imageLoader: PlaceholderImageLoader()
		)
	}
}
