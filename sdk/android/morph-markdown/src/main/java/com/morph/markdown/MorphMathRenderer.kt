package com.morph.markdown

import android.content.Context
import android.graphics.Bitmap
import android.widget.ImageView
import android.view.View
import java.io.File

interface MorphMathRenderer {
	fun render(context: Context, latex: String, display: Boolean, theme: MorphMarkdownTheme): View?
}

class MathJaxMathRenderer(context: Context) : MorphMathRenderer {
	private val fontFile: File = copyFont(context)

	override fun render(
		context: Context,
		latex: String,
		display: Boolean,
		theme: MorphMarkdownTheme
	): View? {
		val size = theme.mathSize() * context.resources.displayMetrics.scaledDensity
		val data = MarkdownNative.renderLatex(fontFile.absolutePath, latex, display, size)
		if (data == null || data.size < 3) return null
		val bitmap = bitmapFromData(data)
		return ImageView(context).apply {
			setImageBitmap(bitmap)
			adjustViewBounds = true
			setPadding(0, context.dp(if (display) 12 else 0), 0, context.dp(if (display) 12 else 0))
		}
	}

	private fun bitmapFromData(data: IntArray): Bitmap {
		val width = data[0]
		val height = data[1]
		val pixels = data.copyOfRange(2, data.size)
		return Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888)
	}

	private fun copyFont(context: Context): File {
		val out = File(context.filesDir, "STIXTwoMath-Regular.ttf")
		if (!out.exists()) {
			context.assets.open("STIXTwoMath-Regular.ttf").use { input ->
				out.outputStream().use { output -> input.copyTo(output) }
			}
		}
		return out
	}
}
