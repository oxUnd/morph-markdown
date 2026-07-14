package com.morph.markdown

internal object InlineTextFragmenter {
	fun fragments(value: String): List<String> {
		if (value.isEmpty()) return emptyList()
		val out = mutableListOf<String>()
		var tokenStart = -1
		for (index in value.indices) {
			val ch = value[index]
			if (isStandalone(ch)) {
				flushToken(value, tokenStart, index, out)
				tokenStart = -1
				out.add(ch.toString())
			} else if (tokenStart < 0) {
				tokenStart = index
			}
		}
		flushToken(value, tokenStart, value.length, out)
		return out.filter { it.isNotEmpty() }
	}

	private fun flushToken(value: String, start: Int, end: Int, out: MutableList<String>) {
		if (start >= 0 && end > start) out.add(value.substring(start, end))
	}

	private fun isStandalone(ch: Char): Boolean {
		return ch.isWhitespace() || isCjk(ch) || isBreakPunctuation(ch)
	}

	private fun isCjk(ch: Char): Boolean {
		val block = Character.UnicodeBlock.of(ch)
		return block == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS ||
			block == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A ||
			block == Character.UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS ||
			block == Character.UnicodeBlock.HIRAGANA ||
			block == Character.UnicodeBlock.KATAKANA ||
			block == Character.UnicodeBlock.HANGUL_SYLLABLES
	}

	private fun isBreakPunctuation(ch: Char): Boolean {
		return ch in setOf(',', '.', ';', ':', '，', '。', '；', '：', '/', '-', '_')
	}
}
