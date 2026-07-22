#if canImport(UIKit)
import UIKit
import XCTest
@testable import MorphMarkdown

final class MathRendererColorTests: XCTestCase {
	func testMathBitmapUsesThemeForegroundColor() throws {
		let renderer = try XCTUnwrap(MathJaxMathRenderer.bundled)
		var theme = MorphMarkdownThemes.normal
		theme.bodyTextColor = 0xffffffff
		let imageView = try XCTUnwrap(renderer.render(latex: "x", display: false, theme: theme) as? UIImageView)
		let image = try XCTUnwrap(imageView.image?.cgImage)
		let data = try XCTUnwrap(image.dataProvider?.data)
		let bytes = CFDataGetBytePtr(data)
		let count = CFDataGetLength(data)
		var foundWhitePixel = false
		for offset in stride(from: 0, to: count, by: 4) where bytes?[offset + 3] ?? 0 > 0 {
			if (bytes?[offset] ?? 0) > 200, (bytes?[offset + 1] ?? 0) > 200, (bytes?[offset + 2] ?? 0) > 200 {
				foundWhitePixel = true
				break
			}
		}
		XCTAssertTrue(foundWhitePixel)
	}
}
#endif
