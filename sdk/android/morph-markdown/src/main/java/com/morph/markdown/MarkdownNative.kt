package com.morph.markdown

internal object MarkdownNative {
	init {
		System.loadLibrary("morph-markdown")
	}

	external fun createEngine(): Long

	external fun append(handle: Long, markdown: String, final: Boolean): Int

	external fun snapshotJson(handle: Long): String?

	external fun destroyEngine(handle: Long)

	external fun renderLatex(
		fontPath: String,
		latex: String,
		display: Boolean,
		fontSizePx: Float
	): IntArray?
}
