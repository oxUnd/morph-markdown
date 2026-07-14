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
		color = theme.tableStyle.borderColor
		strokeWidth = context.dpFloat(theme.tableStyle.borderWidthDp)
	}
	private val headerPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
		color = theme.tableStyle.headerBackgroundColor
		style = Paint.Style.FILL
	}
	private val bodyPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
		color = theme.tableStyle.bodyBackgroundColor
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
		drawBackgrounds(canvas)
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

	private fun drawBackgrounds(canvas: Canvas) {
		for (row in rowHeights.indices) {
			val top = rowHeights.take(row).sum().toFloat()
			val paint = if (rowHeader.getOrNull(row) == true) headerPaint else bodyPaint
			canvas.drawRect(0f, top, measuredWidth.toFloat(), top + rowHeights[row], paint)
		}
	}

	private fun drawGrid(canvas: Canvas) {
		val offset = linePaint.strokeWidth / 2f
		drawVerticalLines(canvas, offset)
		drawHorizontalLines(canvas, offset)
	}

	private fun drawVerticalLines(canvas: Canvas, offset: Float) {
		var x = 0
		canvas.drawLine(offset, offset, offset, measuredHeight - offset, linePaint)
		for (width in colWidths) {
			x += width
			val alignedX = (x.toFloat() - offset).coerceAtLeast(offset)
			canvas.drawLine(alignedX, offset, alignedX, measuredHeight - offset, linePaint)
		}
	}

	private fun drawHorizontalLines(canvas: Canvas, offset: Float) {
		var y = 0
		canvas.drawLine(offset, offset, measuredWidth - offset, offset, linePaint)
		for (height in rowHeights) {
			y += height
			val alignedY = (y.toFloat() - offset).coerceAtLeast(offset)
			canvas.drawLine(offset, alignedY, measuredWidth - offset, alignedY, linePaint)
		}
	}

	private fun cellMaxWidth(): Int {
		return context.dp(theme.tableCellMaxWidthDp)
	}
}
