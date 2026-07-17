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

	@Test
	fun breaksVeryLongDigitRunsIntoEmergencyFragments() {
		val value = "1".repeat(80)
		val fragments = InlineTextFragmenter.fragments(value)

		assertEquals(80, fragments.size)
		assertEquals(listOf("1", "1", "1"), fragments.take(3))
	}

	@Test
	fun keepsModerateAsciiTokensTogether() {
		val fragments = InlineTextFragmenter.fragments("abcdefghij0123456789")

		assertEquals(listOf("abcdefghij0123456789"), fragments)
	}

	@Test
	fun breaksVeryLongAsciiRunsIntoEmergencyFragments() {
		val value = "abcdefghijklmnopqrstuvwxyz0123456789"
		val fragments = InlineTextFragmenter.fragments(value)

		assertEquals(value.length, fragments.size)
		assertEquals("a", fragments.first())
		assertEquals("9", fragments.last())
	}
}
