import XCTest
@testable import MorphMarkdown

final class NativeEngineTests: XCTestCase {
	func testSnapshotParsesGfmAndMath() throws {
		guard let engine = MorphMarkdownEngine() else {
			throw XCTSkip("native engine unavailable")
		}
		defer { engine.close() }

		let markdown = "# Title\n\n- [x] done\n\n| a | b |\n|---|---|\n| $x$ | ![alt](file:///tmp/a.png) |\n"
		XCTAssertEqual(engine.append(markdown, final: true), 0)
		let json = try XCTUnwrap(engine.snapshotJson())

		XCTAssertTrue(json.contains("\"heading\""))
		XCTAssertTrue(json.contains("\"tasklist\""))
		XCTAssertTrue(json.contains("\"table\""))
		XCTAssertTrue(json.contains("\"math_inline\""))
		XCTAssertTrue(json.contains("\"image\""))
	}

	func testSnapshotPreservesLinkUrlTitleAndAutolink() throws {
		let root = try snapshotNode("[Morph](https://example.com \"site\") and <https://autolink.example>\n")
		let links = root.descendants(kind: "link")

		XCTAssertEqual(links.count, 2)
		XCTAssertEqual(links[0].url, "https://example.com")
		XCTAssertEqual(links[0].title, "site")
		XCTAssertEqual(links[0].plainText, "Morph")
		XCTAssertEqual(links[1].url, "https://autolink.example")
		XCTAssertEqual(links[1].plainText, "https://autolink.example")
	}

	private func snapshotNode(_ markdown: String) throws -> MarkdownNode {
		guard let engine = MorphMarkdownEngine() else {
			throw XCTSkip("native engine unavailable")
		}
		defer { engine.close() }

		XCTAssertEqual(engine.append(markdown, final: true), 0)
		let json = try XCTUnwrap(engine.snapshotJson())
		let data = try XCTUnwrap(json.data(using: .utf8))
		return try JSONDecoder().decode(MarkdownNode.self, from: data)
	}
}

private extension MarkdownNode {
	func descendants(kind: String) -> [MarkdownNode] {
		var matches = self.kind == kind ? [self] : []
		for child in children {
			matches.append(contentsOf: child.descendants(kind: kind))
		}
		return matches
	}
}
