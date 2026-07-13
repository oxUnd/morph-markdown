package com.morph.markdown.demo

object MarkdownNative {
	init {
		System.loadLibrary("markdown-render-demo")
	}

	external fun snapshot(markdown: String): String?

	external fun renderLatex(
		fontPath: String,
		latex: String,
		display: Boolean,
		fontSizePx: Float
	): IntArray?
}
