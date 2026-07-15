package com.morph.markdown

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.view.MotionEvent
import android.widget.TextView

internal class LinkTextView(context: Context) : TextView(context) {
	var drawLinkUnderline: Boolean = true
	var linkColor: Int = 0xff2f6f73.toInt()
	private val underlinePaint = Paint(Paint.ANTI_ALIAS_FLAG)
	private val normalBackground = 0x00000000
	private val pressedBackground = 0x1a2f6f73

	override fun onDraw(canvas: Canvas) {
		super.onDraw(canvas)
		if (!drawLinkUnderline) return
		underlinePaint.color = linkColor
		underlinePaint.strokeWidth = context.dpFloat(1f)
		val y = height - paddingBottom - context.dpFloat(1f)
		canvas.drawLine(0f, y, width.toFloat(), y, underlinePaint)
	}

	override fun onTouchEvent(event: MotionEvent): Boolean {
		when (event.actionMasked) {
			MotionEvent.ACTION_DOWN -> setBackgroundColor(pressedBackground)
			MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> setBackgroundColor(normalBackground)
		}
		return super.onTouchEvent(event)
	}
}
