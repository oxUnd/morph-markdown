package com.morph.markdown

import android.content.Context
import android.util.AttributeSet
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.ScrollView

class MorphMarkdownView @JvmOverloads constructor(
	context: Context,
	attrs: AttributeSet? = null
) : ScrollView(context, attrs) {
	private val engine = MorphMarkdownEngine()
	private val body = LinearLayout(context).apply {
		orientation = LinearLayout.VERTICAL
	}
	private val renderer = MorphMarkdownRenderer(context)

	var options = MorphMarkdownOptions()

	var theme: MorphMarkdownTheme
		get() = renderer.theme
		set(value) {
			renderer.theme = value
			renderSnapshot(false)
		}

	var mathRenderer: MorphMathRenderer?
		get() = renderer.mathRenderer
		set(value) {
			renderer.mathRenderer = value
			renderSnapshot(false)
		}

	var imageLoader: MorphImageLoader
		get() = renderer.imageLoader
		set(value) {
			renderer.imageLoader = value
			renderSnapshot(false)
		}

	init {
		clipToPadding = true
		addView(
			body,
			ViewGroup.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT,
				ViewGroup.LayoutParams.WRAP_CONTENT
			)
		)
	}

	fun append(markdown: String, final: Boolean = false) {
		engine.append(markdown, final)
		renderSnapshot(options.autoScrollOnAppend)
	}

	fun renderSnapshot(autoScroll: Boolean = false) {
		val json = engine.snapshotJson()
		if (json == null) {
			renderError()
			return
		}
		renderer.render(json, body)
		if (autoScroll) post { fullScroll(View.FOCUS_DOWN) }
	}

	fun close() {
		engine.close()
	}

	private fun renderError() {
		body.removeAllViews()
		body.addView(android.widget.TextView(context).apply {
			text = "snapshot failed"
			textSize = theme.bodyTextSizeSp
			setTextColor(0xff1b1b1b.toInt())
		})
	}
}
