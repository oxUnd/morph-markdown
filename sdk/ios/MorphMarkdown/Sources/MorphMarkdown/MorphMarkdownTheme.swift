import SwiftUI

public enum MorphListMarkerStyle: Sendable {
	case disc
	case circle
	case square
	case hyphen
}

public enum MorphTextProcessor: Sendable {
	case none
	case cjkSpacing
}

public enum MorphFontProfile: Sendable {
	case system
	case hetiLikeHei
	case hetiLikeSong
}

public struct MorphTableStyle: Sendable {
	public var borderColor: Color
	public var borderWidthDp: CGFloat
	public var headerBackgroundColor: Color
	public var bodyBackgroundColor: Color
	public var headerTextColor: Color
	public var bodyTextColor: Color
	public var headerTextSizeSp: CGFloat?
	public var bodyTextSizeSp: CGFloat?
	public var headerBold: Bool

	public init(
		borderColor: Color = Color(red: 0.27, green: 0.27, blue: 0.27),
		borderWidthDp: CGFloat = 1,
		headerBackgroundColor: Color = Color(red: 0.94, green: 0.94, blue: 0.92),
		bodyBackgroundColor: Color = .clear,
		headerTextColor: Color = Color(red: 0.11, green: 0.11, blue: 0.11),
		bodyTextColor: Color = Color(red: 0.11, green: 0.11, blue: 0.11),
		headerTextSizeSp: CGFloat? = nil,
		bodyTextSizeSp: CGFloat? = nil,
		headerBold: Bool = false
	) {
		self.borderColor = borderColor
		self.borderWidthDp = borderWidthDp
		self.headerBackgroundColor = headerBackgroundColor
		self.bodyBackgroundColor = bodyBackgroundColor
		self.headerTextColor = headerTextColor
		self.bodyTextColor = bodyTextColor
		self.headerTextSizeSp = headerTextSizeSp
		self.bodyTextSizeSp = bodyTextSizeSp
		self.headerBold = headerBold
	}
}

public struct MorphMarkdownTheme: Sendable {
	public var bodyTextSizeSp: CGFloat
	public var headingSizesSp: [CGFloat]
	public var headingLineHeightsSp: [CGFloat]
	public var headingTopSpacingDp: CGFloat
	public var headingBottomSpacingDp: CGFloat
	public var paragraphTopSpacingDp: CGFloat
	public var paragraphBottomSpacingDp: CGFloat
	public var bodyLineHeightMultiplier: CGFloat
	public var tabSize: Int
	public var listIndentDp: CGFloat
	public var nestedListIndentDp: CGFloat
	public var listMarkerWidthDp: CGFloat
	public var listMarkerSizeDp: CGFloat
	public var listItemSpacingDp: CGFloat
	public var orderedMarkerWidthDp: CGFloat
	public var unorderedListMarkers: [MorphListMarkerStyle]
	public var taskBoxSizeDp: CGFloat
	public var taskBoxTextGapDp: CGFloat
	public var blockquoteIndentDp: CGFloat
	public var blockquoteVerticalPaddingDp: CGFloat
	public var blockquoteBottomSpacingDp: CGFloat
	public var codeBlockTabSize: Int
	public var codeBlockTextSizeSp: CGFloat
	public var inlineCodeTextSizeSp: CGFloat
	public var codeBlockPaddingHorizontalDp: CGFloat
	public var codeBlockPaddingVerticalDp: CGFloat
	public var inlineCodePaddingHorizontalDp: CGFloat
	public var inlineCodePaddingVerticalDp: CGFloat
	public var tableCellMaxWidthDp: CGFloat
	public var tableCellWrap: Bool
	public var tableHorizontalScroll: Bool
	public var tableCellPaddingHorizontalDp: CGFloat
	public var tableCellPaddingVerticalDp: CGFloat
	public var tableTopSpacingDp: CGFloat
	public var tableBottomSpacingDp: CGFloat
	public var tableCellLineHeightMultiplier: CGFloat
	public var tableCellLineSpacingDp: CGFloat
	public var tableMathTextScale: CGFloat
	public var tableStyle: MorphTableStyle
	public var imageMaxWidthDp: CGFloat
	public var imageMaxHeightDp: CGFloat
	public var mathTextSizeFollowsBody: Bool
	public var mathTextSizeSp: CGFloat
	public var mathTextScale: CGFloat
	public var textProcessor: MorphTextProcessor
	public var fontProfile: MorphFontProfile
	public var fontAssetPath: String?
	public var boldFontAssetPath: String?

	public init(
		bodyTextSizeSp: CGFloat = 16,
		headingSizesSp: [CGFloat] = [26, 23, 20, 18, 16, 16],
		headingLineHeightsSp: [CGFloat] = [34, 30, 27, 25, 24, 24],
		headingTopSpacingDp: CGFloat = 8,
		headingBottomSpacingDp: CGFloat = 10,
		paragraphTopSpacingDp: CGFloat = 4,
		paragraphBottomSpacingDp: CGFloat = 10,
		bodyLineHeightMultiplier: CGFloat = 1.18,
		tabSize: Int = 4,
		listIndentDp: CGFloat = 20,
		nestedListIndentDp: CGFloat = 12,
		listMarkerWidthDp: CGFloat = 22,
		listMarkerSizeDp: CGFloat = 6,
		listItemSpacingDp: CGFloat = 2,
		orderedMarkerWidthDp: CGFloat = 28,
		unorderedListMarkers: [MorphListMarkerStyle] = [.disc, .circle, .square],
		taskBoxSizeDp: CGFloat = 22,
		taskBoxTextGapDp: CGFloat = 8,
		blockquoteIndentDp: CGFloat = 12,
		blockquoteVerticalPaddingDp: CGFloat = 6,
		blockquoteBottomSpacingDp: CGFloat = 12,
		codeBlockTabSize: Int = 4,
		codeBlockTextSizeSp: CGFloat = 14,
		inlineCodeTextSizeSp: CGFloat = 15,
		codeBlockPaddingHorizontalDp: CGFloat = 12,
		codeBlockPaddingVerticalDp: CGFloat = 10,
		inlineCodePaddingHorizontalDp: CGFloat = 5,
		inlineCodePaddingVerticalDp: CGFloat = 2,
		tableCellMaxWidthDp: CGFloat = 280,
		tableCellWrap: Bool = true,
		tableHorizontalScroll: Bool = true,
		tableCellPaddingHorizontalDp: CGFloat = 12,
		tableCellPaddingVerticalDp: CGFloat = 8,
		tableTopSpacingDp: CGFloat = 12,
		tableBottomSpacingDp: CGFloat = 14,
		tableCellLineHeightMultiplier: CGFloat = 1.25,
		tableCellLineSpacingDp: CGFloat = 0,
		tableMathTextScale: CGFloat = 1,
		tableStyle: MorphTableStyle = MorphTableStyle(),
		imageMaxWidthDp: CGFloat = 320,
		imageMaxHeightDp: CGFloat = 180,
		mathTextSizeFollowsBody: Bool = true,
		mathTextSizeSp: CGFloat = 16,
		mathTextScale: CGFloat = 1.18,
		textProcessor: MorphTextProcessor = .none,
		fontProfile: MorphFontProfile = .system,
		fontAssetPath: String? = nil,
		boldFontAssetPath: String? = nil
	) {
		self.bodyTextSizeSp = bodyTextSizeSp
		self.headingSizesSp = headingSizesSp
		self.headingLineHeightsSp = headingLineHeightsSp
		self.headingTopSpacingDp = headingTopSpacingDp
		self.headingBottomSpacingDp = headingBottomSpacingDp
		self.paragraphTopSpacingDp = paragraphTopSpacingDp
		self.paragraphBottomSpacingDp = paragraphBottomSpacingDp
		self.bodyLineHeightMultiplier = bodyLineHeightMultiplier
		self.tabSize = tabSize
		self.listIndentDp = listIndentDp
		self.nestedListIndentDp = nestedListIndentDp
		self.listMarkerWidthDp = listMarkerWidthDp
		self.listMarkerSizeDp = listMarkerSizeDp
		self.listItemSpacingDp = listItemSpacingDp
		self.orderedMarkerWidthDp = orderedMarkerWidthDp
		self.unorderedListMarkers = unorderedListMarkers
		self.taskBoxSizeDp = taskBoxSizeDp
		self.taskBoxTextGapDp = taskBoxTextGapDp
		self.blockquoteIndentDp = blockquoteIndentDp
		self.blockquoteVerticalPaddingDp = blockquoteVerticalPaddingDp
		self.blockquoteBottomSpacingDp = blockquoteBottomSpacingDp
		self.codeBlockTabSize = codeBlockTabSize
		self.codeBlockTextSizeSp = codeBlockTextSizeSp
		self.inlineCodeTextSizeSp = inlineCodeTextSizeSp
		self.codeBlockPaddingHorizontalDp = codeBlockPaddingHorizontalDp
		self.codeBlockPaddingVerticalDp = codeBlockPaddingVerticalDp
		self.inlineCodePaddingHorizontalDp = inlineCodePaddingHorizontalDp
		self.inlineCodePaddingVerticalDp = inlineCodePaddingVerticalDp
		self.tableCellMaxWidthDp = tableCellMaxWidthDp
		self.tableCellWrap = tableCellWrap
		self.tableHorizontalScroll = tableHorizontalScroll
		self.tableCellPaddingHorizontalDp = tableCellPaddingHorizontalDp
		self.tableCellPaddingVerticalDp = tableCellPaddingVerticalDp
		self.tableTopSpacingDp = tableTopSpacingDp
		self.tableBottomSpacingDp = tableBottomSpacingDp
		self.tableCellLineHeightMultiplier = tableCellLineHeightMultiplier
		self.tableCellLineSpacingDp = tableCellLineSpacingDp
		self.tableMathTextScale = tableMathTextScale
		self.tableStyle = tableStyle
		self.imageMaxWidthDp = imageMaxWidthDp
		self.imageMaxHeightDp = imageMaxHeightDp
		self.mathTextSizeFollowsBody = mathTextSizeFollowsBody
		self.mathTextSizeSp = mathTextSizeSp
		self.mathTextScale = mathTextScale
		self.textProcessor = textProcessor
		self.fontProfile = fontProfile
		self.fontAssetPath = fontAssetPath
		self.boldFontAssetPath = boldFontAssetPath
	}

	public func headingSize(_ level: Int) -> CGFloat {
		headingSizesSp[safeHeadingIndex(level, count: headingSizesSp.count)]
	}

	public func headingLineHeight(_ level: Int) -> CGFloat {
		headingLineHeightsSp[safeHeadingIndex(level, count: headingLineHeightsSp.count)]
	}

	public func mathSize() -> CGFloat {
		(mathTextSizeFollowsBody ? bodyTextSizeSp : mathTextSizeSp) * mathTextScale
	}

	public func bodyFont(bold: Bool = false) -> Font {
		switch fontProfile {
		case .hetiLikeHei:
			return .custom("PingFang SC", size: bodyTextSizeSp, relativeTo: .body)
		case .hetiLikeSong:
			return .custom("Songti SC", size: bodyTextSizeSp, relativeTo: .body)
		case .system:
			return .system(size: bodyTextSizeSp, weight: bold ? .semibold : .regular)
		}
	}

	public func headingFont(level: Int) -> Font {
		let size = headingSize(level)
		switch fontProfile {
		case .hetiLikeHei:
			return .custom("PingFang SC", size: size, relativeTo: .title)
		case .hetiLikeSong:
			return .custom("Songti SC", size: size, relativeTo: .title)
		case .system:
			return .system(size: size, weight: .semibold)
		}
	}

	public var codeFont: Font {
		.system(size: codeBlockTextSizeSp, design: .monospaced)
	}

	public var inlineCodeFont: Font {
		.system(size: inlineCodeTextSizeSp, design: .monospaced)
	}

	public var imageMaxSize: CGSize {
		CGSize(width: imageMaxWidthDp, height: imageMaxHeightDp)
	}

	private func safeHeadingIndex(_ level: Int, count: Int) -> Int {
		min(max(level - 1, 0), max(count - 1, 0))
	}
}

public enum MorphMarkdownThemes {
	public static let normal = MorphMarkdownTheme()
	public static let Normal = normal

	public static let largeHeadings = normal.copy(
		headingSizesSp: [30, 26, 22, 19, 17, 16]
	)
	public static let LargeHeadings = largeHeadings

	public static let compactHeadings = normal.copy(
		headingSizesSp: [23, 21, 19, 17, 16, 16]
	)
	public static let CompactHeadings = compactHeadings

	public static let hetiLike = normal.copy(
		headingSizesSp: [32, 24, 20, 18, 16, 14],
		headingLineHeightsSp: [48, 36, 36, 24, 24, 24],
		headingTopSpacingDp: 20,
		headingBottomSpacingDp: 12,
		paragraphTopSpacingDp: 8,
		paragraphBottomSpacingDp: 20,
		bodyLineHeightMultiplier: 1.5,
		listIndentDp: 32,
		nestedListIndentDp: 10,
		codeBlockTextSizeSp: 14,
		inlineCodeTextSizeSp: 14,
		tableCellPaddingHorizontalDp: 8,
		tableCellPaddingVerticalDp: 6,
		tableTopSpacingDp: 12,
		tableBottomSpacingDp: 20,
		tableCellLineHeightMultiplier: 1.35,
		tableMathTextScale: 1,
		tableStyle: MorphTableStyle(
			borderColor: Color(red: 0.29, green: 0.29, blue: 0.27),
			headerBackgroundColor: Color(red: 0.95, green: 0.95, blue: 0.93)
		),
		textProcessor: .cjkSpacing,
		fontProfile: .hetiLikeHei
	)
	public static let HetiLike = hetiLike

	public static let hetiLikeHei = hetiLike.copy(
		fontProfile: .hetiLikeHei
	)
	public static let HetiLikeHei = hetiLikeHei

	public static func hetiLikeWithFont(
		fontAssetPath: String,
		boldFontAssetPath: String? = nil
	) -> MorphMarkdownTheme {
		hetiLike.copy(
			fontProfile: .hetiLikeSong,
			fontAssetPath: fontAssetPath,
			boldFontAssetPath: boldFontAssetPath
		)
	}
}

public extension MorphMarkdownTheme {
	func copy(
		bodyTextSizeSp: CGFloat? = nil,
		headingSizesSp: [CGFloat]? = nil,
		headingLineHeightsSp: [CGFloat]? = nil,
		headingTopSpacingDp: CGFloat? = nil,
		headingBottomSpacingDp: CGFloat? = nil,
		paragraphTopSpacingDp: CGFloat? = nil,
		paragraphBottomSpacingDp: CGFloat? = nil,
		bodyLineHeightMultiplier: CGFloat? = nil,
		tabSize: Int? = nil,
		listIndentDp: CGFloat? = nil,
		nestedListIndentDp: CGFloat? = nil,
		listMarkerWidthDp: CGFloat? = nil,
		listMarkerSizeDp: CGFloat? = nil,
		listItemSpacingDp: CGFloat? = nil,
		orderedMarkerWidthDp: CGFloat? = nil,
		unorderedListMarkers: [MorphListMarkerStyle]? = nil,
		taskBoxSizeDp: CGFloat? = nil,
		taskBoxTextGapDp: CGFloat? = nil,
		blockquoteIndentDp: CGFloat? = nil,
		blockquoteVerticalPaddingDp: CGFloat? = nil,
		blockquoteBottomSpacingDp: CGFloat? = nil,
		codeBlockTabSize: Int? = nil,
		codeBlockTextSizeSp: CGFloat? = nil,
		inlineCodeTextSizeSp: CGFloat? = nil,
		tableCellMaxWidthDp: CGFloat? = nil,
		tableCellWrap: Bool? = nil,
		tableHorizontalScroll: Bool? = nil,
		tableCellPaddingHorizontalDp: CGFloat? = nil,
		tableCellPaddingVerticalDp: CGFloat? = nil,
		tableTopSpacingDp: CGFloat? = nil,
		tableBottomSpacingDp: CGFloat? = nil,
		tableCellLineHeightMultiplier: CGFloat? = nil,
		tableCellLineSpacingDp: CGFloat? = nil,
		tableMathTextScale: CGFloat? = nil,
		tableStyle: MorphTableStyle? = nil,
		imageMaxWidthDp: CGFloat? = nil,
		imageMaxHeightDp: CGFloat? = nil,
		mathTextSizeFollowsBody: Bool? = nil,
		mathTextSizeSp: CGFloat? = nil,
		mathTextScale: CGFloat? = nil,
		textProcessor: MorphTextProcessor? = nil,
		fontProfile: MorphFontProfile? = nil,
		fontAssetPath: String? = nil,
		boldFontAssetPath: String? = nil
	) -> MorphMarkdownTheme {
		MorphMarkdownTheme(
			bodyTextSizeSp: bodyTextSizeSp ?? self.bodyTextSizeSp,
			headingSizesSp: headingSizesSp ?? self.headingSizesSp,
			headingLineHeightsSp: headingLineHeightsSp ?? self.headingLineHeightsSp,
			headingTopSpacingDp: headingTopSpacingDp ?? self.headingTopSpacingDp,
			headingBottomSpacingDp: headingBottomSpacingDp ?? self.headingBottomSpacingDp,
			paragraphTopSpacingDp: paragraphTopSpacingDp ?? self.paragraphTopSpacingDp,
			paragraphBottomSpacingDp: paragraphBottomSpacingDp ?? self.paragraphBottomSpacingDp,
			bodyLineHeightMultiplier: bodyLineHeightMultiplier ?? self.bodyLineHeightMultiplier,
			tabSize: tabSize ?? self.tabSize,
			listIndentDp: listIndentDp ?? self.listIndentDp,
			nestedListIndentDp: nestedListIndentDp ?? self.nestedListIndentDp,
			listMarkerWidthDp: listMarkerWidthDp ?? self.listMarkerWidthDp,
			listMarkerSizeDp: listMarkerSizeDp ?? self.listMarkerSizeDp,
			listItemSpacingDp: listItemSpacingDp ?? self.listItemSpacingDp,
			orderedMarkerWidthDp: orderedMarkerWidthDp ?? self.orderedMarkerWidthDp,
			unorderedListMarkers: unorderedListMarkers ?? self.unorderedListMarkers,
			taskBoxSizeDp: taskBoxSizeDp ?? self.taskBoxSizeDp,
			taskBoxTextGapDp: taskBoxTextGapDp ?? self.taskBoxTextGapDp,
			blockquoteIndentDp: blockquoteIndentDp ?? self.blockquoteIndentDp,
			blockquoteVerticalPaddingDp: blockquoteVerticalPaddingDp ?? self.blockquoteVerticalPaddingDp,
			blockquoteBottomSpacingDp: blockquoteBottomSpacingDp ?? self.blockquoteBottomSpacingDp,
			codeBlockTabSize: codeBlockTabSize ?? self.codeBlockTabSize,
			codeBlockTextSizeSp: codeBlockTextSizeSp ?? self.codeBlockTextSizeSp,
			inlineCodeTextSizeSp: inlineCodeTextSizeSp ?? self.inlineCodeTextSizeSp,
			tableCellMaxWidthDp: tableCellMaxWidthDp ?? self.tableCellMaxWidthDp,
			tableCellWrap: tableCellWrap ?? self.tableCellWrap,
			tableHorizontalScroll: tableHorizontalScroll ?? self.tableHorizontalScroll,
			tableCellPaddingHorizontalDp: tableCellPaddingHorizontalDp ?? self.tableCellPaddingHorizontalDp,
			tableCellPaddingVerticalDp: tableCellPaddingVerticalDp ?? self.tableCellPaddingVerticalDp,
			tableTopSpacingDp: tableTopSpacingDp ?? self.tableTopSpacingDp,
			tableBottomSpacingDp: tableBottomSpacingDp ?? self.tableBottomSpacingDp,
			tableCellLineHeightMultiplier: tableCellLineHeightMultiplier ?? self.tableCellLineHeightMultiplier,
			tableCellLineSpacingDp: tableCellLineSpacingDp ?? self.tableCellLineSpacingDp,
			tableMathTextScale: tableMathTextScale ?? self.tableMathTextScale,
			tableStyle: tableStyle ?? self.tableStyle,
			imageMaxWidthDp: imageMaxWidthDp ?? self.imageMaxWidthDp,
			imageMaxHeightDp: imageMaxHeightDp ?? self.imageMaxHeightDp,
			mathTextSizeFollowsBody: mathTextSizeFollowsBody ?? self.mathTextSizeFollowsBody,
			mathTextSizeSp: mathTextSizeSp ?? self.mathTextSizeSp,
			mathTextScale: mathTextScale ?? self.mathTextScale,
			textProcessor: textProcessor ?? self.textProcessor,
			fontProfile: fontProfile ?? self.fontProfile,
			fontAssetPath: fontAssetPath ?? self.fontAssetPath,
			boldFontAssetPath: boldFontAssetPath ?? self.boldFontAssetPath
		)
	}
}
