package com.morph.markdown

internal object ListIndentPolicy {
	fun rowIndent(depth: Int, topLevelIndent: Int): Int {
		return if (depth == 0) topLevelIndent else 0
	}
}
