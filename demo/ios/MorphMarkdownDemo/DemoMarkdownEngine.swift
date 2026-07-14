import MorphMarkdown
import Foundation

final class DemoMarkdownEngine: MorphMarkdownNativeEngine {
	private var latestJSON: String?

	func append(_ chunk: String, final: Bool) -> Int {
		latestJSON = chunk
		return 0
	}

	func snapshotJSON() -> String? {
		latestJSON
	}

	func close() {}
}
