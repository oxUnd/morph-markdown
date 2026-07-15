import MorphMarkdown
import UIKit

final class DemoViewController: UIViewController {
	private let controls = UIStackView()
	private let controlsScroll = UIScrollView()
	private let markdownView = MorphMarkdownUIView()
	private let linkToast = UILabel()
	private var chunks: [StreamChunk] = []
	private var chunkIndex = 0
	private var timer: Timer?
	private var themeMode = 0
	private var tableWrap = true
	private var compactCode = false
	private var demoImageURL: URL?

	override func viewDidLoad() {
		super.viewDidLoad()
		view.backgroundColor = UIColor(red: 0.98, green: 0.98, blue: 0.96, alpha: 1)
		configureMarkdownView()
		configureControls()
		configureLayout()
		rebuildControls()
		renderInitialDocument()
	}

	override func viewDidDisappear(_ animated: Bool) {
		super.viewDidDisappear(animated)
		timer?.invalidate()
	}

	private func configureMarkdownView() {
		markdownView.backgroundColor = view.backgroundColor
		markdownView.contentInset = UIEdgeInsets(top: 16, left: 0, bottom: 32, right: 0)
		markdownView.scrollIndicatorInsets = markdownView.contentInset
		markdownView.imageLoader = FileImageLoader()
		if let path = Bundle.main.path(forResource: "STIXTwoMath-Regular", ofType: "ttf") {
			markdownView.mathRenderer = MathJaxMathRenderer(fontPath: path)
		}
		markdownView.theme = currentTheme()
		markdownView.onLinkClick = { [weak self] url, _ in
			self?.showLinkToast(url)
		}
	}

	private func configureControls() {
		controls.axis = .horizontal
		controls.spacing = 8
		controls.alignment = .center
		controls.translatesAutoresizingMaskIntoConstraints = false
		controlsScroll.showsHorizontalScrollIndicator = false
		controlsScroll.translatesAutoresizingMaskIntoConstraints = false
		controlsScroll.addSubview(controls)
	}

	private func configureLayout() {
		markdownView.translatesAutoresizingMaskIntoConstraints = false
		view.addSubview(controlsScroll)
		view.addSubview(markdownView)
		configureLinkToast()
		NSLayoutConstraint.activate([
			controlsScroll.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
			controlsScroll.leadingAnchor.constraint(equalTo: view.leadingAnchor),
			controlsScroll.trailingAnchor.constraint(equalTo: view.trailingAnchor),
			controlsScroll.heightAnchor.constraint(equalToConstant: 54),
			controls.leadingAnchor.constraint(equalTo: controlsScroll.contentLayoutGuide.leadingAnchor, constant: 12),
			controls.trailingAnchor.constraint(equalTo: controlsScroll.contentLayoutGuide.trailingAnchor, constant: -12),
			controls.centerYAnchor.constraint(equalTo: controlsScroll.frameLayoutGuide.centerYAnchor),
			markdownView.topAnchor.constraint(equalTo: controlsScroll.bottomAnchor),
			markdownView.leadingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.leadingAnchor, constant: 20),
			markdownView.trailingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.trailingAnchor, constant: -20),
			markdownView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
			linkToast.centerXAnchor.constraint(equalTo: view.centerXAnchor),
			linkToast.leadingAnchor.constraint(greaterThanOrEqualTo: view.safeAreaLayoutGuide.leadingAnchor, constant: 20),
			linkToast.trailingAnchor.constraint(lessThanOrEqualTo: view.safeAreaLayoutGuide.trailingAnchor, constant: -20),
			linkToast.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -18)
		])
	}

	private func configureLinkToast() {
		linkToast.alpha = 0
		linkToast.numberOfLines = 1
		linkToast.font = UIFont.systemFont(ofSize: 13, weight: .medium)
		linkToast.textColor = .white
		linkToast.backgroundColor = UIColor.black.withAlphaComponent(0.78)
		linkToast.layer.cornerRadius = 8
		linkToast.layer.masksToBounds = true
		linkToast.textAlignment = .center
		linkToast.translatesAutoresizingMaskIntoConstraints = false
		view.addSubview(linkToast)
	}

	private func showLinkToast(_ url: String) {
		linkToast.text = "  \(url)  "
		UIView.animate(withDuration: 0.12) {
			self.linkToast.alpha = 1
		}
		DispatchQueue.main.asyncAfter(deadline: .now() + 1.6) { [weak self] in
			UIView.animate(withDuration: 0.2) {
				self?.linkToast.alpha = 0
			}
		}
	}

	private func rebuildControls() {
		controls.arrangedSubviews.forEach { view in
			controls.removeArrangedSubview(view)
			view.removeFromSuperview()
		}
		addButton("Reset", action: #selector(resetTapped))
		addButton("Stream", action: #selector(streamTapped))
		addButton("Full", action: #selector(fullTapped))
		addButton("Table", action: #selector(tableDemoTapped))
		addButton("Theme \(themeLabel())", action: #selector(themeTapped))
		addButton(tableWrap ? "Wrap" : "No wrap", action: #selector(tableTapped))
		addButton(compactCode ? "Code compact" : "Code normal", action: #selector(codeTapped))
	}

	private func addButton(_ title: String, action: Selector) {
		let button = UIButton(type: .system)
		var configuration = UIButton.Configuration.plain()
		configuration.title = title
		configuration.contentInsets = NSDirectionalEdgeInsets(top: 7, leading: 10, bottom: 7, trailing: 10)
		button.configuration = configuration
		button.titleLabel?.font = UIFont.systemFont(ofSize: 13, weight: .medium)
		button.tintColor = .label
		button.layer.borderColor = UIColor.label.withAlphaComponent(0.75).cgColor
		button.layer.borderWidth = 1
		button.addTarget(self, action: action, for: .touchUpInside)
		controls.addArrangedSubview(button)
	}

	private func startStreaming() {
		timer?.invalidate()
		chunkIndex = 0
		let markdown = MarkdownFixtures.create(validURL: demoImage().absoluteString, invalidURL: invalidImageURL().absoluteString)
		chunks = StreamingSimulator.create(markdown: markdown)
		markdownView.reset()
		scheduleNextChunk()
	}

	private func scheduleNextChunk() {
		guard chunkIndex < chunks.count else {
			return
		}
		let chunk = chunks[chunkIndex]
		timer = Timer.scheduledTimer(withTimeInterval: chunk.delay, repeats: false) { [weak self] _ in
			self?.appendNextChunk()
		}
	}

	private func appendNextChunk() {
		guard chunkIndex < chunks.count else {
			return
		}
		let final = chunkIndex == chunks.count - 1
		markdownView.append(chunks[chunkIndex].text, final: final)
		chunkIndex += 1
		scheduleNextChunk()
	}

	private func applyTheme() {
		rebuildControls()
		markdownView.theme = currentTheme()
	}

	private func renderFullDocument() {
		timer?.invalidate()
		let markdown = MarkdownFixtures.create(validURL: demoImage().absoluteString, invalidURL: invalidImageURL().absoluteString)
		markdownView.setMarkdown(markdown)
	}

	private func renderTableDocument() {
		timer?.invalidate()
		let markdown = MarkdownFixtures.tableDemo(validURL: demoImage().absoluteString, invalidURL: invalidImageURL().absoluteString)
		markdownView.setMarkdown(markdown)
	}

	private func renderInitialDocument() {
		if ProcessInfo.processInfo.arguments.contains("--table") {
			renderTableDocument()
		} else {
			renderFullDocument()
		}
	}

	private func currentTheme() -> MorphMarkdownTheme {
		var theme: MorphMarkdownTheme
		switch themeMode {
		case 1:
			theme = MorphMarkdownThemes.largeHeadings
		case 2:
			theme = MorphMarkdownThemes.hetiLike
		case 3:
			theme = MorphMarkdownThemes.compactHeadings
		default:
			theme = MorphMarkdownThemes.normal
		}
		theme.tableCellWrap = tableWrap
		if compactCode {
			theme.codeBlockTextSize = 13
			theme.inlineCodeTextSize = 14
		}
		return theme
	}

	private func themeLabel() -> String {
		switch themeMode {
		case 1: return "large"
		case 2: return "heti"
		case 3: return "compact"
		default: return "normal"
		}
	}

	private func demoImage() -> URL {
		if let demoImageURL {
			return demoImageURL
		}
		let url = FileManager.default.temporaryDirectory.appendingPathComponent("morph-demo-image.png")
		DemoImageWriter.write(to: url)
		demoImageURL = url
		return url
	}

	private func invalidImageURL() -> URL {
		return FileManager.default.temporaryDirectory.appendingPathComponent("missing-demo-image.png")
	}

	@objc private func resetTapped() {
		renderFullDocument()
	}

	@objc private func streamTapped() {
		startStreaming()
	}

	@objc private func fullTapped() {
		renderFullDocument()
	}

	@objc private func tableDemoTapped() {
		renderTableDocument()
	}

	@objc private func themeTapped() {
		themeMode = (themeMode + 1) % 4
		applyTheme()
	}

	@objc private func tableTapped() {
		tableWrap.toggle()
		applyTheme()
	}

	@objc private func codeTapped() {
		compactCode.toggle()
		applyTheme()
	}
}
