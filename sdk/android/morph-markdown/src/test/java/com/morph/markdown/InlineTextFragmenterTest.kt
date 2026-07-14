package com.morph.markdown

import org.junit.Assert.assertEquals
import org.junit.Test

class InlineTextFragmenterTest {
	@Test
	fun splitsChineseIntoSingleCharacterFragments() {
		val fragments = InlineTextFragmenter.fragments("求根公式")

		assertEquals(listOf("求", "根", "公", "式"), fragments)
	}

	@Test
	fun keepsEnglishWordsTogetherAndSpacesSeparate() {
		val fragments = InlineTextFragmenter.fragments("Morph Markdown SDK")

		assertEquals(listOf("Morph", " ", "Markdown", " ", "SDK"), fragments)
	}

	@Test
	fun keepsPunctuationAsBreakableFragments() {
		val fragments = InlineTextFragmenter.fragments("Android/iOS。")

		assertEquals(listOf("Android", "/", "iOS", "。"), fragments)
	}
}
