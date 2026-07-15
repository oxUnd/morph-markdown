package com.morph.markdown

data class MorphMarkdownOptions(
	val autoScrollOnAppend: Boolean = true,
	val appendRenderDebounceMs: Long = 160L
)
