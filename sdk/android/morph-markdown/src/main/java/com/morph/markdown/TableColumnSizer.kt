package com.morph.markdown

internal data class TableCellWidth(
	val column: Int,
	val minWidth: Int,
	val preferredWidth: Int
)

internal data class TableColumnWidth(
	val minWidth: Int,
	val preferredWidth: Int
)

internal object TableColumnSizer {
	fun sizeColumns(
		cells: List<TableCellWidth>,
		columnCount: Int,
		availableWidth: Int?,
		maxColumnWidth: Int,
		wrap: Boolean
	): List<Int> {
		if (columnCount <= 0) return emptyList()
		val columns = columns(cells, columnCount, maxColumnWidth, wrap)
		if (!wrap || availableWidth == null || availableWidth <= 0) {
			return columns.map { it.preferredWidth }
		}
		return fitColumns(columns, availableWidth)
	}

	private fun columns(
		cells: List<TableCellWidth>,
		columnCount: Int,
		maxColumnWidth: Int,
		wrap: Boolean
	): List<TableColumnWidth> {
		return (0 until columnCount).map { column ->
			val columnCells = cells.filter { it.column == column }
			val min = columnCells.maxOfOrNull { it.minWidth } ?: 0
			val preferred = columnCells.maxOfOrNull { it.preferredWidth } ?: 0
			TableColumnWidth(min, preferredWidth(min, preferred, maxColumnWidth, wrap))
		}
	}

	private fun preferredWidth(minWidth: Int, preferredWidth: Int, maxColumnWidth: Int, wrap: Boolean): Int {
		if (!wrap) return preferredWidth
		return preferredWidth.coerceAtMost(maxColumnWidth).coerceAtLeast(minWidth)
	}

	private fun fitColumns(columns: List<TableColumnWidth>, availableWidth: Int): List<Int> {
		val minSum = columns.sumOf { it.minWidth }
		val preferredSum = columns.sumOf { it.preferredWidth }
		return when {
			minSum >= availableWidth -> columns.map { it.minWidth }
			preferredSum > availableWidth -> shrinkColumns(columns, preferredSum - availableWidth)
			else -> growColumns(columns, availableWidth - preferredSum)
		}
	}

	private fun shrinkColumns(columns: List<TableColumnWidth>, overflow: Int): List<Int> {
		val capacities = columns.map { (it.preferredWidth - it.minWidth).coerceAtLeast(0) }
		val shrink = distribute(overflow, capacities)
		return columns.mapIndexed { index, column -> column.preferredWidth - shrink[index] }
	}

	private fun growColumns(columns: List<TableColumnWidth>, extra: Int): List<Int> {
		val weights = columns.map { it.preferredWidth.coerceAtLeast(1) }
		val growth = distribute(extra, weights)
		return columns.mapIndexed { index, column -> column.preferredWidth + growth[index] }
	}

	private fun distribute(total: Int, weights: List<Int>): List<Int> {
		if (total <= 0 || weights.isEmpty()) return weights.map { 0 }
		val sum = weights.sum()
		if (sum <= 0) return weights.map { 0 }
		val base = weights.map { total * it / sum }
		return addRemainder(base, weights, total - base.sum())
	}

	private fun addRemainder(base: List<Int>, weights: List<Int>, remainder: Int): List<Int> {
		if (remainder <= 0) return base
		val out = base.toMutableList()
		val order = weights.indices.sortedByDescending { weights[it] }
		for (i in 0 until remainder) out[order[i % order.size]] += 1
		return out
	}
}
