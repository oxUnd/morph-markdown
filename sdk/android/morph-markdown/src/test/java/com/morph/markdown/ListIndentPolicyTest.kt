package com.morph.markdown

import org.junit.Assert.assertEquals
import org.junit.Test

class ListIndentPolicyTest {
	@Test
	fun defaultThemesAlignTopLevelMarkersWithContentEdge() {
		assertEquals(0, MorphMarkdownThemes.Normal.listIndentDp)
		assertEquals(0, MorphMarkdownThemes.HetiLike.listIndentDp)
	}

	@Test
	fun customIndentOnlyAppliesToTopLevelRows() {
		assertEquals(20, ListIndentPolicy.rowIndent(depth = 0, topLevelIndent = 20))
		assertEquals(0, ListIndentPolicy.rowIndent(depth = 1, topLevelIndent = 20))
		assertEquals(0, ListIndentPolicy.rowIndent(depth = 3, topLevelIndent = 20))
	}
}
