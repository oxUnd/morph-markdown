import UIKit

enum DemoImageWriter {
	static func write(to url: URL) {
		let format = UIGraphicsImageRendererFormat()
		format.scale = 1
		let image = UIGraphicsImageRenderer(size: CGSize(width: 640, height: 360), format: format).image { context in
			draw(in: context.cgContext)
		}
		try? image.pngData()?.write(to: url)
	}

	private static func draw(in context: CGContext) {
		fillBackground(context)
		drawTitle(in: context)
		drawTable(in: context)
	}

	private static func fillBackground(_ context: CGContext) {
		context.setFillColor(UIColor(red: 0.97, green: 0.98, blue: 0.96, alpha: 1).cgColor)
		context.fill(CGRect(x: 0, y: 0, width: 640, height: 360))
	}

	private static func drawTitle(in context: CGContext) {
		let title = NSAttributedString(string: "MorphMarkdown iOS", attributes: [
			.font: UIFont.boldSystemFont(ofSize: 42),
			.foregroundColor: UIColor(red: 0.12, green: 0.13, blue: 0.13, alpha: 1)
		])
		title.draw(at: CGPoint(x: 40, y: 38))
		let subtitle = NSAttributedString(string: "generated PNG resource", attributes: [
			.font: UIFont.monospacedSystemFont(ofSize: 28, weight: .regular),
			.foregroundColor: UIColor(red: 0.12, green: 0.13, blue: 0.13, alpha: 1)
		])
		subtitle.draw(at: CGPoint(x: 40, y: 100))
	}

	private static func drawTable(in context: CGContext) {
		let rect = CGRect(x: 40, y: 160, width: 560, height: 150)
		context.setStrokeColor(UIColor(red: 0.27, green: 0.27, blue: 0.27, alpha: 1).cgColor)
		context.setLineWidth(4)
		context.stroke(rect.insetBy(dx: 2, dy: 2))
		drawLine("| markdown | image |", y: 196)
		drawLine("| formula  | table |", y: 246)
	}

	private static func drawLine(_ text: String, y: CGFloat) {
		NSAttributedString(string: text, attributes: [
			.font: UIFont.monospacedSystemFont(ofSize: 28, weight: .regular),
			.foregroundColor: UIColor(red: 0.12, green: 0.13, blue: 0.13, alpha: 1)
		]).draw(at: CGPoint(x: 70, y: y))
	}
}
