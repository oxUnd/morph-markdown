package com.morph.markdown

import android.content.Context
import android.view.ViewGroup

class InlineLayout(context: Context) : ViewGroup(context) {
	override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
		val maxWidth = MeasureSpec.getSize(widthMeasureSpec)
		var x = paddingLeft
		var y = paddingTop
		var lineHeight = 0
		for (i in 0 until childCount) {
			val child = getChildAt(i)
			measureChild(child, widthMeasureSpec, heightMeasureSpec)
			if (x + child.measuredWidth > maxWidth - paddingRight && x > paddingLeft) {
				x = paddingLeft
				y += lineHeight
				lineHeight = 0
			}
			x += child.measuredWidth
			lineHeight = maxOf(lineHeight, child.measuredHeight)
		}
		val height = y + lineHeight + paddingBottom
		setMeasuredDimension(maxWidth, resolveSize(height, heightMeasureSpec))
	}

	override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
		val maxWidth = r - l
		var x = paddingLeft
		var y = paddingTop
		var lineHeight = 0
		for (i in 0 until childCount) {
			val child = getChildAt(i)
			if (x + child.measuredWidth > maxWidth - paddingRight && x > paddingLeft) {
				x = paddingLeft
				y += lineHeight
				lineHeight = 0
			}
			val top = y + (lineHeight - child.measuredHeight).coerceAtLeast(0) / 2
			child.layout(x, top, x + child.measuredWidth, top + child.measuredHeight)
			x += child.measuredWidth
			lineHeight = maxOf(lineHeight, child.measuredHeight)
		}
	}
}
