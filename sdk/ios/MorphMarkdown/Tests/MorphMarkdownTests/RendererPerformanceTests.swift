#if canImport(UIKit)
import UIKit
import XCTest
import Darwin
@testable import MorphMarkdown

final class RendererPerformanceTests: XCTestCase {
	private let targetBytes = 100 * 1024

	func testHundredKilobyteInitialRenderBudget() throws {
		try requirePerformanceEnvironment()
		let markdown = performanceFixture()
		let memoryBefore = residentMemory()
		let start = CACurrentMediaTime()
		let view = MorphMarkdownUIView(frame: CGRect(x: 0, y: 0, width: 390, height: 844))
		view.mathRenderer = nil
		view.setMarkdown(markdown)
		view.layoutIfNeeded()
		let elapsed = CACurrentMediaTime() - start
		let memoryAfter = residentMemory()
		let memoryGrowth = memoryAfter > memoryBefore ? memoryAfter - memoryBefore : 0

		XCTAssertGreaterThanOrEqual(markdown.utf8.count, targetBytes)
		XCTAssertLessThanOrEqual(elapsed, 0.250, "100 KB initial render exceeded the 250 ms release budget")
		XCTAssertLessThanOrEqual(memoryGrowth, 50 * 1024 * 1024, "100 KB render exceeded the 50 MB memory budget")
	}

	func testHundredKilobyteStreamingAppendBudget() throws {
		try requirePerformanceEnvironment()
		let view = MorphMarkdownUIView(frame: CGRect(x: 0, y: 0, width: 390, height: 844))
		view.mathRenderer = nil
		view.setMarkdown(performanceFixture(), final: false)
		let start = CACurrentMediaTime()
		view.append("\n\nFinal streaming block.", final: true)
		let elapsed = CACurrentMediaTime() - start

		XCTAssertLessThanOrEqual(elapsed, 0.020, "100 KB streaming append exceeded the 20 ms budget")
	}

	private func requirePerformanceEnvironment() throws {
		guard ProcessInfo.processInfo.environment["MORPH_RUN_PERFORMANCE_TESTS"] == "1" else {
			throw XCTSkip("Set MORPH_RUN_PERFORMANCE_TESTS=1 on an iPhone 15-class device.")
		}
	}

	private func residentMemory() -> UInt64 {
		var info = mach_task_basic_info()
		var count = mach_msg_type_number_t(
			MemoryLayout<mach_task_basic_info>.size / MemoryLayout<natural_t>.size
		)
		let result = withUnsafeMutablePointer(to: &info) { pointer in
			pointer.withMemoryRebound(to: integer_t.self, capacity: Int(count)) { rebound in
				task_info(mach_task_self_, task_flavor_t(MACH_TASK_BASIC_INFO), rebound, &count)
			}
		}
		return result == KERN_SUCCESS ? UInt64(info.resident_size) : 0
	}

	private func performanceFixture() -> String {
		let prelude = """
		# 100 KB performance fixture

		- [x] completed task
		- [ ] pending task with enough text to wrap naturally across a narrow phone viewport

		| feature | status | notes |
		|---|---|---|
		| streaming | ready | stable-prefix updates reuse completed blocks |
		| tables | ready | intrinsic widths preserve unbreakable content |

		> Block quotes remain readable while content is appended.

		"""
		let sentence = "MorphMarkdown renders **bold**, *emphasis*, `inline code`, [links](https://example.com), and 中文混排文本 while streaming stable blocks. "
		let section = String(repeating: sentence, count: 16) + "\n\n"
		var output = prelude
		while output.utf8.count < targetBytes {
			output += section
		}
		return output
	}
}
#endif
