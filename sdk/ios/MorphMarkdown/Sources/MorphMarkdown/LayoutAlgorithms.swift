import Foundation
import SwiftUI
#if canImport(UIKit)
import UIKit
#elseif canImport(AppKit)
import AppKit
#endif

struct InlineItemSize {
	let width: CGFloat
	let height: CGFloat
}

struct InlineLine {
	let start: Int
	let end: Int
	let width: CGFloat
	let height: CGFloat
}

enum InlineLineBreaker {
	static func breakLines(
		items: [InlineItemSize],
		maxWidth: CGFloat,
		minLineHeight: CGFloat
	) -> [InlineLine] {
		if items.isEmpty { return [] }
		var lines: [InlineLine] = []
		var lineStart = 0
		var lineWidth: CGFloat = 0
		var lineHeight: CGFloat = 0
		for index in items.indices {
			let item = items[index]
			if shouldWrap(lineWidth: lineWidth, itemWidth: item.width, maxWidth: maxWidth) {
				lines.append(line(start: lineStart, end: index, width: lineWidth, height: lineHeight, minLineHeight: minLineHeight))
				lineStart = index
				lineWidth = 0
				lineHeight = 0
			}
			lineWidth += item.width
			lineHeight = max(lineHeight, item.height)
		}
		lines.append(line(start: lineStart, end: items.count, width: lineWidth, height: lineHeight, minLineHeight: minLineHeight))
		return lines
	}

	private static func shouldWrap(lineWidth: CGFloat, itemWidth: CGFloat, maxWidth: CGFloat) -> Bool {
		lineWidth > 0 && lineWidth + itemWidth > maxWidth
	}

	private static func line(
		start: Int,
		end: Int,
		width: CGFloat,
		height: CGFloat,
		minLineHeight: CGFloat
	) -> InlineLine {
		InlineLine(start: start, end: end, width: width, height: max(height, minLineHeight))
	}
}

enum InlineTextFragmenter {
	static func fragments(_ value: String) -> [String] {
		if value.isEmpty { return [] }
		var out: [String] = []
		var token = ""
		for ch in value {
			if isStandalone(ch) {
				flush(&token, into: &out)
				out.append(String(ch))
			} else {
				token.append(ch)
			}
		}
		flush(&token, into: &out)
		return out.filter { !$0.isEmpty }
	}

	private static func flush(_ token: inout String, into out: inout [String]) {
		if !token.isEmpty { out.append(token) }
		token = ""
	}

	private static func isStandalone(_ ch: Character) -> Bool {
		ch.isWhitespace || isCJK(ch) || isBreakPunctuation(ch)
	}

	private static func isCJK(_ ch: Character) -> Bool {
		guard let scalar = ch.unicodeScalars.first else { return false }
		switch scalar.value {
		case 0x3040...0x30ff, 0x3400...0x4dbf, 0x4e00...0x9fff, 0xac00...0xd7af, 0xf900...0xfaff:
			return true
		default:
			return false
		}
	}

	private static func isBreakPunctuation(_ ch: Character) -> Bool {
		",.;:，。；：/-_".contains(ch)
	}
}

struct TableCellWidth {
	let column: Int
	let minWidth: CGFloat
	let preferredWidth: CGFloat
}

private struct TableColumnWidth {
	let minWidth: CGFloat
	let preferredWidth: CGFloat
}

enum TableColumnSizer {
	static func sizeColumns(
		cells: [TableCellWidth],
		columnCount: Int,
		availableWidth: CGFloat?,
		maxColumnWidth: CGFloat,
		wrap: Bool
	) -> [CGFloat] {
		if columnCount <= 0 { return [] }
		let columns = resolvedColumns(
			cells: integerCells(cells),
			columnCount: columnCount,
			maxColumnWidth: ceil(maxColumnWidth),
			wrap: wrap
		)
		guard wrap, let availableWidth = availableWidth.map({ floor($0) }), availableWidth > 0 else {
			return columns.map(\.preferredWidth)
		}
		return fitColumns(columns, availableWidth: availableWidth)
	}

	private static func integerCells(_ cells: [TableCellWidth]) -> [TableCellWidth] {
		cells.map { cell in
			TableCellWidth(
				column: cell.column,
				minWidth: ceil(cell.minWidth),
				preferredWidth: ceil(cell.preferredWidth)
			)
		}
	}

	private static func resolvedColumns(
		cells: [TableCellWidth],
		columnCount: Int,
		maxColumnWidth: CGFloat,
		wrap: Bool
	) -> [TableColumnWidth] {
		(0..<columnCount).map { column in
			let columnCells = cells.filter { $0.column == column }
			let minWidth = columnCells.map(\.minWidth).max() ?? 0
			let preferred = columnCells.map(\.preferredWidth).max() ?? 0
			return TableColumnWidth(
				minWidth: minWidth,
				preferredWidth: preferredWidth(minWidth: minWidth, preferredWidth: preferred, maxColumnWidth: maxColumnWidth, wrap: wrap)
			)
		}
	}

	private static func preferredWidth(
		minWidth: CGFloat,
		preferredWidth: CGFloat,
		maxColumnWidth: CGFloat,
		wrap: Bool
	) -> CGFloat {
		if !wrap { return preferredWidth }
		return max(minWidth, min(preferredWidth, maxColumnWidth))
	}

	private static func fitColumns(_ columns: [TableColumnWidth], availableWidth: CGFloat) -> [CGFloat] {
		let minSum = columns.map(\.minWidth).reduce(0, +)
		let preferredSum = columns.map(\.preferredWidth).reduce(0, +)
		if minSum >= availableWidth { return columns.map(\.minWidth) }
		if preferredSum > availableWidth {
			return shrinkColumns(columns, overflow: preferredSum - availableWidth)
		}
		return growColumns(columns, extra: availableWidth - preferredSum)
	}

	private static func shrinkColumns(_ columns: [TableColumnWidth], overflow: CGFloat) -> [CGFloat] {
		let capacities = columns.map { max(0, $0.preferredWidth - $0.minWidth) }
		let shrink = distribute(total: overflow, weights: capacities)
		return columns.enumerated().map { index, column in column.preferredWidth - shrink[index] }
	}

	private static func growColumns(_ columns: [TableColumnWidth], extra: CGFloat) -> [CGFloat] {
		let weights = columns.map { max(1, $0.preferredWidth) }
		let growth = distribute(total: extra, weights: weights)
		return columns.enumerated().map { index, column in column.preferredWidth + growth[index] }
	}

	private static func distribute(total: CGFloat, weights: [CGFloat]) -> [CGFloat] {
		if total <= 0 || weights.isEmpty { return weights.map { _ in 0 } }
		let intTotal = Int(total.rounded(.down))
		let intWeights = weights.map { Int($0.rounded(.down)) }
		let sum = intWeights.reduce(0, +)
		if intTotal <= 0 || sum <= 0 { return weights.map { _ in 0 } }
		let base = intWeights.map { intTotal * $0 / sum }
		return addRemainder(base: base, weights: intWeights, remainder: intTotal - base.reduce(0, +)).map(CGFloat.init)
	}

	private static func addRemainder(base: [Int], weights: [Int], remainder: Int) -> [Int] {
		if remainder <= 0 { return base }
		var out = base
		let order = weights.indices.sorted { weights[$0] > weights[$1] }
		for i in 0..<remainder {
			out[order[i % order.count]] += 1
		}
		return out
	}
}

enum MarkdownTextMeasurer {
	static func width(_ value: String, size: CGFloat, monospace: Bool = false, bold: Bool = false) -> CGFloat {
		if value.isEmpty { return 0 }
		#if canImport(UIKit)
		let font: UIFont
		if monospace {
			font = .monospacedSystemFont(ofSize: size, weight: bold ? .semibold : .regular)
		} else {
			font = .systemFont(ofSize: size, weight: bold ? .semibold : .regular)
		}
		return ceil((value as NSString).size(withAttributes: [.font: font]).width)
		#elseif canImport(AppKit)
		let font: NSFont
		if monospace {
			font = .monospacedSystemFont(ofSize: size, weight: bold ? .semibold : .regular)
		} else {
			font = .systemFont(ofSize: size, weight: bold ? .semibold : .regular)
		}
		return ceil((value as NSString).size(withAttributes: [.font: font]).width)
		#else
		return CGFloat(value.count) * size * (monospace ? 0.62 : 0.55)
		#endif
	}

	static func longestUnbreakableWidth(_ value: String, size: CGFloat, monospace: Bool = false, bold: Bool = false) -> CGFloat {
		var maxWidth: CGFloat = 0
		for fragment in InlineTextFragmenter.fragments(value) where !fragment.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
			maxWidth = max(maxWidth, width(fragment, size: size, monospace: monospace, bold: bold))
		}
		return maxWidth
	}
}
