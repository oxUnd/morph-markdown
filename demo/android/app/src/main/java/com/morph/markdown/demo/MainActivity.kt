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
import android.widget.TableLayout
import android.widget.TableRow
import android.widget.TextView
import java.io.File
import org.json.JSONArray
import org.json.JSONObject

class MainActivity : Activity() {
	private val handler = Handler(Looper.getMainLooper())
	private val markdown = StringBuilder()
	private lateinit var body: LinearLayout
	private lateinit var scroll: ScrollView
	private lateinit var fontFile: File
	private lateinit var chunks: List<String>
	private var chunkIndex = 0
	private var insetTop = 0
	private var insetBottom = 0

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE)
		fontFile = copyFont()
		chunks = createChunks(createDemoImage())
		scroll = ScrollView(this).apply {
			clipToPadding = true
			setBackgroundColor(0xfffafaf7.toInt())
		}
		ViewCompat.setOnApplyWindowInsetsListener(scroll) { _, insets ->
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
		setContentView(scroll)
		scroll.requestApplyInsets()
		applySafePadding()
		render()
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
			"val engine = MarkdownNative.snapshot(markdown)\n",
			"val pixels = MarkdownNative.renderLatex(font, \"x^2\", false, size)\n",
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
		scroll.setPadding(0, insetTop, 0, insetBottom)
		body.setPadding(dp(20), dp(20), dp(20), dp(32))
	}

	private fun pump() {
		if (chunkIndex >= chunks.size) return
		markdown.append(chunks[chunkIndex++])
		render()
		handler.postDelayed({ pump() }, 420L)
	}

	private fun render() {
		body.removeAllViews()
		val raw = MarkdownNative.snapshot(markdown.toString())
		if (raw == null) {
			body.addView(text("snapshot failed", 16f))
			return
		}
		val root = JSONObject(raw)
		renderChildren(root.optJSONArray("children"), body)
		scroll.post { scroll.fullScroll(View.FOCUS_DOWN) }
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
				else parent.addView(text(node.optString("literal"), 16f))
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
		group.addView(text(prefix + firstParagraphText(item), 16f))
		val children = item.optJSONArray("children") ?: return group
		for (i in 0 until children.length()) {
			val child = children.getJSONObject(i)
			if (child.optString("kind") == "list") {
				val nested = LinearLayout(this).apply {
					orientation = LinearLayout.VERTICAL
					setPadding(dp(20), 0, 0, 0)
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
		return text(plainText(node), (26 - level * 2).toFloat()).apply {
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
		box.addView(text(plainText(node).trim(), 16f).apply {
			setPadding(dp(12), 0, 0, 0)
		})
		return box
	}

	private fun table(node: JSONObject): View {
		val scroller = HorizontalScrollView(this).apply {
			isHorizontalScrollBarEnabled = true
			overScrollMode = View.OVER_SCROLL_IF_CONTENT_SCROLLS
			setPadding(0, dp(12), 0, dp(14))
		}
		val table = TableLayout(this)
		val rows = node.optJSONArray("children") ?: return scroller
		for (i in 0 until rows.length()) {
			val rowNode = rows.getJSONObject(i)
			val row = TableRow(this)
			val cells = rowNode.optJSONArray("children") ?: JSONArray()
			for (j in 0 until cells.length()) {
				val cellNode = cells.getJSONObject(j)
				val cell = LinearLayout(this).apply {
					orientation = LinearLayout.HORIZONTAL
				}
				cell.setPadding(dp(12), dp(8), dp(12), dp(8))
				cell.background = border(i == 0)
				populateInline(cell, inlineChildrenOf(cellNode), true)
				row.addView(cell)
			}
			table.addView(row)
		}
		scroller.addView(
			table,
			ViewGroup.LayoutParams(
				ViewGroup.LayoutParams.WRAP_CONTENT,
				ViewGroup.LayoutParams.WRAP_CONTENT
			)
		)
		return scroller
	}

	private fun mathView(latex: String, display: Boolean): View {
		val fontSize = 16f * resources.displayMetrics.scaledDensity
		val data = MarkdownNative.renderLatex(fontFile.absolutePath, latex, display, fontSize)
		if (data == null || data.size < 3) return text(latex, 16f)
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
			return text("invalid image: $url", 14f).apply {
				setPadding(dp(12), dp(10), dp(12), dp(10))
				background = border(false)
			}
		}
		return ImageView(this).apply {
			setImageBitmap(bitmap)
			adjustViewBounds = true
			maxWidth = dp(320)
			maxHeight = dp(180)
			setPadding(0, dp(6), 0, dp(6))
		}
	}

	private fun inlineCode(code: String, tableCell: Boolean = false): TextView {
		return text(code, 15f).apply {
			typeface = Typeface.MONOSPACE
			if (tableCell) maxWidth = dp(220)
			setPadding(dp(5), dp(2), dp(5), dp(2))
			background = fill(0xffeeeeea.toInt())
		}
	}

	private fun cellText(value: String, tableCell: Boolean): TextView {
		return text(value, 16f).apply {
			if (tableCell) {
				maxWidth = dp(240)
				setSingleLine(false)
			}
		}
	}

	private fun codeBlock(code: String): TextView {
		return text(code, 14f).apply {
			typeface = Typeface.MONOSPACE
			setPadding(dp(12), dp(10), dp(12), dp(10))
			background = fill(0xffeeeeea.toInt())
		}
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
