package com.morph.markdown

import android.content.Context
import android.view.View
import android.view.ViewGroup

class InlineLayout(context: Context) : ViewGroup(context) {
	var lineSpacingPx: Int = 0
	var minLineHeightPx: Int = 0
	private var measuredLines: List<InlineLine> = emptyList()

	override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
		val mode = MeasureSpec.getMode(widthMeasureSpec)
		val maxWidth = measuredWidthLimit(widthMeasureSpec)
		val available = contentWidthLimit(maxWidth)
		for (i in 0 until childCount) {
			measureInlineChild(getChildAt(i), available, heightMeasureSpec)
		}
		measuredLines = InlineLineBreaker.breakLines(childSizes(), available, minLineHeightPx)
		val desiredWidth = paddingLeft + contentWidth() + paddingRight
		val height = paddingTop + contentHeight() + paddingBottom
		setMeasuredDimension(resolveWidth(desiredWidth, widthMeasureSpec, mode), resolveSize(height, heightMeasureSpec))
	}

	override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
		var y = paddingTop
		for (line in measuredLines) {
			layoutLine(line, y)
			y += line.height + lineSpacingPx
		}
	}

	private fun measureInlineChild(child: View, available: Int, heightMeasureSpec: Int) {
		val widthSpec = MeasureSpec.makeMeasureSpec(available, MeasureSpec.AT_MOST)
		measureChild(child, widthSpec, heightMeasureSpec)
	}

	private fun childSizes(): List<InlineItemSize> {
		return (0 until childCount).map { index ->
			val child = getChildAt(index)
			InlineItemSize(child.measuredWidth, child.measuredHeight)
		}
	}

	private fun contentHeight(): Int {
		if (measuredLines.isEmpty()) return 0
		return measuredLines.sumOf { it.height } + lineSpacingPx * (measuredLines.size - 1)
	}

	private fun contentWidth(): Int {
		return measuredLines.maxOfOrNull { it.width } ?: 0
	}

	private fun contentWidthLimit(maxWidth: Int): Int {
		return (maxWidth - paddingLeft - paddingRight).coerceAtLeast(0)
	}

	private fun measuredWidthLimit(widthMeasureSpec: Int): Int {
		if (MeasureSpec.getMode(widthMeasureSpec) == MeasureSpec.UNSPECIFIED) return Int.MAX_VALUE / 4
		return MeasureSpec.getSize(widthMeasureSpec)
	}

	private fun resolveWidth(desired: Int, widthMeasureSpec: Int, mode: Int): Int {
		val size = MeasureSpec.getSize(widthMeasureSpec)
		return when (mode) {
			MeasureSpec.EXACTLY -> size
			MeasureSpec.AT_MOST -> desired.coerceAtMost(size)
			else -> desired
		}
	}

	private fun layoutLine(line: InlineLine, top: Int) {
		var x = paddingLeft
		for (index in line.start until line.end) {
			val child = getChildAt(index)
			val childTop = top + (line.height - child.measuredHeight).coerceAtLeast(0) / 2
			child.layout(x, childTop, x + child.measuredWidth, childTop + child.measuredHeight)
			x += child.measuredWidth
		}
	}
}
