import Foundation

struct StreamChunk {
	let text: String
	let delay: TimeInterval
}

enum StreamingSimulator {
	static func create(markdown: String) -> [StreamChunk] {
		var generator = SeededGenerator(seed: 20260714)
		var out: [StreamChunk] = []
		var index = markdown.startIndex
		while index < markdown.endIndex {
			let next = nextIndex(in: markdown, from: index, generator: &generator)
			let text = String(markdown[index..<next])
			out.append(StreamChunk(text: text, delay: delay(for: text, generator: &generator)))
			index = next
		}
		return out
	}

	private static func nextIndex(in text: String, from start: String.Index, generator: inout SeededGenerator) -> String.Index {
		let count = nextTokenSize(in: text, from: start, generator: &generator)
		let raw = text.index(start, offsetBy: count, limitedBy: text.endIndex) ?? text.endIndex
		return extendToNaturalBoundary(in: text, start: start, end: raw)
	}

	private static func nextTokenSize(in text: String, from start: String.Index, generator: inout SeededGenerator) -> Int {
		if text[start] == "\n" {
			let next = text.index(after: start)
			return next < text.endIndex && text[next] == "\n" ? 2 : 1
		}
		switch generator.nextInt(upperBound: 100) {
		case 0...52: return generator.nextInt(in: 1...4)
		case 53...82: return generator.nextInt(in: 5...10)
		case 83...94: return generator.nextInt(in: 11...19)
		default: return generator.nextInt(in: 20...35)
		}
	}

	private static func extendToNaturalBoundary(in text: String, start: String.Index, end: String.Index) -> String.Index {
		if end >= text.endIndex {
			return text.endIndex
		}
		if end == start {
			return text.index(after: start)
		}
		let previous = text.index(before: end)
		if text[previous].isWhitespace {
			return end
		}
		return text[end].isWhitespace || punctuation.contains(text[end]) ? text.index(after: end) : end
	}

	private static func delay(for text: String, generator: inout SeededGenerator) -> TimeInterval {
		let base: Int
		if text.contains("\n\n") {
			base = generator.nextInt(in: 460...920)
		} else if text.contains("\n") {
			base = generator.nextInt(in: 220...520)
		} else if text.contains(where: { strongPunctuation.contains($0) }) {
			base = generator.nextInt(in: 180...420)
		} else if text.count <= 3 {
			base = generator.nextInt(in: 24...90)
		} else if text.count <= 10 {
			base = generator.nextInt(in: 45...140)
		} else {
			base = generator.nextInt(in: 80...210)
		}
		return TimeInterval(base + occasionalPause(generator: &generator)) / 1000
	}

	private static func occasionalPause(generator: inout SeededGenerator) -> Int {
		switch generator.nextInt(upperBound: 100) {
		case 0...4: return generator.nextInt(in: 420...950)
		case 5...16: return generator.nextInt(in: 120...360)
		default: return 0
		}
	}

	private static let punctuation = Set(".，,:;!?)]}>|。：；！？）】》、")
	private static let strongPunctuation = Set(".!?;。！？；")
}

private struct SeededGenerator {
	private var state: UInt64

	init(seed: UInt64) {
		state = seed
	}

	mutating func nextInt(upperBound: Int) -> Int {
		return Int(next() % UInt64(upperBound))
	}

	mutating func nextInt(in range: ClosedRange<Int>) -> Int {
		return range.lowerBound + nextInt(upperBound: range.upperBound - range.lowerBound + 1)
	}

	private mutating func next() -> UInt64 {
		state = state &* 6364136223846793005 &+ 1442695040888963407
		return state >> 32
	}
}
