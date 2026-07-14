package com.morph.markdown

import android.content.Context
import android.widget.HorizontalScrollView

internal class MarkdownTableScrollView(context: Context) : HorizontalScrollView(context) {
	override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
		tableChild()?.viewportWidthHint = viewportWidth(widthMeasureSpec)
		super.onMeasure(widthMeasureSpec, heightMeasureSpec)
	}

	private fun tableChild(): MarkdownTableView? {
		return getChildAt(0) as? MarkdownTableView
	}

	private fun viewportWidth(widthMeasureSpec: Int): Int {
		val width = MeasureSpec.getSize(widthMeasureSpec)
		return (width - paddingLeft - paddingRight).coerceAtLeast(0)
	}
}
