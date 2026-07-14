import Foundation

public protocol MorphMarkdownNativeEngine: AnyObject {
	@discardableResult
	func append(_ chunk: String, final: Bool) -> Int
	func snapshotJSON() -> String?
	func close()
}

public final class MorphMarkdownEngine: ObservableObject {
	@Published public private(set) var nodes: [MorphMarkdownNode] = []
	private let native: MorphMarkdownNativeEngine

	public convenience init() {
		self.init(native: NativeMorphMarkdownEngine())
	}

	public init(native: MorphMarkdownNativeEngine) {
		self.native = native
	}

	@discardableResult
	public func append(_ chunk: String, final: Bool = false) -> Int {
		let rc = native.append(chunk, final: final)
		reloadSnapshot()
		return rc
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

public final class NativeMorphMarkdownEngine: MorphMarkdownNativeEngine {
	private var closed = false

	public init() {}

	@discardableResult
	public func append(_ chunk: String, final: Bool) -> Int {
		_ = (chunk, final)
		return closed ? -1 : -2
	}

	public func snapshotJSON() -> String? {
		nil
	}

	public func close() {
		closed = true
	}
}

public struct MorphMarkdownOptions: Sendable {
	public var autoScrollOnAppend: Bool

	public init(autoScrollOnAppend: Bool = true) {
		self.autoScrollOnAppend = autoScrollOnAppend
	}
}
