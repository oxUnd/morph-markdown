package com.morph.markdown

internal object InlineTextFragmenter {
	fun fragments(value: String): List<String> {
		return TextBreakRules.fragments(value)
	}
}
