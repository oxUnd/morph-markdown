package com.morph.markdown.demo

import android.app.Activity
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import android.widget.CheckBox
import android.widget.HorizontalScrollView
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import java.io.File
import org.json.JSONArray
import org.json.JSONObject

class MainActivity : Activity() {
	private val handler = Handler(Looper.getMainLooper())
	private val markdown = StringBuilder()
	private lateinit var root: LinearLayout
	private lateinit var body: LinearLayout
	private lateinit var controls: LinearLayout
	private lateinit var scroll: ScrollView
	private lateinit var fontFile: File
	private lateinit var chunks: List<String>
	private var chunkIndex = 0
	private var insetTop = 0
	private var insetBottom = 0
	private var headingMode = 0
	private var tabMode = 1
	private var tableWrap = true
	private var compactCode = false
	private var renderConfig = MarkdownRenderPresets.Normal

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE)
		fontFile = copyFont()
		chunks = createChunks(createDemoImage())
		root = LinearLayout(this).apply {
			orientation = LinearLayout.VERTICAL
			setBackgroundColor(0xfffafaf7.toInt())
		}
		controls = LinearLayout(this).apply {
			orientation = LinearLayout.HORIZONTAL
		}
		scroll = ScrollView(this).apply {
			clipToPadding = true
			setBackgroundColor(0xfffafaf7.toInt())
		}
		ViewCompat.setOnApplyWindowInsetsListener(root) { _, insets ->
			val bars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
			insetTop = bars.top
			insetBottom = bars.bottom
			applySafePadding()
			insets
		}
		body = LinearLayout(this).apply {
			orientation = LinearLayout.VERTICAL
		}
		scroll.addView(
			body,
			ViewGroup.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT,
				ViewGroup.LayoutParams.WRAP_CONTENT
			)
		)
		root.addView(
			controls,
			LinearLayout.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT,
				ViewGroup.LayoutParams.WRAP_CONTENT
			)
		)
		root.addView(
			scroll,
			LinearLayout.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT,
				0,
				1f
			)
		)
		setContentView(root)
		root.requestApplyInsets()
		applySafePadding()
		rebuildControls()
		render(false)
		pump()
	}

	private fun createChunks(validImage: File): List<String> {
		val validUri = "file://${validImage.absolutePath}"
		val invalidUri = "file://${File(cacheDir, "missing-demo-image.png").absolutePath}"
		return listOf(
			"# Streaming Markdown on Android\n\n",
			"Model text arrives in chunks. This paragraph has **bold**, *emphasis*, ",
			"`inline code`, [a link](https://example.com), and inline math: ",
			"\$e^{i\\pi}+1=0\$",
			" and display math follows.\n\n",
			"$$\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}$$\n\n",
			"## Lists and tasks\n\n",
			"1. Ordered item one\n",
			"2. Ordered item two with nested bullets\n",
			"   - nested bullet A\n",
			"   - nested bullet B\n\n",
			"- [x] parse CommonMark/GFM task lists\n",
			"- [ ] migrate renderer into production UI\n",
			"- [ ] preserve streaming updates on partial blocks\n\n",
			"> A block quote can arrive while the model is still generating. ",
			"It should stay readable and not collapse the layout.\n\n",
			"```kotlin\n",
			"val engine\t= MarkdownNative.snapshot(markdown)\n",
			"val pixels\t= MarkdownNative.renderLatex(font, \"x^2\", false, size)\n",
			"```\n\n",
			"---\n\n",
			"## Dynamic table\n\n",
			"| feature | status |\n",
			"|:---|:---:|\n",
			"| CommonMark blocks | ok |\n",
			"| GFM tasklist | ok |\n",
			"| mathjax-c bitmap | ok |\n",
			"| dynamic table growth | ok |\n",
			"| inline formula \$a^2+b^2=c^2\$ | rendered in cell |\n",
			"| valid image ![generated]($validUri) | decoded bitmap in cell |\n",
			"| invalid image ![missing]($invalidUri) | error placeholder in cell |\n",
			"| code `cell.value()` and [link](https://example.com) | mixed inline |\n",
			"| tab text | key\tvalue\twith tabs |\n",
			"| long cell | this is a deliberately long table value that should wrap inside the cell while the table can still scroll horizontally |\n\n",
			"HTML sample: <span>treated as text by policy</span>\n\n",
			"Valid image:\n\n![generated demo]($validUri \"generated\")\n\n",
			"Invalid image:\n\n![missing demo]($invalidUri \"missing\")\n"
		)
	}

	private fun createDemoImage(): File {
		val out = File(cacheDir, "markdown-render-generated.png")
		val bitmap = Bitmap.createBitmap(640, 360, Bitmap.Config.ARGB_8888)
		val canvas = Canvas(bitmap)
		val paint = Paint(Paint.ANTI_ALIAS_FLAG)
		canvas.drawColor(Color.rgb(247, 248, 244))
		paint.color = Color.rgb(32, 33, 33)
		paint.textSize = 42f
		paint.typeface = Typeface.DEFAULT_BOLD
		canvas.drawText("markdown-render", 40f, 76f, paint)
		paint.typeface = Typeface.MONOSPACE
		paint.textSize = 30f
		canvas.drawText("generated PNG", 40f, 124f, paint)
		paint.style = Paint.Style.STROKE
		paint.strokeWidth = 4f
		paint.color = Color.rgb(70, 70, 70)
		canvas.drawRoundRect(RectF(40f, 160f, 600f, 310f), 16f, 16f, paint)
		paint.style = Paint.Style.FILL
		paint.textSize = 28f
		canvas.drawText("| markdown | image |", 70f, 216f, paint)
		canvas.drawText("| formula  | table |", 70f, 266f, paint)
		out.outputStream().use { stream ->
			bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream)
		}
		bitmap.recycle()
		return out
	}

	private fun applySafePadding() {
		controls.setPadding(dp(12), insetTop + dp(8), dp(12), dp(8))
		scroll.setPadding(0, 0, 0, insetBottom)
		body.setPadding(dp(20), dp(16), dp(20), dp(32))
	}

	private fun pump() {
		if (chunkIndex >= chunks.size) return
		markdown.append(chunks[chunkIndex++])
		render(true)
		handler.postDelayed({ pump() }, 420L)
	}

	private fun rebuildControls() {
		renderConfig = currentConfig()
		controls.removeAllViews()
		controls.addView(control("H ${headingLabel()}") {
			headingMode = (headingMode + 1) % 3
			rebuildControls()
			render(false)
		})
		controls.addView(control("Tab ${tabSize()}") {
			tabMode = (tabMode + 1) % 3
			rebuildControls()
			render(false)
		})
		controls.addView(control(if (tableWrap) "Table wrap" else "Table nowrap") {
			tableWrap = !tableWrap
			rebuildControls()
			render(false)
		})
		controls.addView(control(if (compactCode) "Code compact" else "Code normal") {
			compactCode = !compactCode
			rebuildControls()
			render(false)
		})
	}

	private fun currentConfig(): MarkdownRenderConfig {
		var config = when (headingMode) {
			1 -> MarkdownRenderPresets.LargeHeadings
			2 -> MarkdownRenderPresets.CompactHeadings
			else -> MarkdownRenderPresets.Normal
		}
		val tabs = tabSize()
		config = config.copy(tabSize = tabs, codeBlockTabSize = tabs)
		if (!tableWrap) config = config.copy(tableCellWrap = false)
		if (compactCode) {
			config = config.copy(codeBlockTextSizeSp = 13f, inlineCodeTextSizeSp = 14f)
		}
		return config
	}

	private fun render(autoScroll: Boolean) {
		body.removeAllViews()
		val raw = MarkdownNative.snapshot(markdown.toString())
		if (raw == null) {
			body.addView(text("snapshot failed", renderConfig.bodyTextSizeSp))
			return
		}
		val root = JSONObject(raw)
		renderChildren(root.optJSONArray("children"), body)
		if (autoScroll) scroll.post { scroll.fullScroll(View.FOCUS_DOWN) }
	}

	private fun renderChildren(children: JSONArray?, parent: LinearLayout) {
		if (children == null) return
		for (i in 0 until children.length()) {
			renderBlock(children.getJSONObject(i), parent)
		}
	}

	private fun renderBlock(node: JSONObject, parent: LinearLayout) {
		when (node.optString("kind")) {
			"heading" -> parent.addView(heading(node))
			"paragraph" -> parent.addView(inlineGroup(node.optJSONArray("children")))
			"list" -> renderList(node, parent)
			"table" -> parent.addView(table(node))
			"block_quote" -> parent.addView(blockQuote(node))
			"math_block" -> parent.addView(mathView(node.optString("literal"), true))
			"image" -> parent.addView(imageView(node))
			"code_block" -> parent.addView(codeBlock(node.optString("literal")))
			"thematic_break" -> parent.addView(rule())
			"soft_break", "hard_break" -> parent.addView(spacer(4))
			else -> {
				val children = node.optJSONArray("children")
				if (children != null) renderChildren(children, parent)
				else parent.addView(text(expandTabs(node.optString("literal"), renderConfig.tabSize), renderConfig.bodyTextSizeSp))
			}
		}
	}

	private fun renderList(node: JSONObject, parent: LinearLayout) {
		val children = node.optJSONArray("children") ?: return
		val ordered = node.optString("list_type") == "ordered"
		var number = node.optInt("start", 1)
		for (i in 0 until children.length()) {
			val item = children.getJSONObject(i)
			if (item.optString("kind") == "tasklist") {
				parent.addView(taskItem(item))
			} else {
				parent.addView(listItem(item, if (ordered) "${number++}. " else "- "))
			}
		}
	}

	private fun listItem(item: JSONObject, prefix: String): LinearLayout {
		val group = LinearLayout(this).apply {
			orientation = LinearLayout.VERTICAL
			setPadding(0, 0, 0, dp(2))
		}
		group.addView(text(expandTabs(prefix + firstParagraphText(item), renderConfig.tabSize), renderConfig.bodyTextSizeSp))
		val children = item.optJSONArray("children") ?: return group
		for (i in 0 until children.length()) {
			val child = children.getJSONObject(i)
			if (child.optString("kind") == "list") {
				val nested = LinearLayout(this).apply {
					orientation = LinearLayout.VERTICAL
					setPadding(dp(renderConfig.listIndentDp), 0, 0, 0)
				}
				renderList(child, nested)
				group.addView(nested)
			}
		}
		return group
	}

	private fun firstParagraphText(item: JSONObject): String {
		val children = item.optJSONArray("children") ?: return plainText(item).trim()
		for (i in 0 until children.length()) {
			val child = children.getJSONObject(i)
			if (child.optString("kind") == "paragraph")
				return plainText(child).trim()
		}
		return plainText(item).trim()
	}

	private fun taskItem(item: JSONObject): LinearLayout {
		val row = LinearLayout(this).apply {
			orientation = LinearLayout.HORIZONTAL
			setPadding(0, 0, 0, dp(2))
		}
		row.addView(CheckBox(this).apply {
			isChecked = item.optBoolean("checked", false)
			isEnabled = false
			minWidth = 0
			minHeight = 0
			setPadding(0, 0, dp(4), 0)
		})
		row.addView(inlineGroup(inlineChildrenOf(item)))
		return row
	}

	private fun inlineChildrenOf(node: JSONObject): JSONArray? {
		val children = node.optJSONArray("children") ?: return null
		for (i in 0 until children.length()) {
			val child = children.getJSONObject(i)
			if (child.optString("kind") == "paragraph")
				return child.optJSONArray("children")
		}
		return children
	}

	private fun heading(node: JSONObject): TextView {
		val level = node.optInt("level", 1).coerceIn(1, 6)
		return text(plainText(node), renderConfig.headingSize(level)).apply {
			typeface = Typeface.DEFAULT_BOLD
			setPadding(0, dp(8), 0, dp(10))
		}
	}

	private fun inlineGroup(children: JSONArray?): View {
		val row = InlineLayout(this)
		row.setPadding(0, dp(4), 0, dp(10))
		populateInline(row, children, false)
		return row
	}

	private fun populateInline(row: ViewGroup, children: JSONArray?, tableCell: Boolean) {
		if (children == null) return
		for (i in 0 until children.length()) {
			val child = children.getJSONObject(i)
			when (child.optString("kind")) {
				"text" -> row.addView(cellText(child.optString("literal"), tableCell))
				"code" -> row.addView(inlineCode(child.optString("literal"), tableCell))
				"soft_break", "hard_break" -> row.addView(cellText("\n", tableCell))
				"math_inline" -> row.addView(mathView(child.optString("literal"), false))
				"math_block" -> row.addView(mathView(child.optString("literal"), true))
				"image" -> row.addView(imageView(child))
				else -> row.addView(cellText(plainText(child), tableCell))
			}
		}
	}

	private fun blockQuote(node: JSONObject): View {
		val box = LinearLayout(this).apply {
			orientation = LinearLayout.HORIZONTAL
			setPadding(0, dp(6), 0, dp(12))
		}
		box.addView(View(this).apply {
			background = fill(0xff767676.toInt())
			layoutParams = LinearLayout.LayoutParams(dp(4), ViewGroup.LayoutParams.MATCH_PARENT)
		})
		box.addView(text(expandTabs(plainText(node).trim(), renderConfig.tabSize), renderConfig.bodyTextSizeSp).apply {
			setPadding(dp(renderConfig.blockquoteIndentDp), 0, 0, 0)
		})
		return box
	}

	private fun table(node: JSONObject): View {
		val scroller = HorizontalScrollView(this).apply {
			isHorizontalScrollBarEnabled = true
			overScrollMode = View.OVER_SCROLL_IF_CONTENT_SCROLLS
			setPadding(0, dp(12), 0, dp(14))
		}
		val table = MarkdownTableView(this, renderConfig)
		val rows = node.optJSONArray("children") ?: return scroller
		for (i in 0 until rows.length()) {
			val rowNode = rows.getJSONObject(i)
			val cells = rowNode.optJSONArray("children") ?: JSONArray()
			table.beginRow(i == 0)
			for (j in 0 until cells.length()) {
				val cellNode = cells.getJSONObject(j)
				val cell = LinearLayout(this).apply {
					orientation = LinearLayout.HORIZONTAL
				}
				cell.setPadding(dp(12), dp(8), dp(12), dp(8))
				populateInline(cell, inlineChildrenOf(cellNode), true)
				table.addCell(cell)
			}
		}
		table.layoutParams = ViewGroup.LayoutParams(
			ViewGroup.LayoutParams.WRAP_CONTENT,
			ViewGroup.LayoutParams.WRAP_CONTENT
		)
		if (!renderConfig.tableHorizontalScroll) return table
		scroller.addView(table)
		return scroller
	}

	private fun mathView(latex: String, display: Boolean): View {
		val fontSize = renderConfig.mathSize() * resources.displayMetrics.scaledDensity
		val data = MarkdownNative.renderLatex(fontFile.absolutePath, latex, display, fontSize)
		if (data == null || data.size < 3) return text(latex, renderConfig.bodyTextSizeSp)
		val width = data[0]
		val height = data[1]
		val pixels = data.copyOfRange(2, data.size)
		val bitmap = Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888)
		return ImageView(this).apply {
			setImageBitmap(bitmap)
			adjustViewBounds = true
			setPadding(0, dp(if (display) 12 else 0), 0, dp(if (display) 12 else 0))
		}
	}

	private fun imageView(node: JSONObject): View {
		val url = node.optString("url", node.optString("literal"))
		val path = if (url.startsWith("file://")) url.removePrefix("file://") else url
		val bitmap = BitmapFactory.decodeFile(path)
		if (bitmap == null) {
			return text("invalid image: $url", renderConfig.inlineCodeTextSizeSp).apply {
				setPadding(dp(12), dp(10), dp(12), dp(10))
				background = border(false)
			}
		}
		return ImageView(this).apply {
			setImageBitmap(bitmap)
			adjustViewBounds = true
			maxWidth = dp(renderConfig.imageMaxWidthDp)
			maxHeight = dp(renderConfig.imageMaxHeightDp)
			setPadding(0, dp(6), 0, dp(6))
		}
	}

	private fun inlineCode(code: String, tableCell: Boolean = false): TextView {
		return text(expandTabs(code, renderConfig.tabSize), renderConfig.inlineCodeTextSizeSp).apply {
			typeface = Typeface.MONOSPACE
			if (tableCell && renderConfig.tableCellWrap) {
				maxWidth = dp((renderConfig.tableCellMaxWidthDp * 0.78f).toInt())
			}
			setPadding(dp(5), dp(2), dp(5), dp(2))
			background = fill(0xffeeeeea.toInt())
		}
	}

	private fun cellText(value: String, tableCell: Boolean): TextView {
		return text(expandTabs(value, renderConfig.tabSize), renderConfig.bodyTextSizeSp).apply {
			if (tableCell && renderConfig.tableCellWrap) {
				maxWidth = dp((renderConfig.tableCellMaxWidthDp * 0.86f).toInt())
				setSingleLine(false)
			} else if (tableCell) {
				setSingleLine(true)
			}
		}
	}

	private fun codeBlock(code: String): TextView {
		return text(expandTabs(code, renderConfig.codeBlockTabSize), renderConfig.codeBlockTextSizeSp).apply {
			typeface = Typeface.MONOSPACE
			setPadding(dp(12), dp(10), dp(12), dp(10))
			background = fill(0xffeeeeea.toInt())
		}
	}

	private fun control(label: String, action: () -> Unit): TextView {
		return text(label, 13f).apply {
			setTextColor(0xff202020.toInt())
			setPadding(dp(10), dp(7), dp(10), dp(7))
			background = border(false)
			setOnClickListener { action() }
			layoutParams = LinearLayout.LayoutParams(
				ViewGroup.LayoutParams.WRAP_CONTENT,
				ViewGroup.LayoutParams.WRAP_CONTENT
			).apply {
				setMargins(0, 0, dp(8), 0)
			}
		}
	}

	private fun headingLabel(): String {
		return when (headingMode) {
			1 -> "large"
			2 -> "compact"
			else -> "normal"
		}
	}

	private fun tabSize(): Int {
		return when (tabMode) {
			0 -> 2
			2 -> 8
			else -> 4
		}
	}

	private fun expandTabs(value: String, tabSize: Int): String {
		if (!value.contains('\t')) return value
		val out = StringBuilder(value.length)
		var col = 0
		for (ch in value) {
			when (ch) {
				'\t' -> {
					val spaces = tabSize - (col % tabSize)
					repeat(spaces) {
						out.append(' ')
						col += 1
					}
				}
				'\n' -> {
					out.append(ch)
					col = 0
				}
				else -> {
					out.append(ch)
					col += 1
				}
			}
		}
		return out.toString()
	}

	private fun plainText(node: JSONObject): String {
		val literal = node.optString("literal", "")
		val children = node.optJSONArray("children") ?: return literal
		val out = StringBuilder(literal)
		for (i in 0 until children.length()) {
			out.append(plainText(children.getJSONObject(i)))
		}
		return out.toString()
	}

	private fun text(value: String, sizeSp: Float): TextView {
		return TextView(this).apply {
			text = value
			textSize = sizeSp
			setTextColor(0xff1b1b1b.toInt())
			includeFontPadding = true
			setLineSpacing(dp(2).toFloat(), 1.0f)
		}
	}

	private fun spacer(height: Int): View {
		return View(this).apply {
			layoutParams = LinearLayout.LayoutParams(1, dp(height))
		}
	}

	private fun rule(): View {
		return View(this).apply {
			background = fill(0xffb7b7b0.toInt())
			layoutParams = LinearLayout.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT,
				dp(1)
			).apply {
				setMargins(0, dp(14), 0, dp(14))
			}
		}
	}

	private fun border(header: Boolean): GradientDrawable {
		return GradientDrawable().apply {
			setColor(if (header) 0xffefefea.toInt() else 0x00ffffff)
			setStroke(dp(1), 0xff454545.toInt())
		}
	}

	private fun fill(color: Int): GradientDrawable {
		return GradientDrawable().apply {
			setColor(color)
			cornerRadius = dp(4).toFloat()
		}
	}

	private fun copyFont(): File {
		val out = File(filesDir, "STIXTwoMath-Regular.ttf")
		if (!out.exists()) {
			assets.open("STIXTwoMath-Regular.ttf").use { input ->
				out.outputStream().use { output -> input.copyTo(output) }
			}
		}
		return out
	}

	private fun dp(value: Int): Int {
		return (value * resources.displayMetrics.density + 0.5f).toInt()
	}
}

class InlineLayout(context: android.content.Context) : ViewGroup(context) {
	override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
		val maxWidth = MeasureSpec.getSize(widthMeasureSpec)
		var x = paddingLeft
		var y = paddingTop
		var lineHeight = 0
		for (i in 0 until childCount) {
			val child = getChildAt(i)
			measureChild(child, widthMeasureSpec, heightMeasureSpec)
			if (x + child.measuredWidth > maxWidth - paddingRight && x > paddingLeft) {
				x = paddingLeft
				y += lineHeight
				lineHeight = 0
			}
			x += child.measuredWidth
			lineHeight = maxOf(lineHeight, child.measuredHeight)
		}
		val height = y + lineHeight + paddingBottom
		setMeasuredDimension(maxWidth, resolveSize(height, heightMeasureSpec))
	}

	override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
		val maxWidth = r - l
		var x = paddingLeft
		var y = paddingTop
		var lineHeight = 0
		for (i in 0 until childCount) {
			val child = getChildAt(i)
			if (x + child.measuredWidth > maxWidth - paddingRight && x > paddingLeft) {
				x = paddingLeft
				y += lineHeight
				lineHeight = 0
			}
			val top = y + (lineHeight - child.measuredHeight).coerceAtLeast(0) / 2
			child.layout(x, top, x + child.measuredWidth, top + child.measuredHeight)
			x += child.measuredWidth
			lineHeight = maxOf(lineHeight, child.measuredHeight)
		}
	}
}

class MarkdownTableView(
	context: android.content.Context,
	private val config: MarkdownRenderConfig
) : ViewGroup(context) {
	private data class Cell(val row: Int, val col: Int, val header: Boolean, val view: View)

	private val cells = mutableListOf<Cell>()
	private val colWidths = mutableListOf<Int>()
	private val rowHeights = mutableListOf<Int>()
	private val rowHeader = mutableListOf<Boolean>()
	private val linePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
		color = 0xff454545.toInt()
		strokeWidth = context.resources.displayMetrics.density * 1.5f
	}
	private val headerPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
		color = 0xffefefea.toInt()
		style = Paint.Style.FILL
	}
	private var currentRow = -1
	private var currentCol = 0

	fun beginRow(header: Boolean) {
		currentRow += 1
		currentCol = 0
		rowHeader.add(header)
	}

	fun addCell(view: View) {
		cells.add(Cell(currentRow, currentCol, rowHeader[currentRow], view))
		addView(view)
		currentCol += 1
	}

	override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
		val cols = cells.maxOfOrNull { it.col + 1 } ?: 0
		val rows = cells.maxOfOrNull { it.row + 1 } ?: 0
		colWidths.clear()
		rowHeights.clear()
		repeat(cols) { colWidths.add(0) }
		repeat(rows) { rowHeights.add(0) }

		for (cell in cells) {
			val widthSpec = if (config.tableCellWrap) {
				MeasureSpec.makeMeasureSpec(cellMaxWidth(), MeasureSpec.AT_MOST)
			} else {
				MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED)
			}
			measureChild(
				cell.view,
				widthSpec,
				heightMeasureSpec
			)
			colWidths[cell.col] = maxOf(colWidths[cell.col], cell.view.measuredWidth)
			rowHeights[cell.row] = maxOf(rowHeights[cell.row], cell.view.measuredHeight)
		}

		val width = colWidths.sum()
		val height = rowHeights.sum()
		setMeasuredDimension(
			resolveSize(width, widthMeasureSpec),
			resolveSize(height, heightMeasureSpec)
		)
	}

	override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
		val rowTops = IntArray(rowHeights.size)
		val colLefts = IntArray(colWidths.size)
		for (i in 1 until rowTops.size)
			rowTops[i] = rowTops[i - 1] + rowHeights[i - 1]
		for (i in 1 until colLefts.size)
			colLefts[i] = colLefts[i - 1] + colWidths[i - 1]

		for (cell in cells) {
			val left = colLefts[cell.col]
			val top = rowTops[cell.row]
			val width = colWidths[cell.col]
			val height = rowHeights[cell.row]
			cell.view.layout(left, top, left + width, top + height)
		}
	}

	override fun dispatchDraw(canvas: Canvas) {
		for (row in rowHeights.indices) {
			if (rowHeader.getOrNull(row) == true) {
				val top = rowHeights.take(row).sum().toFloat()
				canvas.drawRect(0f, top, measuredWidth.toFloat(), top + rowHeights[row], headerPaint)
			}
		}
		super.dispatchDraw(canvas)
		drawGrid(canvas)
	}

	private fun drawGrid(canvas: Canvas) {
		var x = 0
		canvas.drawLine(0f, 0f, measuredWidth.toFloat(), 0f, linePaint)
		for (width in colWidths) {
			canvas.drawLine(x.toFloat(), 0f, x.toFloat(), measuredHeight.toFloat(), linePaint)
			x += width
		}
		canvas.drawLine(x.toFloat(), 0f, x.toFloat(), measuredHeight.toFloat(), linePaint)

		var y = 0
		for (height in rowHeights) {
			canvas.drawLine(0f, y.toFloat(), measuredWidth.toFloat(), y.toFloat(), linePaint)
			y += height
		}
		canvas.drawLine(0f, y.toFloat(), measuredWidth.toFloat(), y.toFloat(), linePaint)
	}

	private fun cellMaxWidth(): Int {
		return (context.resources.displayMetrics.density * config.tableCellMaxWidthDp + 0.5f).toInt()
	}
}
