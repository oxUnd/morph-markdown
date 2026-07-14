import SwiftUI
#if canImport(UIKit)
import UIKit
#elseif canImport(AppKit)
import AppKit
#endif

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
		Text(latex)
			.font(.system(size: theme.mathSize(), design: .monospaced))
			.padding(.horizontal, display ? 12 : 0)
			.padding(.vertical, display ? 8 : 0)
			.frame(maxWidth: display ? .infinity : nil, alignment: .center)
	}
}

public struct MathJaxMathRenderer: MorphMathRenderer {
	private let fontPath: String?

	public init(fontPath: String? = nil) {
		self.fontPath = fontPath
	}

	public func render(latex: String, display: Bool, theme: MorphMarkdownTheme) -> some View {
		if let image = MorphNativeMath.renderLatex(
			fontPath: resolvedFontPath(),
			latex: latex,
			display: display,
			fontSize: theme.mathSize()
		) {
			image
				.resizable()
				.interpolation(.none)
				.aspectRatio(contentMode: .fit)
				.fixedSize()
				.padding(.vertical, display ? 12 : 0)
				.frame(maxWidth: display ? .infinity : nil, alignment: .center)
		} else {
			PlaceholderMathRenderer().render(latex: latex, display: display, theme: theme)
		}
	}

	private func resolvedFontPath() -> String? {
		if let fontPath { return fontPath }
		return nil
	}
}

public struct PlaceholderImageLoader: MorphImageLoader {
	public init() {}

	public func image(url: String, theme: MorphMarkdownTheme) -> some View {
		Text("invalid image: \(displayName(url))")
			.font(theme.inlineCodeFont)
			.padding(.horizontal, theme.codeBlockPaddingHorizontalDp)
			.padding(.vertical, theme.codeBlockPaddingVerticalDp)
			.frame(maxWidth: .infinity, alignment: .leading)
			.background(Color(red: 0.93, green: 0.93, blue: 0.90))
			.overlay(Rectangle().stroke(Color.secondary.opacity(0.45), lineWidth: 1))
	}

	private func displayName(_ url: String) -> String {
		let path = url.hasPrefix("file://") ? String(url.dropFirst("file://".count)) : url
		return URL(fileURLWithPath: path).lastPathComponent
	}
}

public struct FileImageLoader: MorphImageLoader {
	public init() {}

	public func image(url: String, theme: MorphMarkdownTheme) -> some View {
		let path = url.hasPrefix("file://") ? String(url.dropFirst("file://".count)) : url
		loadedImage(path: path, url: url, theme: theme)
	}

	@ViewBuilder
	private func loadedImage(path: String, url: String, theme: MorphMarkdownTheme) -> some View {
		#if canImport(UIKit)
		if let image = UIImage(contentsOfFile: path) {
			Image(uiImage: image)
				.resizable()
				.aspectRatio(contentMode: .fit)
				.frame(maxWidth: theme.imageMaxWidthDp, maxHeight: theme.imageMaxHeightDp)
				.padding(.vertical, 6)
		} else {
			PlaceholderImageLoader().image(url: url, theme: theme)
		}
		#elseif canImport(AppKit)
		if let image = NSImage(contentsOfFile: path) {
			Image(nsImage: image)
				.resizable()
				.aspectRatio(contentMode: .fit)
				.frame(maxWidth: theme.imageMaxWidthDp, maxHeight: theme.imageMaxHeightDp)
				.padding(.vertical, 6)
		} else {
			PlaceholderImageLoader().image(url: url, theme: theme)
		}
		#else
		PlaceholderImageLoader().image(url: url, theme: theme)
		#endif
	}
}

private enum MorphNativeMath {
	static func renderLatex(
		fontPath: String?,
		latex: String,
		display: Bool,
		fontSize: CGFloat
	) -> Image? {
		// Hook point for the native MathJax-C bridge. The bridge needs iOS
		// FreeType/Harfbuzz artifacts, which are not present in this checkout.
		_ = (fontPath, latex, display, fontSize)
		return nil
	}
}
