package com.morph.markdown

internal sealed class RenderDebounceDecision {
	data class Schedule(val delayMs: Long) : RenderDebounceDecision()
	data class RenderNow(val autoScroll: Boolean) : RenderDebounceDecision()
	object None : RenderDebounceDecision()
}

internal class RenderDebounceState {
	private var scheduled = false
	private var pendingAutoScroll = false

	fun onAppend(final: Boolean, autoScroll: Boolean, debounceMs: Long): RenderDebounceDecision {
		pendingAutoScroll = pendingAutoScroll || autoScroll
		if (final || debounceMs <= 0L) {
			return RenderDebounceDecision.RenderNow(consumeAutoScroll())
		}
		if (scheduled) return RenderDebounceDecision.None
		scheduled = true
		return RenderDebounceDecision.Schedule(debounceMs)
	}

	fun onScheduledRender(): Boolean {
		scheduled = false
		return consumeAutoScroll()
	}

	fun cancel() {
		scheduled = false
		pendingAutoScroll = false
	}

	private fun consumeAutoScroll(): Boolean {
		val autoScroll = pendingAutoScroll
		pendingAutoScroll = false
		scheduled = false
		return autoScroll
	}
}
