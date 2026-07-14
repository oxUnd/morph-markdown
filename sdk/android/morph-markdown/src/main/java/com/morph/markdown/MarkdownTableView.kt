package com.morph.markdown

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.view.View
import android.view.ViewGroup

class MarkdownTableView(
	context: Context,
	private val theme: MorphMarkdownTheme
) : ViewGroup(context) {
	private data class Cell(val row: Int, val col: Int, val header: Boolean, val view: View)

	private val cells = mutableListOf<Cell>()
	private val colWidths = mutableListOf<Int>()
	private val rowHeights = mutableListOf<Int>()
	private val rowHeader = mutableListOf<Boolean>()
	private val linePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
		color = 0xff454545.toInt()
		strokeWidth = context.resources.displayMetrics.density * 1.5f
	}
	private val headerPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
		color = 0xffefefea.toInt()
		style = Paint.Style.FILL
	}
	private var currentRow = -1
	private var currentCol = 0

	fun beginRow(header: Boolean) {
		currentRow += 1
		currentCol = 0
		rowHeader.add(header)
	}

	fun addCell(view: View) {
		cells.add(Cell(currentRow, currentCol, rowHeader[currentRow], view))
		addView(view)
		currentCol += 1
	}

	override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
		resetMeasures()
		for (cell in cells) {
			measureCell(cell, heightMeasureSpec)
		}
		setMeasuredDimension(
			resolveSize(colWidths.sum(), widthMeasureSpec),
			resolveSize(rowHeights.sum(), heightMeasureSpec)
		)
	}

	override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
		val rowTops = offsets(rowHeights)
		val colLefts = offsets(colWidths)
		for (cell in cells) {
			layoutCell(cell, rowTops, colLefts)
		}
	}

	override fun dispatchDraw(canvas: Canvas) {
		drawHeaders(canvas)
		super.dispatchDraw(canvas)
		drawGrid(canvas)
	}

	private fun resetMeasures() {
		val cols = cells.maxOfOrNull { it.col + 1 } ?: 0
		val rows = cells.maxOfOrNull { it.row + 1 } ?: 0
		colWidths.clear()
		rowHeights.clear()
		repeat(cols) { colWidths.add(0) }
		repeat(rows) { rowHeights.add(0) }
	}

	private fun measureCell(cell: Cell, heightMeasureSpec: Int) {
		val widthSpec = if (theme.tableCellWrap) {
			MeasureSpec.makeMeasureSpec(cellMaxWidth(), MeasureSpec.AT_MOST)
		} else {
			MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED)
		}
		measureChild(cell.view, widthSpec, heightMeasureSpec)
		colWidths[cell.col] = maxOf(colWidths[cell.col], cell.view.measuredWidth)
		rowHeights[cell.row] = maxOf(rowHeights[cell.row], cell.view.measuredHeight)
	}

	private fun layoutCell(cell: Cell, rowTops: IntArray, colLefts: IntArray) {
		val left = colLefts[cell.col]
		val top = rowTops[cell.row]
		val width = colWidths[cell.col]
		val height = rowHeights[cell.row]
		cell.view.layout(left, top, left + width, top + height)
	}

	private fun offsets(values: List<Int>): IntArray {
		val out = IntArray(values.size)
		for (i in 1 until out.size)
			out[i] = out[i - 1] + values[i - 1]
		return out
	}

	private fun drawHeaders(canvas: Canvas) {
		for (row in rowHeights.indices) {
			if (rowHeader.getOrNull(row) == true) {
				val top = rowHeights.take(row).sum().toFloat()
				canvas.drawRect(0f, top, measuredWidth.toFloat(), top + rowHeights[row], headerPaint)
			}
		}
	}

	private fun drawGrid(canvas: Canvas) {
		drawVerticalLines(canvas)
		drawHorizontalLines(canvas)
	}

	private fun drawVerticalLines(canvas: Canvas) {
		var x = 0
		canvas.drawLine(0f, 0f, measuredWidth.toFloat(), 0f, linePaint)
		for (width in colWidths) {
			canvas.drawLine(x.toFloat(), 0f, x.toFloat(), measuredHeight.toFloat(), linePaint)
			x += width
		}
		canvas.drawLine(x.toFloat(), 0f, x.toFloat(), measuredHeight.toFloat(), linePaint)
	}

	private fun drawHorizontalLines(canvas: Canvas) {
		var y = 0
		for (height in rowHeights) {
			canvas.drawLine(0f, y.toFloat(), measuredWidth.toFloat(), y.toFloat(), linePaint)
			y += height
		}
		canvas.drawLine(0f, y.toFloat(), measuredWidth.toFloat(), y.toFloat(), linePaint)
	}

	private fun cellMaxWidth(): Int {
		return context.dp(theme.tableCellMaxWidthDp)
	}
}
