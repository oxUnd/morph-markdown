package com.morph.markdown

import android.content.Context
import android.graphics.Bitmap
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.LinearLayout
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
		val fontSizePx = context.sp(theme.mathSize())
		val data = MarkdownNative.renderLatex(fontFile.absolutePath, latex, display, fontSizePx)
		if (data == null || data.size < 3) return null
		val bitmap = bitmapFromData(data)
		val image = fixedBitmapView(context, bitmap)
		if (!display) return image
		return displayMathContainer(context, image)
	}

	private fun fixedBitmapView(context: Context, bitmap: Bitmap): ImageView {
		return ImageView(context).apply {
			setImageBitmap(bitmap)
			adjustViewBounds = false
			scaleType = ImageView.ScaleType.CENTER
			layoutParams = ViewGroup.LayoutParams(bitmap.width, bitmap.height)
		}
	}

	private fun displayMathContainer(context: Context, image: View): View {
		return LinearLayout(context).apply {
			orientation = LinearLayout.VERTICAL
			setPadding(0, context.dp(12), 0, context.dp(12))
			addView(image)
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
