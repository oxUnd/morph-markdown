#if canImport(UIKit)
import CoreGraphics
import Foundation

public enum MorphMarkdownLayoutMode: Equatable {
	case scrollable
	case intrinsicHeight
}

public typealias MorphMarkdownContentLongPressHandler = (_ point: CGPoint) -> Bool
public typealias MorphMarkdownPlainTextHandler = (_ text: String) -> Void

public struct MorphMarkdownConfiguration {
	public var theme: MorphMarkdownTheme
	public var options: MorphMarkdownOptions
	public var layoutMode: MorphMarkdownLayoutMode
	public var viewportWidth: CGFloat?
	public var mathRenderer: MorphMathRenderer?
	public var imageLoader: MorphImageLoader
	public var onRendered: (() -> Void)?
	public var onPlainTextRendered: MorphMarkdownPlainTextHandler?
	public var onLinkClick: MorphMarkdownLinkHandler?
	public var onContentLongPress: MorphMarkdownContentLongPressHandler?

	public init(
		theme: MorphMarkdownTheme = MorphMarkdownThemes.normal,
		options: MorphMarkdownOptions = MorphMarkdownOptions(),
		layoutMode: MorphMarkdownLayoutMode = .scrollable,
		viewportWidth: CGFloat? = nil,
		mathRenderer: MorphMathRenderer? = MathJaxMathRenderer.bundled,
		imageLoader: MorphImageLoader = FileImageLoader.shared,
		onRendered: (() -> Void)? = nil,
		onPlainTextRendered: MorphMarkdownPlainTextHandler? = nil,
		onLinkClick: MorphMarkdownLinkHandler? = nil,
		onContentLongPress: MorphMarkdownContentLongPressHandler? = nil
	) {
		self.theme = theme
		self.options = options
		self.layoutMode = layoutMode
		self.viewportWidth = viewportWidth
		self.mathRenderer = mathRenderer
		self.imageLoader = imageLoader
		self.onRendered = onRendered
		self.onPlainTextRendered = onPlainTextRendered
		self.onLinkClick = onLinkClick
		self.onContentLongPress = onContentLongPress
	}
}
#endif
