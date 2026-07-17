package com.morph.markdown

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.view.View

class TaskMarkerView(
	context: Context,
	private val checked: Boolean,
	private val theme: MorphMarkdownTheme
) : View(context) {
	private val strokePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
		color = theme.bodyTextColor
		style = Paint.Style.STROKE
		strokeWidth = context.resources.displayMetrics.density * 2f
	}
	private val fillPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
		color = theme.bodyTextColor
		style = Paint.Style.FILL
	}
	private val checkPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
		color = theme.taskCheckColor
		style = Paint.Style.STROKE
		strokeCap = Paint.Cap.ROUND
		strokeJoin = Paint.Join.ROUND
		strokeWidth = context.resources.displayMetrics.density * 2.4f
	}

	override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
		val size = context.dp(theme.taskBoxSizeDp)
		setMeasuredDimension(size, size)
	}

	override fun onDraw(canvas: Canvas) {
		super.onDraw(canvas)
		drawBox(canvas)
		if (checked) drawCheck(canvas)
	}

	private fun drawBox(canvas: Canvas) {
		val inset = context.dp(2).toFloat()
		val right = measuredWidth - inset
		val bottom = measuredHeight - inset
		if (checked) {
			canvas.drawRoundRect(inset, inset, right, bottom, 2f, 2f, fillPaint)
		} else {
			canvas.drawRoundRect(inset, inset, right, bottom, 2f, 2f, strokePaint)
		}
	}

	private fun drawCheck(canvas: Canvas) {
		val w = measuredWidth.toFloat()
		val h = measuredHeight.toFloat()
		canvas.drawLine(w * 0.28f, h * 0.52f, w * 0.43f, h * 0.68f, checkPaint)
		canvas.drawLine(w * 0.43f, h * 0.68f, w * 0.74f, h * 0.34f, checkPaint)
	}
}
