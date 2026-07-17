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
	for (fragment in TextBreakRules.fragments(value)) {
		maxWidth = maxOf(maxWidth, fragmentWidth(fragment))
	}
	return maxWidth
}

private fun TextView.fragmentWidth(fragment: String): Int {
	if (fragment.isBlank()) return 0
	return ceil(paint.measureText(fragment).toDouble()).toInt()
}

private fun View.measuredNaturalWidth(heightMeasureSpec: Int): Int {
	val params = layoutParams
	if (params != null && params.width > 0) return params.width
	val widthSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED)
	measure(widthSpec, heightMeasureSpec)
	return measuredWidth
}
