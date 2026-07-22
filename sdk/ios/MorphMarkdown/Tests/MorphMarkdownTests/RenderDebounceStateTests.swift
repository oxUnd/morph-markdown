import XCTest
@testable import MorphMarkdown

final class RenderDebounceStateTests: XCTestCase {
	func testCoalescesStreamingAppendsAndCarriesAutoScroll() {
		let state = RenderDebounceState()
		XCTAssertEqual(state.onAppend(final: false, autoScroll: false, debounceMilliseconds: 160), .schedule(160))
		XCTAssertEqual(state.onAppend(final: false, autoScroll: true, debounceMilliseconds: 160), .none)
		XCTAssertTrue(state.onScheduledRender())
		XCTAssertEqual(state.onAppend(final: false, autoScroll: false, debounceMilliseconds: 160), .schedule(160))
	}

	func testFinalRendersImmediatelyAndCancelsScheduledState() {
		let state = RenderDebounceState()
		XCTAssertEqual(state.onAppend(final: false, autoScroll: true, debounceMilliseconds: 160), .schedule(160))
		XCTAssertEqual(state.onAppend(final: true, autoScroll: false, debounceMilliseconds: 160), .renderNow(true))
		XCTAssertEqual(state.onAppend(final: false, autoScroll: false, debounceMilliseconds: 160), .schedule(160))
	}
}
