import Foundation

public protocol MorphMarkdownNativeEngine: AnyObject {
	func append(_ chunk: String, final: Bool)
	func snapshotJSON() -> String?
	func close()
}

public final class MorphMarkdownEngine: ObservableObject {
	@Published public private(set) var nodes: [MorphMarkdownNode] = []
	private let native: MorphMarkdownNativeEngine

	public init(native: MorphMarkdownNativeEngine) {
		self.native = native
	}

	public func append(_ chunk: String, final: Bool = false) {
		native.append(chunk, final: final)
		reloadSnapshot()
	}

	public func reloadSnapshot() {
		guard let json = native.snapshotJSON() else {
			nodes = []
			return
		}
		nodes = MorphMarkdownJSON.decode(json)
	}

	deinit {
		native.close()
	}
}
