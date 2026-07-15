import CoreGraphics
import Foundation

public enum MorphListMarkerStyle: String, Codable, Equatable {
	case disc
	case circle
	case square
	case hyphen
}

public enum MorphTextProcessor: String, Codable, Equatable {
	case none
	case cjkSpacing
}

public enum MorphFontProfile: String, Codable, Equatable {
	case system
	case hetiLikeHei
	case hetiLikeSong
}

public struct MorphTableStyle: Equatable {
	public var borderColor: UInt32
	public var borderWidth: CGFloat
	public var headerBackgroundColor: UInt32
	public var bodyBackgroundColor: UInt32
	public var headerTextColor: UInt32
	public var bodyTextColor: UInt32
	public var headerTextSize: CGFloat?
	public var bodyTextSize: CGFloat?
	public var headerBold: Bool

	public init(
		borderColor: UInt32 = 0xff454545,
		borderWidth: CGFloat = 1,
		headerBackgroundColor: UInt32 = 0xffefefea,
		bodyBackgroundColor: UInt32 = 0x00ffffff,
		headerTextColor: UInt32 = 0xff1b1b1b,
		bodyTextColor: UInt32 = 0xff1b1b1b,
		headerTextSize: CGFloat? = nil,
		bodyTextSize: CGFloat? = nil,
		headerBold: Bool = false
	) {
		self.borderColor = borderColor
		self.borderWidth = borderWidth
		self.headerBackgroundColor = headerBackgroundColor
		self.bodyBackgroundColor = bodyBackgroundColor
		self.headerTextColor = headerTextColor
		self.bodyTextColor = bodyTextColor
		self.headerTextSize = headerTextSize
		self.bodyTextSize = bodyTextSize
		self.headerBold = headerBold
	}
}

public struct MorphMarkdownTheme: Equatable {
	public var bodyTextSize: CGFloat = 16
	public var headingSizes: [CGFloat] = [26, 23, 20, 18, 16, 16]
	public var headingLineHeights: [CGFloat] = [34, 30, 27, 25, 24, 24]
	public var headingTopSpacing: CGFloat = 8
	public var headingBottomSpacing: CGFloat = 10
	public var paragraphTopSpacing: CGFloat = 4
	public var paragraphBottomSpacing: CGFloat = 10
	public var bodyLineHeightMultiplier: CGFloat = 1.18
	public var tabSize: Int = 4
	public var listIndent: CGFloat = 20
	public var nestedListIndent: CGFloat = 12
	public var listMarkerWidth: CGFloat = 22
	public var listMarkerSize: CGFloat = 6
	public var listItemSpacing: CGFloat = 2
	public var orderedMarkerWidth: CGFloat = 28
	public var unorderedListMarkers: [MorphListMarkerStyle] = [.disc, .circle, .square]
	public var taskBoxSize: CGFloat = 22
	public var taskBoxTextGap: CGFloat = 8
	public var blockquoteIndent: CGFloat = 12
	public var blockquoteVerticalPadding: CGFloat = 6
	public var blockquoteBottomSpacing: CGFloat = 12
	public var codeBlockTabSize: Int = 4
	public var codeBlockTextSize: CGFloat = 14
	public var inlineCodeTextSize: CGFloat = 15
	public var codeBlockPaddingHorizontal: CGFloat = 12
	public var codeBlockPaddingVertical: CGFloat = 10
	public var inlineCodePaddingHorizontal: CGFloat = 5
	public var inlineCodePaddingVertical: CGFloat = 2
	public var tableCellMaxWidth: CGFloat = 280
	public var tableCellWrap: Bool = true
	public var tableHorizontalScroll: Bool = true
	public var tableCellPaddingHorizontal: CGFloat = 12
	public var tableCellPaddingVertical: CGFloat = 8
	public var tableTopSpacing: CGFloat = 12
	public var tableBottomSpacing: CGFloat = 14
	public var tableCellLineHeightMultiplier: CGFloat = 1.25
	public var tableCellLineSpacing: CGFloat = 0
	public var tableMathTextScale: CGFloat = 1
	public var tableStyle: MorphTableStyle = MorphTableStyle()
	public var imageMaxWidth: CGFloat = 320
	public var imageMaxHeight: CGFloat = 180
	public var linkTextColor: UInt32 = 0xff2f6f73
	public var linkUnderline: Bool = true
	public var mathTextSizeFollowsBody: Bool = true
	public var mathTextSize: CGFloat = 16
	public var mathTextScale: CGFloat = 1.18
	public var textProcessor: MorphTextProcessor = .none
	public var fontProfile: MorphFontProfile = .system
	public var fontName: String?
	public var boldFontName: String?

	public init() {}

	public func headingSize(_ level: Int) -> CGFloat {
		return headingSizes[index(for: level, in: headingSizes)]
	}

	public func headingLineHeight(_ level: Int) -> CGFloat {
		return headingLineHeights[index(for: level, in: headingLineHeights)]
	}

	public func mathSize() -> CGFloat {
		let base = mathTextSizeFollowsBody ? bodyTextSize : mathTextSize
		return base * mathTextScale
	}

	private func index(for level: Int, in values: [CGFloat]) -> Int {
		return min(max(level - 1, 0), max(values.count - 1, 0))
	}
}

public enum MorphMarkdownThemes {
	public static let normal = MorphMarkdownTheme()

	public static let largeHeadings: MorphMarkdownTheme = {
		var theme = normal
		theme.headingSizes = [30, 26, 22, 19, 17, 16]
		return theme
	}()

	public static let compactHeadings: MorphMarkdownTheme = {
		var theme = normal
		theme.headingSizes = [23, 21, 19, 17, 16, 16]
		return theme
	}()

	public static let hetiLike: MorphMarkdownTheme = {
		var theme = normal
		theme.headingSizes = [32, 24, 20, 18, 16, 14]
		theme.headingLineHeights = [48, 36, 36, 24, 24, 24]
		theme.headingTopSpacing = 20
		theme.headingBottomSpacing = 12
		theme.paragraphTopSpacing = 8
		theme.paragraphBottomSpacing = 20
		theme.bodyLineHeightMultiplier = 1.5
		theme.listIndent = 32
		theme.nestedListIndent = 10
		theme.codeBlockTextSize = 14
		theme.inlineCodeTextSize = 14
		theme.tableCellPaddingHorizontal = 8
		theme.tableCellPaddingVertical = 6
		theme.tableBottomSpacing = 20
		theme.tableCellLineHeightMultiplier = 1.35
		theme.tableStyle.headerBackgroundColor = 0xfff2f2ed
		theme.tableStyle.borderColor = 0xff4a4a45
		theme.textProcessor = .cjkSpacing
		theme.fontProfile = .hetiLikeHei
		return theme
	}()
}
