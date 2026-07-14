import SwiftUI

public enum MorphListMarkerStyle: Sendable {
	case disc
	case circle
	case square
	case hyphen
}

public struct MorphMarkdownTheme: Sendable {
	public var bodyFont: Font
	public var headingFonts: [Font]
	public var codeFont: Font
	public var inlineCodeFont: Font
	public var bodyLineSpacing: CGFloat
	public var paragraphVerticalSpacing: CGFloat
	public var headingTopSpacing: CGFloat
	public var headingBottomSpacing: CGFloat
	public var tabSize: Int
	public var listIndent: CGFloat
	public var nestedListIndent: CGFloat
	public var listMarkerWidth: CGFloat
	public var listMarkerSize: CGFloat
	public var unorderedListMarkers: [MorphListMarkerStyle]
	public var blockquoteIndent: CGFloat
	public var tableCellMaxWidth: CGFloat
	public var tableCellWrap: Bool
	public var tableCellPaddingHorizontal: CGFloat
	public var tableCellPaddingVertical: CGFloat
	public var imageMaxSize: CGSize

	public init(
		bodyFont: Font = .body,
		headingFonts: [Font] = [.title, .title2, .title3, .headline, .body, .body],
		codeFont: Font = .system(.body, design: .monospaced),
		inlineCodeFont: Font = .system(.body, design: .monospaced),
		bodyLineSpacing: CGFloat = 2,
		paragraphVerticalSpacing: CGFloat = 4,
		headingTopSpacing: CGFloat = 8,
		headingBottomSpacing: CGFloat = 10,
		tabSize: Int = 4,
		listIndent: CGFloat = 20,
		nestedListIndent: CGFloat = 12,
		listMarkerWidth: CGFloat = 22,
		listMarkerSize: CGFloat = 6,
		unorderedListMarkers: [MorphListMarkerStyle] = [.disc, .circle, .square],
		blockquoteIndent: CGFloat = 12,
		tableCellMaxWidth: CGFloat = 280,
		tableCellWrap: Bool = true,
		tableCellPaddingHorizontal: CGFloat = 8,
		tableCellPaddingVertical: CGFloat = 8,
		imageMaxSize: CGSize = CGSize(width: 320, height: 180)
	) {
		self.bodyFont = bodyFont
		self.headingFonts = headingFonts
		self.codeFont = codeFont
		self.inlineCodeFont = inlineCodeFont
		self.bodyLineSpacing = bodyLineSpacing
		self.paragraphVerticalSpacing = paragraphVerticalSpacing
		self.headingTopSpacing = headingTopSpacing
		self.headingBottomSpacing = headingBottomSpacing
		self.tabSize = tabSize
		self.listIndent = listIndent
		self.nestedListIndent = nestedListIndent
		self.listMarkerWidth = listMarkerWidth
		self.listMarkerSize = listMarkerSize
		self.unorderedListMarkers = unorderedListMarkers
		self.blockquoteIndent = blockquoteIndent
		self.tableCellMaxWidth = tableCellMaxWidth
		self.tableCellWrap = tableCellWrap
		self.tableCellPaddingHorizontal = tableCellPaddingHorizontal
		self.tableCellPaddingVertical = tableCellPaddingVertical
		self.imageMaxSize = imageMaxSize
	}

	public func headingFont(level: Int) -> Font {
		let index = min(max(level - 1, 0), headingFonts.count - 1)
		return headingFonts[index]
	}

	public static let hetiLike = MorphMarkdownTheme(
		bodyFont: .custom("Songti SC", size: 16, relativeTo: .body),
		headingFonts: [
			.custom("Songti SC", size: 32, relativeTo: .largeTitle),
			.custom("Songti SC", size: 24, relativeTo: .title2),
			.custom("Songti SC", size: 20, relativeTo: .title3),
			.custom("Songti SC", size: 18, relativeTo: .headline),
			.custom("Songti SC", size: 16, relativeTo: .body),
			.custom("Songti SC", size: 14, relativeTo: .subheadline)
		],
		codeFont: .system(size: 14, design: .monospaced),
		inlineCodeFont: .system(size: 14, design: .monospaced),
		bodyLineSpacing: 8,
		paragraphVerticalSpacing: 12,
		headingTopSpacing: 24,
		headingBottomSpacing: 12,
		listIndent: 32,
		nestedListIndent: 10,
		tableCellPaddingHorizontal: 8,
		tableCellPaddingVertical: 6
	)

	public static let hetiLikeHei = MorphMarkdownTheme(
		bodyFont: .custom("PingFang SC", size: 16, relativeTo: .body),
		headingFonts: [
			.custom("PingFang SC", size: 32, relativeTo: .largeTitle),
			.custom("PingFang SC", size: 24, relativeTo: .title2),
			.custom("PingFang SC", size: 20, relativeTo: .title3),
			.custom("PingFang SC", size: 18, relativeTo: .headline),
			.custom("PingFang SC", size: 16, relativeTo: .body),
			.custom("PingFang SC", size: 14, relativeTo: .subheadline)
		],
		codeFont: .system(size: 14, design: .monospaced),
		inlineCodeFont: .system(size: 14, design: .monospaced),
		bodyLineSpacing: 8,
		paragraphVerticalSpacing: 12,
		headingTopSpacing: 24,
		headingBottomSpacing: 12,
		listIndent: 32,
		nestedListIndent: 10,
		tableCellPaddingHorizontal: 8,
		tableCellPaddingVertical: 6
	)
}
