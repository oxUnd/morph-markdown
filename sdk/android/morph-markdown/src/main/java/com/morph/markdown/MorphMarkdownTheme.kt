package com.morph.markdown

enum class MorphListMarkerStyle {
	Disc,
	Circle,
	Square,
	Hyphen
}

enum class MorphTextProcessor {
	None,
	CjkSpacing
}

enum class MorphFontProfile {
	System,
	HetiLikeHei,
	HetiLikeSong
}

data class MorphMarkdownTheme(
	val bodyTextSizeSp: Float = 16f,
	val headingSizesSp: List<Float> = listOf(26f, 23f, 20f, 18f, 16f, 16f),
	val headingLineHeightsSp: List<Float> = listOf(34f, 30f, 27f, 25f, 24f, 24f),
	val headingTopSpacingDp: Int = 8,
	val headingBottomSpacingDp: Int = 10,
	val paragraphTopSpacingDp: Int = 4,
	val paragraphBottomSpacingDp: Int = 10,
	val bodyLineHeightMultiplier: Float = 1.18f,
	val tabSize: Int = 4,
	val listIndentDp: Int = 20,
	val nestedListIndentDp: Int = 12,
	val listMarkerWidthDp: Int = 22,
	val listMarkerSizeDp: Int = 6,
	val listItemSpacingDp: Int = 2,
	val orderedMarkerWidthDp: Int = 28,
	val unorderedListMarkers: List<MorphListMarkerStyle> = listOf(
		MorphListMarkerStyle.Disc,
		MorphListMarkerStyle.Circle,
		MorphListMarkerStyle.Square
	),
	val taskBoxSizeDp: Int = 22,
	val taskBoxTextGapDp: Int = 8,
	val blockquoteIndentDp: Int = 12,
	val blockquoteVerticalPaddingDp: Int = 6,
	val blockquoteBottomSpacingDp: Int = 12,
	val codeBlockTabSize: Int = 4,
	val codeBlockTextSizeSp: Float = 14f,
	val inlineCodeTextSizeSp: Float = 15f,
	val codeBlockPaddingHorizontalDp: Int = 12,
	val codeBlockPaddingVerticalDp: Int = 10,
	val inlineCodePaddingHorizontalDp: Int = 5,
	val inlineCodePaddingVerticalDp: Int = 2,
	val tableCellMaxWidthDp: Int = 280,
	val tableCellWrap: Boolean = true,
	val tableHorizontalScroll: Boolean = true,
	val tableCellPaddingHorizontalDp: Int = 12,
	val tableCellPaddingVerticalDp: Int = 8,
	val tableTopSpacingDp: Int = 12,
	val tableBottomSpacingDp: Int = 14,
	val imageMaxWidthDp: Int = 320,
	val imageMaxHeightDp: Int = 180,
	val mathTextSizeFollowsBody: Boolean = true,
	val mathTextSizeSp: Float = 16f,
	val textProcessor: MorphTextProcessor = MorphTextProcessor.None,
	val fontProfile: MorphFontProfile = MorphFontProfile.System,
	val fontAssetPath: String? = null,
	val boldFontAssetPath: String? = null
) {
	fun headingSize(level: Int): Float {
		return headingSizesSp[(level - 1).coerceIn(0, headingSizesSp.lastIndex)]
	}

	fun headingLineHeight(level: Int): Float {
		return headingLineHeightsSp[(level - 1).coerceIn(0, headingLineHeightsSp.lastIndex)]
	}

	fun mathSize(): Float {
		return if (mathTextSizeFollowsBody) bodyTextSizeSp else mathTextSizeSp
	}
}

object MorphMarkdownThemes {
	val Normal = MorphMarkdownTheme()

	val LargeHeadings = Normal.copy(
		headingSizesSp = listOf(30f, 26f, 22f, 19f, 17f, 16f)
	)

	val CompactHeadings = Normal.copy(
		headingSizesSp = listOf(23f, 21f, 19f, 17f, 16f, 16f)
	)

	val HetiLike = Normal.copy(
		headingSizesSp = listOf(32f, 24f, 20f, 18f, 16f, 14f),
		headingLineHeightsSp = listOf(48f, 36f, 36f, 24f, 24f, 24f),
		headingTopSpacingDp = 20,
		headingBottomSpacingDp = 12,
		paragraphTopSpacingDp = 8,
		paragraphBottomSpacingDp = 20,
		bodyLineHeightMultiplier = 1.5f,
		listIndentDp = 32,
		nestedListIndentDp = 10,
		listItemSpacingDp = 2,
		codeBlockTextSizeSp = 14f,
		inlineCodeTextSizeSp = 14f,
		tableCellPaddingHorizontalDp = 8,
		tableCellPaddingVerticalDp = 6,
		tableTopSpacingDp = 12,
		tableBottomSpacingDp = 20,
		textProcessor = MorphTextProcessor.CjkSpacing,
		fontProfile = MorphFontProfile.HetiLikeHei
	)

	val HetiLikeHei = HetiLike.copy(
		headingSizesSp = listOf(32f, 24f, 20f, 18f, 16f, 14f),
		headingLineHeightsSp = listOf(48f, 36f, 36f, 24f, 24f, 24f),
		fontProfile = MorphFontProfile.HetiLikeHei
	)

	fun hetiLikeWithFont(fontAssetPath: String, boldFontAssetPath: String? = null): MorphMarkdownTheme {
		return HetiLike.copy(
			fontAssetPath = fontAssetPath,
			boldFontAssetPath = boldFontAssetPath,
			fontProfile = MorphFontProfile.HetiLikeSong
		)
	}
}
