package com.morph.markdown.demo

data class MarkdownRenderConfig(
	val bodyTextSizeSp: Float = 16f,
	val headingSizesSp: List<Float> = listOf(26f, 23f, 20f, 18f, 16f, 16f),
	val tabSize: Int = 4,
	val listIndentDp: Int = 20,
	val blockquoteIndentDp: Int = 12,
	val codeBlockTabSize: Int = 4,
	val codeBlockTextSizeSp: Float = 14f,
	val inlineCodeTextSizeSp: Float = 15f,
	val tableCellMaxWidthDp: Int = 280,
	val tableCellWrap: Boolean = true,
	val tableHorizontalScroll: Boolean = true,
	val imageMaxWidthDp: Int = 320,
	val imageMaxHeightDp: Int = 180,
	val mathTextSizeFollowsBody: Boolean = true,
	val mathTextSizeSp: Float = 16f
) {
	fun headingSize(level: Int): Float {
		return headingSizesSp[(level - 1).coerceIn(0, headingSizesSp.lastIndex)]
	}

	fun mathSize(): Float {
		return if (mathTextSizeFollowsBody) bodyTextSizeSp else mathTextSizeSp
	}
}

object MarkdownRenderPresets {
	val Normal = MarkdownRenderConfig()

	val LargeHeadings = Normal.copy(
		headingSizesSp = listOf(30f, 26f, 22f, 19f, 17f, 16f)
	)

	val CompactHeadings = Normal.copy(
		headingSizesSp = listOf(23f, 21f, 19f, 17f, 16f, 16f)
	)

	val CompactCode = Normal.copy(
		codeBlockTextSizeSp = 13f,
		inlineCodeTextSizeSp = 14f
	)

	fun withTabSize(size: Int): MarkdownRenderConfig {
		return Normal.copy(tabSize = size, codeBlockTabSize = size)
	}

	fun withTableWrap(wrap: Boolean): MarkdownRenderConfig {
		return Normal.copy(tableCellWrap = wrap)
	}
}
