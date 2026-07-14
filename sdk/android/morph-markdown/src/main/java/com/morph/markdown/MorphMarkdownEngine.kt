package com.morph.markdown

import java.io.Closeable

class MorphMarkdownEngine : Closeable {
	private var handle: Long = MarkdownNative.createEngine()

	fun append(markdown: String, final: Boolean = false): Int {
		if (handle == 0L) return -1
		return MarkdownNative.append(handle, markdown, final)
	}

	fun snapshotJson(): String? {
		if (handle == 0L) return null
		return MarkdownNative.snapshotJson(handle)
	}

	override fun close() {
		if (handle != 0L) {
			MarkdownNative.destroyEngine(handle)
			handle = 0L
		}
	}
}
