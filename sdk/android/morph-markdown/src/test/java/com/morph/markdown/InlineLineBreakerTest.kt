package com.morph.markdown

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class InlineLineBreakerTest {
	@Test
	fun wrapsItemsWhenLineWidthOverflows() {
		val lines = InlineLineBreaker.breakLines(
			items = listOf(
				InlineItemSize(width = 40, height = 12),
				InlineItemSize(width = 45, height = 12),
				InlineItemSize(width = 30, height = 12)
			),
			maxWidth = 90,
			minLineHeight = 16
		)

		assertEquals(2, lines.size)
		assertEquals(0, lines[0].start)
		assertEquals(2, lines[0].end)
		assertEquals(16, lines[0].height)
		assertEquals(2, lines[1].start)
		assertEquals(3, lines[1].end)
	}

	@Test
	fun tallInlineMathOnlyExpandsItsOwnLine() {
		val lines = InlineLineBreaker.breakLines(
			items = listOf(
				InlineItemSize(width = 40, height = 18),
				InlineItemSize(width = 35, height = 42),
				InlineItemSize(width = 70, height = 18)
			),
			maxWidth = 90,
			minLineHeight = 20
		)

		assertEquals(2, lines.size)
		assertEquals(42, lines[0].height)
		assertEquals(20, lines[1].height)
	}

	@Test
	fun emptyInputProducesNoLines() {
		val lines = InlineLineBreaker.breakLines(emptyList(), maxWidth = 90, minLineHeight = 20)

		assertTrue(lines.isEmpty())
	}
}
