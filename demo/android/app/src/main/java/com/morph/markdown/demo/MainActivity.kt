package com.morph.markdown.demo

import android.app.Activity
import android.graphics.Bitmap
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.ViewGroup
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
	private var chunkIndex = 0

	private val chunks = listOf(
		"# Streaming Markdown on Android\n\n",
		"Model text arrives in chunks. Inline formula: ",
		"\$e^{i\\pi}+1=0\$",
		" and display math follows.\n\n",
		"$$\\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}$$\n\n",
		"- [x] parse CommonMark/GFM\n",
		"- [ ] migrate renderer into production UI\n\n",
		"| feature | status |\n",
		"|---|---|\n",
		"| markdown-render IR | ok |\n",
		"| mathjax-c bitmap | ok |\n",
		"| dynamic table growth | ok |\n\n",
		"![demo image](file:///tmp/demo.png)\n"
	)

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		fontFile = copyFont()
		scroll = ScrollView(this)
		body = LinearLayout(this).apply {
			orientation = LinearLayout.VERTICAL
			setPadding(dp(20), dp(20), dp(20), dp(32))
		}
		scroll.addView(
			body,
			ViewGroup.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT,
				ViewGroup.LayoutParams.WRAP_CONTENT
			)
		)
		setContentView(scroll)
		render()
		pump()
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
			"math_block" -> parent.addView(mathView(node.optString("literal"), true))
			"image" -> parent.addView(imagePlaceholder(node))
			"code_block" -> parent.addView(codeBlock(node.optString("literal")))
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
		for (i in 0 until children.length()) {
			val item = children.getJSONObject(i)
			val literal = plainText(item).trim()
			val checked = literal.startsWith("[x]") || literal.startsWith("[X]")
			val unchecked = literal.startsWith("[ ]")
			val prefix = when {
				checked -> "- [x] "
				unchecked -> "- [ ] "
				else -> "- "
			}
			val bodyText = literal
				.removePrefix("[x]")
				.removePrefix("[X]")
				.removePrefix("[ ]")
				.trim()
			parent.addView(text(prefix + bodyText, 16f))
		}
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
		if (children == null) return row
		for (i in 0 until children.length()) {
			val child = children.getJSONObject(i)
			when (child.optString("kind")) {
				"text" -> row.addView(text(child.optString("literal"), 16f))
				"soft_break", "hard_break" -> row.addView(text("\n", 16f))
				"math_inline" -> row.addView(mathView(child.optString("literal"), false))
				"math_block" -> row.addView(mathView(child.optString("literal"), true))
				"image" -> row.addView(imagePlaceholder(child))
				else -> row.addView(text(plainText(child), 16f))
			}
		}
		return row
	}

	private fun table(node: JSONObject): TableLayout {
		val table = TableLayout(this)
		table.setPadding(0, dp(12), 0, dp(14))
		val rows = node.optJSONArray("children") ?: return table
		for (i in 0 until rows.length()) {
			val rowNode = rows.getJSONObject(i)
			val row = TableRow(this)
			val cells = rowNode.optJSONArray("children") ?: JSONArray()
			for (j in 0 until cells.length()) {
				val cell = text(plainText(cells.getJSONObject(j)).trim(), 15f)
				cell.setPadding(dp(12), dp(8), dp(12), dp(8))
				cell.background = border(i == 0)
				row.addView(cell)
			}
			table.addView(row)
		}
		return table
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

	private fun imagePlaceholder(node: JSONObject): TextView {
		val label = "image: " + node.optString("url", node.optString("literal"))
		return text(label, 14f).apply {
			setPadding(dp(12), dp(10), dp(12), dp(10))
			background = border(false)
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
