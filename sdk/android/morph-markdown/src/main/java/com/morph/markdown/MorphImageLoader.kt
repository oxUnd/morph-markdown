package com.morph.markdown

import android.content.Context
import android.graphics.BitmapFactory
import android.view.View
import android.widget.ImageView

interface MorphImageLoader {
	fun load(context: Context, url: String, theme: MorphMarkdownTheme): View?
}

class FileImageLoader : MorphImageLoader {
	override fun load(context: Context, url: String, theme: MorphMarkdownTheme): View? {
		val path = if (url.startsWith("file://")) url.removePrefix("file://") else url
		val bitmap = BitmapFactory.decodeFile(path) ?: return null
		return ImageView(context).apply {
			setImageBitmap(bitmap)
			adjustViewBounds = true
			maxWidth = context.dp(theme.imageMaxWidthDp)
			maxHeight = context.dp(theme.imageMaxHeightDp)
			setPadding(0, context.dp(6), 0, context.dp(6))
		}
	}
}
