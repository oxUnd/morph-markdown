package com.morph.markdown

import android.content.Context
import android.util.AttributeSet
import android.view.GestureDetector
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.ScrollView

class MorphMarkdownView @JvmOverloads constructor(
	context: Context,
	attrs: AttributeSet? = null
) : ScrollView(context, attrs) {
	private var engine = MorphMarkdownEngine()
	private val body = LinearLayout(context).apply {
		orientation = LinearLayout.VERTICAL
	}
	private val renderer = MorphMarkdownRenderer(context)
	private val renderDebounce = RenderDebounceState()
	private var contentLongPressHandled = false
	private val contentGestureDetector = GestureDetector(context, object : GestureDetector.SimpleOnGestureListener() {
		override fun onDown(e: MotionEvent): Boolean = true

		override fun onLongPress(e: MotionEvent) {
			val handled = onContentLongClick?.invoke(e.x, e.y) == true
			if (handled) {
				contentLongPressHandled = true
				performHapticFeedback(HapticFeedbackConstants.LONG_PRESS)
				parent?.requestDisallowInterceptTouchEvent(true)
			}
		}
	})
	private val scheduledRender = Runnable {
		performRenderSnapshot(renderDebounce.onScheduledRender(), reuseStablePrefix = true)
	}

	var options = MorphMarkdownOptions()
	var onRendered: (() -> Unit)? = null
	var onContentLongClick: ((x: Float, y: Float) -> Boolean)? = null

	var theme: MorphMarkdownTheme
		get() = renderer.theme
		set(value) {
			renderer.theme = value
			renderSnapshot(autoScroll = false, reuseStablePrefix = false)
		}

	var mathRenderer: MorphMathRenderer?
		get() = renderer.mathRenderer
		set(value) {
			renderer.mathRenderer = value
			renderSnapshot(autoScroll = false, reuseStablePrefix = false)
		}

	var imageLoader: MorphImageLoader
		get() = renderer.imageLoader
		set(value) {
			renderer.imageLoader = value
			renderSnapshot(autoScroll = false, reuseStablePrefix = false)
		}

	var onLinkClick: MorphMarkdownLinkHandler?
		get() = renderer.onLinkClick
		set(value) {
			renderer.onLinkClick = value
			renderSnapshot(autoScroll = false, reuseStablePrefix = false)
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
		when (val decision = renderDebounce.onAppend(final, options.autoScrollOnAppend, options.appendRenderDebounceMs)) {
			RenderDebounceDecision.None -> Unit
			is RenderDebounceDecision.Schedule -> postDelayed(scheduledRender, decision.delayMs)
			is RenderDebounceDecision.RenderNow -> renderSnapshot(decision.autoScroll, reuseStablePrefix = true)
		}
	}

	override fun dispatchTouchEvent(ev: MotionEvent): Boolean {
		if (ev.actionMasked == MotionEvent.ACTION_DOWN) {
			contentLongPressHandled = false
		}
		contentGestureDetector.onTouchEvent(ev)
		if (contentLongPressHandled) {
			if (ev.actionMasked == MotionEvent.ACTION_UP ||
				ev.actionMasked == MotionEvent.ACTION_CANCEL) {
				contentLongPressHandled = false
				parent?.requestDisallowInterceptTouchEvent(false)
			}
			return true
		}
		return super.dispatchTouchEvent(ev)
	}

	fun renderSnapshot(autoScroll: Boolean = false, reuseStablePrefix: Boolean = false) {
		removeCallbacks(scheduledRender)
		renderDebounce.cancel()
		performRenderSnapshot(autoScroll, reuseStablePrefix)
	}

	private fun performRenderSnapshot(autoScroll: Boolean = false, reuseStablePrefix: Boolean) {
		val json = engine.snapshotJson()
		if (json == null) {
			renderError()
			return
		}
		if (reuseStablePrefix) {
			renderer.renderReusingStablePrefix(json, body, engine.stableBlockCount())
		} else {
			renderer.render(json, body)
		}
		onRendered?.invoke()
		if (autoScroll) post { fullScroll(View.FOCUS_DOWN) }
	}

	fun reset() {
		removeCallbacks(scheduledRender)
		renderDebounce.cancel()
		engine.close()
		engine = MorphMarkdownEngine()
		renderer.render("""{"children":[]}""", body)
		onRendered?.invoke()
	}

	fun close() {
		removeCallbacks(scheduledRender)
		renderDebounce.cancel()
		engine.close()
	}

	private fun renderError() {
		body.removeAllViews()
		body.addView(android.widget.TextView(context).apply {
			text = "snapshot failed"
			textSize = theme.bodyTextSizeSp
			setTextColor(theme.bodyTextColor)
		})
	}
}
