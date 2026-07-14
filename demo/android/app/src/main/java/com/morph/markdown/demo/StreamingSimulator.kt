package com.morph.markdown.demo

import kotlin.math.min
import kotlin.random.Random

data class StreamChunk(
	val text: String,
	val delayMs: Long
)

object StreamingSimulator {
	private val random = Random(20260714)

	fun create(markdown: String): List<StreamChunk> {
		val out = mutableListOf<StreamChunk>()
		var index = 0
		while (index < markdown.length) {
			val next = nextEnd(markdown, index)
			val text = markdown.substring(index, next)
			out.add(StreamChunk(text, delayFor(text)))
			index = next
		}
		return out
	}

	private fun nextEnd(markdown: String, start: Int): Int {
		val target = start + nextTokenSize(markdown, start)
		val bounded = min(target, markdown.length)
		return extendToNaturalBoundary(markdown, start, bounded)
	}

	private fun nextTokenSize(markdown: String, start: Int): Int {
		val ch = markdown[start]
		if (ch == '\n') return if (markdown.getOrNull(start + 1) == '\n') 2 else 1
		return when (random.nextInt(100)) {
			in 0..52 -> random.nextInt(1, 5)
			in 53..82 -> random.nextInt(5, 11)
			in 83..94 -> random.nextInt(11, 20)
			else -> random.nextInt(20, 36)
		}
	}

	private fun extendToNaturalBoundary(markdown: String, start: Int, end: Int): Int {
		if (end >= markdown.length) return markdown.length
		if (end == start) return min(start + 1, markdown.length)
		if (markdown[end - 1].isWhitespace()) return end
		val next = markdown[end]
		if (next.isWhitespace() || isPunctuation(next)) return end + 1
		return end
	}

	private fun delayFor(text: String): Long {
		val base = when {
			text.contains("\n\n") -> random.nextLong(460, 920)
			text.any { it == '\n' } -> random.nextLong(220, 520)
			text.any { isStrongPunctuation(it) } -> random.nextLong(180, 420)
			text.length <= 3 -> random.nextLong(24, 90)
			text.length <= 10 -> random.nextLong(45, 140)
			else -> random.nextLong(80, 210)
		}
		return withOccasionalPause(base)
	}

	private fun withOccasionalPause(base: Long): Long {
		return when (random.nextInt(100)) {
			in 0..4 -> base + random.nextLong(420, 950)
			in 5..16 -> base + random.nextLong(120, 360)
			else -> base
		}
	}

	private fun isPunctuation(ch: Char): Boolean {
		return ch in punctuation
	}

	private fun isStrongPunctuation(ch: Char): Boolean {
		return ch in strongPunctuation
	}

	private val punctuation = setOf(
		'.', ',', ':', ';', '!', '?', ')', ']', '}', '>', '|',
		'。', '，', '：', '；', '！', '？', '）', '】', '》', '、'
	)

	private val strongPunctuation = setOf(
		'.', '!', '?', ';', '。', '！', '？', '；'
	)
}
