package com.morph.markdown

import android.view.View
import android.widget.TextView
import kotlin.math.ceil

internal fun View.tableMinIntrinsicWidth(heightMeasureSpec: Int): Int {
	return when (this) {
		is InlineLayout -> inlineMinIntrinsicWidth(heightMeasureSpec)
		is TextView -> textMinIntrinsicWidth()
		else -> measuredNaturalWidth(heightMeasureSpec)
	}
}

internal fun View.tablePreferredIntrinsicWidth(heightMeasureSpec: Int): Int {
	val widthSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED)
	measure(widthSpec, heightMeasureSpec)
	return measuredWidth
}

private fun InlineLayout.inlineMinIntrinsicWidth(heightMeasureSpec: Int): Int {
	var maxChild = 0
	for (i in 0 until childCount) {
		maxChild = maxOf(maxChild, getChildAt(i).tableMinIntrinsicWidth(heightMeasureSpec))
	}
	return paddingLeft + maxChild + paddingRight
}

private fun TextView.textMinIntrinsicWidth(): Int {
	val contentWidth = longestUnbreakableWidth(text?.toString().orEmpty())
	return paddingLeft + contentWidth + paddingRight
}

private fun TextView.longestUnbreakableWidth(value: String): Int {
	var maxWidth = 0
	var start = -1
	for (index in value.indices) {
		val ch = value[index]
		if (isBreakable(ch)) {
			maxWidth = maxOf(maxWidth, tokenWidth(value, start, index), charWidth(ch))
			start = -1
		} else if (start < 0) {
			start = index
		}
	}
	return maxOf(maxWidth, tokenWidth(value, start, value.length))
}

private fun TextView.tokenWidth(value: String, start: Int, end: Int): Int {
	if (start < 0 || end <= start) return 0
	return ceil(paint.measureText(value, start, end).toDouble()).toInt()
}

private fun TextView.charWidth(ch: Char): Int {
	if (ch.isWhitespace()) return 0
	return ceil(paint.measureText(ch.toString()).toDouble()).toInt()
}

private fun View.measuredNaturalWidth(heightMeasureSpec: Int): Int {
	val params = layoutParams
	if (params != null && params.width > 0) return params.width
	val widthSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED)
	measure(widthSpec, heightMeasureSpec)
	return measuredWidth
}

private fun isBreakable(ch: Char): Boolean {
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
