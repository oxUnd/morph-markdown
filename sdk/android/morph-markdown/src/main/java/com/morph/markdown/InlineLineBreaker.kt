package com.morph.markdown

internal data class InlineItemSize(
	val width: Int,
	val height: Int
)

internal data class InlineLine(
	val start: Int,
	val end: Int,
	val width: Int,
	val height: Int
)

internal object InlineLineBreaker {
	fun breakLines(
		items: List<InlineItemSize>,
		maxWidth: Int,
		minLineHeight: Int
	): List<InlineLine> {
		if (items.isEmpty()) return emptyList()
		val lines = mutableListOf<InlineLine>()
		var lineStart = 0
		var lineWidth = 0
		var lineHeight = 0
		for (index in items.indices) {
			val item = items[index]
			if (shouldWrap(lineWidth, item.width, maxWidth)) {
				lines.add(line(lineStart, index, lineWidth, lineHeight, minLineHeight))
				lineStart = index
				lineWidth = 0
				lineHeight = 0
			}
			lineWidth += item.width
			lineHeight = maxOf(lineHeight, item.height)
		}
		lines.add(line(lineStart, items.size, lineWidth, lineHeight, minLineHeight))
		return lines
	}

	private fun shouldWrap(lineWidth: Int, itemWidth: Int, maxWidth: Int): Boolean {
		return lineWidth > 0 && lineWidth + itemWidth > maxWidth
	}

	private fun line(
		start: Int,
		end: Int,
		width: Int,
		height: Int,
		minLineHeight: Int
	): InlineLine {
		return InlineLine(start, end, width, maxOf(height, minLineHeight))
	}
}
