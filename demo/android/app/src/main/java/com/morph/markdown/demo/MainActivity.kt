package com.morph.markdown.demo

import android.app.Activity
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.HorizontalScrollView
import android.widget.LinearLayout
import android.widget.TextView
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import com.morph.markdown.FileImageLoader
import com.morph.markdown.MathJaxMathRenderer
import com.morph.markdown.MorphMarkdownTheme
import com.morph.markdown.MorphMarkdownThemes
import com.morph.markdown.MorphMarkdownView
import java.io.File

class MainActivity : Activity() {
	private val handler = Handler(Looper.getMainLooper())
	private lateinit var root: LinearLayout
	private lateinit var controls: LinearLayout
	private lateinit var controlsHost: HorizontalScrollView
	private lateinit var markdownView: MorphMarkdownView
	private lateinit var chunks: List<StreamChunk>
	private var chunkIndex = 0
	private var insetTop = 0
	private var insetBottom = 0
	private var headingMode = 3
	private var tabMode = 1
	private var tableWrap = true
	private var compactCode = false
	private var assetFont = true

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE)
		chunks = createChunks(createDemoImage())
		createRoot()
		setContentView(root)
		root.requestApplyInsets()
		rebuildControls()
		markdownView.theme = currentTheme()
		pump()
	}

	override fun onDestroy() {
		handler.removeCallbacksAndMessages(null)
		markdownView.close()
		super.onDestroy()
	}

	private fun createRoot() {
		root = LinearLayout(this).apply {
			orientation = LinearLayout.VERTICAL
			setBackgroundColor(0xfffafaf7.toInt())
		}
		controls = LinearLayout(this).apply {
			orientation = LinearLayout.HORIZONTAL
		}
		controlsHost = HorizontalScrollView(this).apply {
			isHorizontalScrollBarEnabled = false
			addView(controls, ViewGroup.LayoutParams(-2, -2))
		}
		markdownView = MorphMarkdownView(this).apply {
			setBackgroundColor(0xfffafaf7.toInt())
			setPadding(0, 0, 0, insetBottom)
			mathRenderer = MathJaxMathRenderer(this@MainActivity)
			imageLoader = FileImageLoader()
		}
		root.addView(controlsHost, LinearLayout.LayoutParams(-1, -2))
		root.addView(markdownView, LinearLayout.LayoutParams(-1, 0, 1f))
		ViewCompat.setOnApplyWindowInsetsListener(root) { _, insets ->
			val bars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
			insetTop = bars.top
			insetBottom = bars.bottom
			applySafePadding()
			insets
		}
	}

	private fun applySafePadding() {
		controls.setPadding(dp(12), insetTop + dp(8), dp(12), dp(8))
		markdownView.setPadding(dp(20), dp(16), dp(20), insetBottom + dp(32))
	}

	private fun pump() {
		if (chunkIndex >= chunks.size) return
		val final = chunkIndex == chunks.lastIndex
		val chunk = chunks[chunkIndex++]
		markdownView.append(chunk.text, final)
		handler.postDelayed({ pump() }, chunk.delayMs)
	}

	private fun rebuildControls() {
		controls.removeAllViews()
		controls.addView(control("H ${headingLabel()}") {
			headingMode = (headingMode + 1) % 5
			applyTheme()
		})
		controls.addView(control("Tab ${tabSize()}") {
			tabMode = (tabMode + 1) % 3
			applyTheme()
		})
		controls.addView(control(if (tableWrap) "Table wrap" else "Table nowrap") {
			tableWrap = !tableWrap
			applyTheme()
		})
		controls.addView(control(if (compactCode) "Code compact" else "Code normal") {
			compactCode = !compactCode
			applyTheme()
		})
		controls.addView(control(if (assetFont) "Font asset" else "Font system") {
			assetFont = !assetFont
			applyTheme()
		})
	}

	private fun applyTheme() {
		rebuildControls()
		markdownView.theme = currentTheme()
	}

	private fun currentTheme(): MorphMarkdownTheme {
		var theme = when (headingMode) {
			1 -> MorphMarkdownThemes.LargeHeadings
			2 -> MorphMarkdownThemes.CompactHeadings
			3 -> hetiTheme()
			4 -> MorphMarkdownThemes.HetiLikeHei
			else -> MorphMarkdownThemes.Normal
		}
		val tabs = tabSize()
		theme = theme.copy(tabSize = tabs, codeBlockTabSize = tabs)
		if (!tableWrap) theme = theme.copy(tableCellWrap = false)
		if (compactCode) theme = theme.copy(codeBlockTextSizeSp = 13f, inlineCodeTextSizeSp = 14f)
		return theme
	}

	private fun hetiTheme(): MorphMarkdownTheme {
		if (!assetFont) return MorphMarkdownThemes.HetiLike
		return MorphMarkdownThemes.hetiLikeWithFont(
			fontAssetPath = "fonts/NotoSerifCJKsc-Regular.otf",
			boldFontAssetPath = "fonts/NotoSerifCJKsc-Bold.otf"
		)
	}

	private fun control(label: String, action: () -> Unit): TextView {
		return TextView(this).apply {
			text = label
			textSize = 13f
			setTextColor(0xff202020.toInt())
			setPadding(dp(10), dp(7), dp(10), dp(7))
			background = border()
			setOnClickListener { action() }
			layoutParams = LinearLayout.LayoutParams(-2, -2).apply {
				setMargins(0, 0, dp(8), 0)
			}
		}
	}

	private fun createChunks(validImage: File): List<StreamChunk> {
		val validUri = "file://${validImage.absolutePath}"
		val invalidUri = "file://${File(cacheDir, "missing-demo-image.png").absolutePath}"
		return StreamingSimulator.create(MarkdownFixtures.create(validUri, invalidUri).joinToString(""))
	}

	private fun createDemoImage(): File {
		val out = File(cacheDir, "markdown-render-generated.png")
		val bitmap = Bitmap.createBitmap(640, 360, Bitmap.Config.ARGB_8888)
		val canvas = Canvas(bitmap)
		val paint = Paint(Paint.ANTI_ALIAS_FLAG)
		drawDemoImage(canvas, paint)
		out.outputStream().use { stream ->
			bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream)
		}
		bitmap.recycle()
		return out
	}

	private fun drawDemoImage(canvas: Canvas, paint: Paint) {
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
	}

	private fun headingLabel(): String {
		return when (headingMode) {
			1 -> "large"
			2 -> "compact"
			3 -> "heti"
			4 -> "heti hei"
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

	private fun border(): GradientDrawable {
		return GradientDrawable().apply {
			setColor(0x00ffffff)
			setStroke(dp(1), 0xff454545.toInt())
		}
	}

	private fun dp(value: Int): Int {
		return (value * resources.displayMetrics.density + 0.5f).toInt()
	}
}
