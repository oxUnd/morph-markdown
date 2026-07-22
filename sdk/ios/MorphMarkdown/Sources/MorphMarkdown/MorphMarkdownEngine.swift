import Foundation
import MorphMarkdownNative

public final class MorphMarkdownEngine {
	private var handle: OpaquePointer?

	public init?() {
		handle = morph_ios_engine_create()
		if handle == nil {
			return nil
		}
	}

	deinit {
		close()
	}

	public func append(_ markdown: String, final: Bool = false) -> Int32 {
		guard let handle else {
			return -1
		}
		return markdown.withCString { bytes in
			morph_ios_engine_append(handle, bytes, strlen(bytes), final ? 1 : 0)
		}
	}

	public func snapshotJson() -> String? {
		guard let handle, let pointer = morph_ios_engine_snapshot_json(handle) else {
			return nil
		}
		defer { morph_ios_free(pointer) }
		return String(cString: pointer)
	}

	public func stableBlockCount() -> Int {
		guard let handle else {
			return 0
		}
		return Int(morph_ios_engine_stable_block_count(handle))
	}

	public func close() {
		if let handle {
			morph_ios_engine_destroy(handle)
			self.handle = nil
		}
	}
}

public struct MorphMarkdownOptions: Equatable {
	public var autoScrollOnAppend: Bool
	public var appendRenderDebounceMilliseconds: Int

	public init(autoScrollOnAppend: Bool = true, appendRenderDebounceMilliseconds: Int = 160) {
		self.autoScrollOnAppend = autoScrollOnAppend
		self.appendRenderDebounceMilliseconds = appendRenderDebounceMilliseconds
	}
}
