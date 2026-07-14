package com.morph.markdown

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.view.View

class ListMarkerView(
	context: Context,
	private val style: MorphListMarkerStyle,
	private val theme: MorphMarkdownTheme
) : View(context) {
	private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
		color = 0xff1b1b1b.toInt()
		strokeWidth = context.resources.displayMetrics.density * 1.5f
	}

	override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
		setMeasuredDimension(context.dp(theme.listMarkerWidthDp), context.dp(theme.bodyTextSizeSp.toInt() + 8))
	}

	override fun onDraw(canvas: Canvas) {
		super.onDraw(canvas)
		when (style) {
			MorphListMarkerStyle.Disc -> drawDisc(canvas)
			MorphListMarkerStyle.Circle -> drawCircle(canvas)
			MorphListMarkerStyle.Square -> drawSquare(canvas)
			MorphListMarkerStyle.Hyphen -> drawHyphen(canvas)
		}
	}

	private fun drawDisc(canvas: Canvas) {
		paint.style = Paint.Style.FILL
		canvas.drawCircle(centerX(), centerY(), markerRadius(), paint)
	}

	private fun drawCircle(canvas: Canvas) {
		paint.style = Paint.Style.STROKE
		canvas.drawCircle(centerX(), centerY(), markerRadius(), paint)
	}

	private fun drawSquare(canvas: Canvas) {
		paint.style = Paint.Style.FILL
		val half = context.dp(theme.listMarkerSizeDp) / 2f
		canvas.drawRect(centerX() - half, centerY() - half, centerX() + half, centerY() + half, paint)
	}

	private fun drawHyphen(canvas: Canvas) {
		paint.style = Paint.Style.STROKE
		val half = context.dp(theme.listMarkerSizeDp) / 2f
		canvas.drawLine(centerX() - half, centerY(), centerX() + half, centerY(), paint)
	}

	private fun centerX(): Float {
		return context.dp(theme.listMarkerWidthDp) / 2f
	}

	private fun centerY(): Float {
		return measuredHeight / 2f
	}

	private fun markerRadius(): Float {
		return context.dp(theme.listMarkerSizeDp) / 2f
	}
}
