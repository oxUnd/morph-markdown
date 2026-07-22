import Foundation

enum RenderDebounceDecision: Equatable {
	case none
	case schedule(Int)
	case renderNow(Bool)
}

final class RenderDebounceState {
	private var scheduled = false
	private var pendingAutoScroll = false

	func onAppend(final: Bool, autoScroll: Bool, debounceMilliseconds: Int) -> RenderDebounceDecision {
		pendingAutoScroll = pendingAutoScroll || autoScroll
		if final || debounceMilliseconds <= 0 {
			return .renderNow(consumeAutoScroll())
		}
		guard !scheduled else { return .none }
		scheduled = true
		return .schedule(debounceMilliseconds)
	}

	func onScheduledRender() -> Bool {
		scheduled = false
		return consumeAutoScroll()
	}

	func cancel() {
		scheduled = false
		pendingAutoScroll = false
	}

	private func consumeAutoScroll() -> Bool {
		let value = pendingAutoScroll
		pendingAutoScroll = false
		scheduled = false
		return value
	}
}
