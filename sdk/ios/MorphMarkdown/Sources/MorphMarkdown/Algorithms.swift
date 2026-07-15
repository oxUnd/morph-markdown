import CoreGraphics
import Foundation

struct InlineItemSize: Equatable {
	let width: CGFloat
	let height: CGFloat
}

struct InlineLine: Equatable {
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
		if items.isEmpty {
			return []
		}
		var lines: [InlineLine] = []
		var start = 0
		var width: CGFloat = 0
		var height: CGFloat = 0
		for index in items.indices {
			let item = items[index]
			if shouldWrap(width: width, itemWidth: item.width, maxWidth: maxWidth) {
				lines.append(line(start: start, end: index, width: width, height: height, minHeight: minLineHeight))
				start = index
				width = 0
				height = 0
			}
			width += item.width
			height = max(height, item.height)
		}
		lines.append(line(start: start, end: items.count, width: width, height: height, minHeight: minLineHeight))
		return lines
	}

	private static func shouldWrap(width: CGFloat, itemWidth: CGFloat, maxWidth: CGFloat) -> Bool {
		return width > 0 && width + itemWidth > maxWidth
	}

	private static func line(start: Int, end: Int, width: CGFloat, height: CGFloat, minHeight: CGFloat) -> InlineLine {
		return InlineLine(start: start, end: end, width: width, height: max(height, minHeight))
	}
}

enum InlineTextFragmenter {
	static func fragments(_ value: String) -> [String] {
		if value.isEmpty {
			return []
		}
		var out: [String] = []
		var token = ""
		for scalar in value.unicodeScalars {
			let string = String(scalar)
			if isStandalone(scalar) {
				flush(&token, into: &out)
				out.append(string)
			} else {
				token.append(string)
			}
		}
		flush(&token, into: &out)
		return out.filter { !$0.isEmpty }
	}

	private static func flush(_ token: inout String, into out: inout [String]) {
		if !token.isEmpty {
			out.append(token)
			token.removeAll(keepingCapacity: true)
		}
	}

	private static func isStandalone(_ scalar: UnicodeScalar) -> Bool {
		return CharacterSet.whitespacesAndNewlines.contains(scalar) || isCJK(scalar) || isBreakPunctuation(scalar)
	}
}

struct TableCellWidth: Equatable {
	let column: Int
	let minWidth: CGFloat
	let preferredWidth: CGFloat
}

struct TableColumnWidth: Equatable {
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
		if columnCount <= 0 {
			return []
		}
		let columns = makeColumns(cells: cells, count: columnCount, maxWidth: maxColumnWidth, wrap: wrap)
		guard wrap, let availableWidth, availableWidth > 0 else {
			return columns.map(\.preferredWidth)
		}
		return fit(columns: columns, availableWidth: availableWidth)
	}

	private static func makeColumns(cells: [TableCellWidth], count: Int, maxWidth: CGFloat, wrap: Bool) -> [TableColumnWidth] {
		return (0..<count).map { column in
			let columnCells = cells.filter { $0.column == column }
			let minWidth = columnCells.map(\.minWidth).max() ?? 0
			let preferred = columnCells.map(\.preferredWidth).max() ?? 0
			return TableColumnWidth(minWidth: minWidth, preferredWidth: preferredWidth(minWidth, preferred, maxWidth, wrap))
		}
	}

	private static func preferredWidth(_ minWidth: CGFloat, _ preferred: CGFloat, _ maxWidth: CGFloat, _ wrap: Bool) -> CGFloat {
		if !wrap {
			return preferred
		}
		return max(min(preferred, maxWidth), minWidth)
	}

	private static func fit(columns: [TableColumnWidth], availableWidth: CGFloat) -> [CGFloat] {
		let minSum = columns.map(\.minWidth).reduce(0, +)
		let preferredSum = columns.map(\.preferredWidth).reduce(0, +)
		if minSum >= availableWidth {
			return columns.map(\.minWidth)
		}
		if preferredSum > availableWidth {
			return shrink(columns, overflow: preferredSum - availableWidth)
		}
		return grow(columns, extra: availableWidth - preferredSum)
	}

	private static func shrink(_ columns: [TableColumnWidth], overflow: CGFloat) -> [CGFloat] {
		let capacities = columns.map { max($0.preferredWidth - $0.minWidth, 0) }
		let shrink = distribute(overflow, weights: capacities)
		return columns.enumerated().map { $0.element.preferredWidth - shrink[$0.offset] }
	}

	private static func grow(_ columns: [TableColumnWidth], extra: CGFloat) -> [CGFloat] {
		let weights = columns.map { max($0.preferredWidth, 1) }
		let growth = distribute(extra, weights: weights)
		return columns.enumerated().map { $0.element.preferredWidth + growth[$0.offset] }
	}

	private static func distribute(_ total: CGFloat, weights: [CGFloat]) -> [CGFloat] {
		let sum = weights.reduce(0, +)
		if total <= 0 || sum <= 0 {
			return weights.map { _ in 0 }
		}
		return weights.map { floor(total * $0 / sum) }.addingRemainder(total)
	}
}

private extension Array where Element == CGFloat {
	func addingRemainder(_ total: CGFloat) -> [CGFloat] {
		var out = self
		var remainder = Int(round(total - reduce(0, +)))
		let order = indices.sorted { self[$0] > self[$1] }
		var index = 0
		while remainder > 0 && !order.isEmpty {
			out[order[index % order.count]] += 1
			remainder -= 1
			index += 1
		}
		return out
	}
}

func expandTabs(_ value: String, tabSize: Int) -> String {
	if !value.contains("\t") {
		return value
	}
	var out = ""
	var column = 0
	for char in value {
		if char == "\t" {
			let spaces = tabSize - (column % tabSize)
			out += String(repeating: " ", count: spaces)
			column += spaces
		} else {
			out.append(char)
			column = char == "\n" ? 0 : column + 1
		}
	}
	return out
}

func processedText(_ value: String, theme: MorphMarkdownTheme, allowCjkSpacing: Bool) -> String {
	if !allowCjkSpacing || theme.textProcessor != .cjkSpacing {
		return value
	}
	return CjkTextProcessor.spacing(value)
}

private enum CjkTextProcessor {
	static func spacing(_ value: String) -> String {
		let scalars = Array(value.unicodeScalars)
		if scalars.count < 2 {
			return value
		}
		var out = ""
		for index in scalars.indices {
			out.append(String(scalars[index]))
			if needsSpacing(left: scalars[index], right: scalars[safe: index + 1]) {
				out.append("\u{2006}")
			}
		}
		return out
	}

	private static func needsSpacing(left: UnicodeScalar, right: UnicodeScalar?) -> Bool {
		guard let right else {
			return false
		}
		return (isCJK(left) && isAsciiWord(right)) || (isAsciiWord(left) && isCJK(right))
	}

	private static func isAsciiWord(_ scalar: UnicodeScalar) -> Bool {
		return (0x21...0x7e).contains(Int(scalar.value)) && !asciiPunctuation.contains(Character(scalar))
	}

	private static let asciiPunctuation: Set<Character> = [".", ",", ":", ";", "!", "?", "'", "\"", "`", "/", "\\", "|", "(", ")", "[", "]", "{", "}", "<", ">", "+", "-", "*", "=", "_"]
}

private extension Array {
	subscript(safe index: Int) -> Element? {
		return indices.contains(index) ? self[index] : nil
	}
}

func isCJK(_ scalar: UnicodeScalar) -> Bool {
	let value = scalar.value
	return (0x4E00...0x9FFF).contains(value) ||
		(0x3400...0x4DBF).contains(value) ||
		(0xF900...0xFAFF).contains(value) ||
		(0x3040...0x309F).contains(value) ||
		(0x30A0...0x30FF).contains(value) ||
		(0xAC00...0xD7AF).contains(value)
}

func isBreakPunctuation(_ scalar: UnicodeScalar) -> Bool {
	return [",", ".", ";", ":", "，", "。", "；", "：", "/", "-", "_"].contains(String(scalar))
}
