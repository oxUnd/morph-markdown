import Foundation

enum DemoMarkdownDocument {
	static func fullJSON(validUri: String, invalidUri: String) -> String {
		resetIDs()
		return documentJSON(children: fullChildren(validUri: validUri, invalidUri: invalidUri))
	}

	static func streamingSnapshots(validUri: String, invalidUri: String) -> [String] {
		resetIDs()
		let children = fullChildren(validUri: validUri, invalidUri: invalidUri)
		let first = documentJSON(children: Array(children.prefix(2)))
		let second = documentJSON(children: Array(children.prefix(6)))
		let third = documentJSON(children: Array(children.prefix(10)))
		return [first, second, third]
	}

	private static var nextID = 1

	private static func resetIDs() {
		nextID = 1
	}

	private static func fullChildren(validUri: String, invalidUri: String) -> [[String: Any]] {
		[
			heading("Streaming Markdown on Android", level: 1),
			paragraph([
				text("Model text arrives in chunks. This paragraph has "),
				strong("bold"),
				text(", "),
				emphasis("emphasis"),
				text(", "),
				code("inline code"),
				text(", "),
				link("a link", url: "https://example.com"),
				text(", and inline math: "),
				math("e^{i\\pi}+1=0"),
				text(" and display math follows.")
			]),
			node("math_block", literal: "\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}"),
			heading("Lists and tasks", level: 2),
			orderedList([
				listItem([paragraph([text("Ordered item one")])]),
				listItem([
					paragraph([text("Ordered item two with nested bullets")]),
					bulletList([
						listItem([paragraph([text("nested bullet A")])]),
						listItem([paragraph([text("nested bullet B")])])
					])
				])
			]),
			heading("Marker styles", level: 2),
			bulletList([
				listItem([paragraph([text("level 1 uses disc marker")])]),
				listItem([
					paragraph([text("level 2 uses circle marker")]),
					bulletList([
						listItem([
							paragraph([text("level 3 uses square marker")]),
							bulletList([
								listItem([paragraph([text("level 4 cycles marker styles")])])
							])
						])
					])
				])
			]),
			task(checked: true, [text("parse CommonMark/GFM task lists")]),
			task(checked: false, [text("migrate renderer into production UI")]),
			task(checked: false, [text("preserve streaming updates on partial blocks")]),
			task(checked: false, [text("a long task item that wraps to a second line so checkbox alignment can be checked against the first line of text")]),
			quote([paragraph([text("A block quote can arrive while the model is still generating. It should stay readable and not collapse the layout.")])]),
			codeBlock("""
			val view\t= MorphMarkdownView(context)
			view.append(chunk, final = false)
			view.mathRenderer\t= MathJaxMathRenderer(context)
			"""),
			node("thematic_break"),
			heading("Dynamic table", level: 2),
			table([
				[text("feature"), text("status")],
				[text("CommonMark blocks"), text("ok")],
				[text("GFM tasklist"), text("ok")],
				[text("mathjax-c bitmap"), text("ok")],
				[text("dynamic table growth"), text("ok")],
				[inline([text("inline formula "), math("a^2+b^2=c^2")]), text("rendered in cell")],
				[inline([text("valid image "), image(validUri)]), text("decoded bitmap in cell")],
				[inline([text("invalid image "), image(invalidUri)]), text("error placeholder in cell")],
				[inline([text("code "), code("cell.value()"), text(" and "), link("link", url: "https://example.com")]), text("mixed inline")],
				[text("tab text"), text("key\tvalue\twith tabs")],
				[text("long cell"), text("this is a deliberately long table value that should wrap inside the cell while the table can still scroll horizontally")]
			]),
			paragraph([text("HTML sample: <span>treated as text by policy</span>")]),
			paragraph([text("Valid image:")]),
			image(validUri),
			paragraph([text("Invalid image:")]),
			image(invalidUri),
			heading("中文排版 HetiLike", level: 1),
			paragraph([
				text("这是一段用于检查中文阅读排版的长段落。今天发布 MorphMarkdown v1.0，"),
				text("它支持 Android/iOS、MathJax-C、GFM table 和 task list。中文与 English、"),
				text("数字 12345、"),
				code("inlineCode()"),
				text("、"),
				link("链接", url: "https://example.com"),
				text(" 混排时，需要保持舒适的行高、合理的中西文间距，以及不会挤在一起的视觉节奏。")
			]),
			heading("二级标题：中文标题间距", level: 2),
			paragraph([
				text("中文段落通常比英文更依赖稳定的行高和段落节奏。HetiLike 主题会使用 16sp 正文、"),
				text("约 1.5 倍行高、偏阅读型的标题层级，并让表格、列表、引用都跟正文网格靠齐。")
			]),
			heading("三级标题：列表与任务", level: 3),
			bulletList([
				listItem([paragraph([text("第一层无序列表使用实心圆，适合普通条目。")])]),
				listItem([
					paragraph([text("第二层列表使用空心圆，用于补充说明。")]),
					bulletList([
						listItem([paragraph([text("第三层列表使用方块，层级需要清楚但不能太重。")])])
					])
				]),
				listItem([paragraph([text("中英混排 item：MorphMarkdown 渲染中文、English 和 2026 年的数字。")])])
			]),
			orderedList([
				listItem([paragraph([text("有序列表第一项，文本比较长时应该自然换行。")])]),
				listItem([
					paragraph([text("有序列表第二项，包含嵌套项目。")]),
					bulletList([
						listItem([paragraph([text("嵌套项目 A：中文与 Android 混排。")])]),
						listItem([paragraph([text("嵌套项目 B：中文与 iOS 混排。")])])
					])
				])
			]),
			task(checked: true, [text("支持中文 HetiLike 主题")]),
			task(checked: false, [text("检查中文 task list 的 checkbox 与文字基线")]),
			task(checked: false, [text("这是一条很长的中文任务，用于验证多行换行后，复选框仍然对齐第一行文字而不是挤到中间")]),
			quote([paragraph([text("引用块也需要中文阅读节奏。它不应该像代码块一样紧凑，而是应该保留足够呼吸感，并且在流式输出时保持布局稳定。")])]),
			codeBlock("中文代码块不参与中西文 spacing：MorphMarkdown\tHetiLike\t中文排版\n"),
			heading("中文表格", level: 2),
			table([
				[text("场景"), text("效果")],
				[text("中文长文本"), text("这是一段故意写得比较长的中文表格内容，用来验证单元格换行、行高、padding 和横向滚动是否仍然稳定。")],
				[inline([text("表格内公式")]), inline([text("勾股定理 "), math("a^2+b^2=c^2"), text(" 应该可以在中文单元格中渲染。")])],
				[inline([text("表格内高公式")]), inline([text("求根公式 "), math("\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}"), text(" 只应该撑高当前行，后面的中文仍然保持稳定换行。")])],
				[text("表格内有效图片"), image(validUri)],
				[text("表格内无效图片"), image(invalidUri)],
				[text("中英混排"), text("MorphMarkdown SDK 在 Android 和 iOS 上复用同一份 IR。")]
			]),
			node("thematic_break"),
			heading("四级标题", level: 4),
			paragraph([
				text("收尾段落：中文排版不只是换字体，还包括字号、行高、段落间距、列表缩进、"),
				text("表格 padding、代码字号和混排 spacing 的整体组合。")
			])
		]
	}

	private static func documentJSON(children: [[String: Any]]) -> String {
		let root: [String: Any] = ["kind": "document", "children": children]
		let data = try! JSONSerialization.data(withJSONObject: root, options: [])
		return String(data: data, encoding: .utf8)!
	}

	private static func node(
		_ kind: String,
		literal: String = "",
		level: Int = 1,
		checked: Bool = false,
		listType: String = "bullet",
		start: Int = 1,
		url: String = "",
		children: [[String: Any]] = []
	) -> [String: Any] {
		defer { nextID += 1 }
		return [
			"id": nextID,
			"kind": kind,
			"literal": literal,
			"level": level,
			"checked": checked,
			"list_type": listType,
			"start": start,
			"url": url,
			"children": children
		]
	}

	private static func heading(_ literal: String, level: Int) -> [String: Any] {
		node("heading", level: level, children: [text(literal)])
	}

	private static func paragraph(_ children: [[String: Any]]) -> [String: Any] {
		node("paragraph", children: children)
	}

	private static func text(_ literal: String) -> [String: Any] {
		node("text", literal: literal)
	}

	private static func code(_ literal: String) -> [String: Any] {
		node("code", literal: literal)
	}

	private static func strong(_ literal: String) -> [String: Any] {
		node("strong", children: [text(literal)])
	}

	private static func emphasis(_ literal: String) -> [String: Any] {
		node("emphasis", children: [text(literal)])
	}

	private static func link(_ literal: String, url: String) -> [String: Any] {
		node("link", url: url, children: [text(literal)])
	}

	private static func math(_ literal: String) -> [String: Any] {
		node("math_inline", literal: literal)
	}

	private static func codeBlock(_ literal: String) -> [String: Any] {
		node("code_block", literal: literal)
	}

	private static func image(_ url: String) -> [String: Any] {
		node("image", url: url)
	}

	private static func bulletList(_ items: [[String: Any]]) -> [String: Any] {
		node("list", listType: "bullet", children: items)
	}

	private static func orderedList(_ items: [[String: Any]]) -> [String: Any] {
		node("list", listType: "ordered", children: items)
	}

	private static func listItem(_ children: [[String: Any]]) -> [String: Any] {
		node("list_item", children: children)
	}

	private static func task(checked: Bool, _ children: [[String: Any]]) -> [String: Any] {
		node("tasklist", checked: checked, children: [paragraph(children)])
	}

	private static func table(_ rows: [[[String: Any]]]) -> [String: Any] {
		node("table", children: rows.map { row in
			node("table_row", children: row.map { cellChildren in
				node("table_cell", children: [cellChildren])
			})
		})
	}

	private static func inline(_ children: [[String: Any]]) -> [String: Any] {
		paragraph(children)
	}

	private static func quote(_ children: [[String: Any]]) -> [String: Any] {
		node("block_quote", children: children)
	}
}
