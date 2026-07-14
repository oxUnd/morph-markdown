import SwiftUI

public protocol MorphMathRenderer {
	associatedtype Body: View

	@ViewBuilder
	func render(latex: String, display: Bool, theme: MorphMarkdownTheme) -> Body
}

public protocol MorphImageLoader {
	associatedtype Body: View

	@ViewBuilder
	func image(url: String, theme: MorphMarkdownTheme) -> Body
}

public struct PlaceholderMathRenderer: MorphMathRenderer {
	public init() {}

	public func render(latex: String, display: Bool, theme: MorphMarkdownTheme) -> some View {
		Text(latex).font(theme.inlineCodeFont)
	}
}

public struct PlaceholderImageLoader: MorphImageLoader {
	public init() {}

	public func image(url: String, theme: MorphMarkdownTheme) -> some View {
		Text("image: \(url)")
			.font(theme.inlineCodeFont)
			.padding(8)
			.border(Color.secondary)
	}
}
