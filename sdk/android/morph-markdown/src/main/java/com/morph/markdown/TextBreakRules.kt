package com.morph.markdown

internal object TextBreakRules {
	private const val EmergencyBreakRunLength = 32

	fun fragments(value: String): List<String> {
		if (value.isEmpty()) return emptyList()
		val out = mutableListOf<String>()
		var tokenStart = -1
		for (index in value.indices) {
			val ch = value[index]
			if (isNaturalBreak(ch)) {
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

	fun hasEmergencyBreak(value: String): Boolean {
		var runLength = 0
		for (ch in value) {
			if (isNaturalBreak(ch)) {
				runLength = 0
			} else {
				runLength += 1
				if (runLength > EmergencyBreakRunLength) return true
			}
		}
		return false
	}

	fun isNaturalBreak(ch: Char): Boolean {
		return ch.isWhitespace() || isCjk(ch) || isBreakPunctuation(ch)
	}

	private fun flushToken(value: String, start: Int, end: Int, out: MutableList<String>) {
		if (start < 0 || end <= start) return
		val token = value.substring(start, end)
		if (token.length <= EmergencyBreakRunLength) {
			out.add(token)
		} else {
			addCodePointFragments(token, out)
		}
	}

	private fun addCodePointFragments(token: String, out: MutableList<String>) {
		var index = 0
		while (index < token.length) {
			val next = token.offsetByCodePoints(index, 1)
			out.add(token.substring(index, next))
			index = next
		}
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
