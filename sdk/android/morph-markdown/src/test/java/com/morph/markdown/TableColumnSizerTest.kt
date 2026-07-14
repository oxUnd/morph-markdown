package com.morph.markdown

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class TableColumnSizerTest {
	@Test
	fun wrapsColumnsIntoAvailableWidthWhenPreferredIsTooWide() {
		val widths = TableColumnSizer.sizeColumns(
			cells = listOf(
				TableCellWidth(column = 0, minWidth = 80, preferredWidth = 220),
				TableCellWidth(column = 1, minWidth = 90, preferredWidth = 260)
			),
			columnCount = 2,
			availableWidth = 360,
			maxColumnWidth = 280,
			wrap = true
		)

		assertEquals(360, widths.sum())
		assertTrue(widths[0] >= 80)
		assertTrue(widths[1] >= 90)
	}

	@Test
	fun keepsMinimumWidthsWhenUnbreakableContentExceedsAvailableWidth() {
		val widths = TableColumnSizer.sizeColumns(
			cells = listOf(
				TableCellWidth(column = 0, minWidth = 260, preferredWidth = 320),
				TableCellWidth(column = 1, minWidth = 220, preferredWidth = 260)
			),
			columnCount = 2,
			availableWidth = 360,
			maxColumnWidth = 280,
			wrap = true
		)

		assertEquals(listOf(260, 220), widths)
		assertTrue(widths.sum() > 360)
	}

	@Test
	fun growsPreferredColumnsToUseAvailableWidth() {
		val widths = TableColumnSizer.sizeColumns(
			cells = listOf(
				TableCellWidth(column = 0, minWidth = 40, preferredWidth = 80),
				TableCellWidth(column = 1, minWidth = 60, preferredWidth = 120)
			),
			columnCount = 2,
			availableWidth = 300,
			maxColumnWidth = 280,
			wrap = true
		)

		assertEquals(300, widths.sum())
		assertTrue(widths[1] > widths[0])
	}

	@Test
	fun leavesNaturalWidthsWhenWrappingIsDisabled() {
		val widths = TableColumnSizer.sizeColumns(
			cells = listOf(
				TableCellWidth(column = 0, minWidth = 40, preferredWidth = 420),
				TableCellWidth(column = 1, minWidth = 60, preferredWidth = 380)
			),
			columnCount = 2,
			availableWidth = 300,
			maxColumnWidth = 280,
			wrap = false
		)

		assertEquals(listOf(420, 380), widths)
	}
}
