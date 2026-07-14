import MorphMarkdown
import SwiftUI
import UIKit

struct ContentView: View {
	@StateObject private var engine = MorphMarkdownEngine(native: DemoMarkdownEngine())
	@State private var headingMode = 3
	@State private var tabMode = 1
	@State private var tableWrap = true
	@State private var compactCode = false
	@State private var assetFont = true
	@State private var streaming = false
	@State private var demoImagePath = ""

	var body: some View {
		GeometryReader { proxy in
			VStack(spacing: 0) {
				controls
					.padding(.top, proxy.safeAreaInsets.top)
				MorphMarkdownView(
					engine: engine,
					options: MorphMarkdownOptions(autoScrollOnAppend: true),
					theme: currentTheme(),
					mathRenderer: MathJaxMathRenderer(),
					imageLoader: FileImageLoader()
				)
				.background(Color(red: 0.98, green: 0.98, blue: 0.97))
			}
			.background(Color(red: 0.98, green: 0.98, blue: 0.97))
			.ignoresSafeArea(edges: .top)
		}
		.task {
			if demoImagePath.isEmpty {
				demoImagePath = createDemoImage()
			}
			replayStreaming()
		}
	}

	private var controls: some View {
		ScrollView(.horizontal, showsIndicators: false) {
			HStack(spacing: 8) {
				control("H \(headingLabel())") {
					headingMode = (headingMode + 1) % 5
				}
				control("Tab \(tabSize())") {
					tabMode = (tabMode + 1) % 3
				}
				control(tableWrap ? "Table wrap" : "Table nowrap") {
					tableWrap.toggle()
				}
				control(compactCode ? "Code compact" : "Code normal") {
					compactCode.toggle()
				}
				control(assetFont ? "Font asset" : "Font system") {
					assetFont.toggle()
				}
				control(streaming ? "Streaming" : "Replay") {
					replayStreaming()
				}
				.disabled(streaming)
			}
			.padding(.horizontal, 12)
			.padding(.top, 8)
			.padding(.bottom, 8)
		}
		.background(Color(red: 0.98, green: 0.98, blue: 0.97))
	}

	private func control(_ title: String, action: @escaping () -> Void) -> some View {
		Button(title, action: action)
			.font(.system(size: 13))
			.foregroundColor(Color(red: 0.13, green: 0.13, blue: 0.13))
			.padding(.horizontal, 10)
			.padding(.vertical, 7)
			.overlay(Rectangle().stroke(Color(red: 0.27, green: 0.27, blue: 0.27), lineWidth: 1))
	}

	private func replayStreaming() {
		guard !demoImagePath.isEmpty else { return }
		streaming = true
		let validUri = "file://\(demoImagePath)"
		let invalidUri = "file://\(temporaryDirectory().appendingPathComponent("missing-demo-image.png").path)"
		Task { @MainActor in
			for snapshot in DemoMarkdownDocument.streamingSnapshots(validUri: validUri, invalidUri: invalidUri) {
				engine.append(snapshot, final: false)
				try? await Task.sleep(nanoseconds: 450_000_000)
			}
			engine.append(DemoMarkdownDocument.fullJSON(validUri: validUri, invalidUri: invalidUri), final: true)
			streaming = false
		}
	}

	private func currentTheme() -> MorphMarkdownTheme {
		var theme: MorphMarkdownTheme
		switch headingMode {
		case 1:
			theme = MorphMarkdownThemes.LargeHeadings
		case 2:
			theme = MorphMarkdownThemes.CompactHeadings
		case 3:
			theme = hetiTheme()
		case 4:
			theme = MorphMarkdownThemes.HetiLikeHei
		default:
			theme = MorphMarkdownThemes.Normal
		}
		let tabs = tabSize()
		theme = theme.copy(tabSize: tabs, codeBlockTabSize: tabs)
		if !tableWrap {
			theme = theme.copy(tableCellWrap: false)
		}
		if compactCode {
			theme = theme.copy(codeBlockTextSizeSp: 13, inlineCodeTextSizeSp: 14)
		}
		theme = theme.copy(
			tableCellMaxWidthDp: 260,
			tableCellPaddingHorizontalDp: 10,
			tableCellPaddingVerticalDp: 8
		)
		return theme
	}

	private func hetiTheme() -> MorphMarkdownTheme {
		if !assetFont { return MorphMarkdownThemes.HetiLike }
		return MorphMarkdownThemes.hetiLikeWithFont(
			fontAssetPath: "fonts/NotoSerifCJKsc-Regular.otf",
			boldFontAssetPath: "fonts/NotoSerifCJKsc-Bold.otf"
		)
	}

	private func headingLabel() -> String {
		switch headingMode {
		case 1: return "large"
		case 2: return "compact"
		case 3: return "heti"
		case 4: return "heti hei"
		default: return "normal"
		}
	}

	private func tabSize() -> Int {
		switch tabMode {
		case 0: return 2
		case 2: return 8
		default: return 4
		}
	}

	private func createDemoImage() -> String {
		let url = temporaryDirectory().appendingPathComponent("markdown-render-generated.png")
		let size = CGSize(width: 640, height: 360)
		let renderer = UIGraphicsImageRenderer(size: size)
		let image = renderer.image { context in
			drawDemoImage(context: context.cgContext, size: size)
		}
		if let data = image.pngData() {
			try? data.write(to: url)
		}
		return url.path
	}

	private func drawDemoImage(context: CGContext, size: CGSize) {
		context.setFillColor(UIColor(red: 0.97, green: 0.97, blue: 0.95, alpha: 1).cgColor)
		context.fill(CGRect(origin: .zero, size: size))
		draw("markdown-render", at: CGPoint(x: 40, y: 40), size: 42, weight: .bold)
		draw("generated PNG", at: CGPoint(x: 40, y: 96), size: 30, weight: .regular, monospace: true)
		context.setStrokeColor(UIColor(red: 0.27, green: 0.27, blue: 0.27, alpha: 1).cgColor)
		context.setLineWidth(4)
		context.stroke(CGRect(x: 40, y: 160, width: 560, height: 150))
		draw("| markdown | image |", at: CGPoint(x: 70, y: 190), size: 28, monospace: true)
		draw("| formula  | table |", at: CGPoint(x: 70, y: 240), size: 28, monospace: true)
	}

	private func draw(
		_ value: String,
		at point: CGPoint,
		size: CGFloat,
		weight: UIFont.Weight = .regular,
		monospace: Bool = false
	) {
		let font = monospace ? UIFont.monospacedSystemFont(ofSize: size, weight: weight) :
			UIFont.systemFont(ofSize: size, weight: weight)
		let attrs: [NSAttributedString.Key: Any] = [
			.font: font,
			.foregroundColor: UIColor(red: 0.13, green: 0.13, blue: 0.13, alpha: 1)
		]
		value.draw(at: point, withAttributes: attrs)
	}

	private func temporaryDirectory() -> URL {
		FileManager.default.temporaryDirectory
	}
}
