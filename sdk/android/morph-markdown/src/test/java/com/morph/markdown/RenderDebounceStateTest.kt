package com.morph.markdown

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class RenderDebounceStateTest {
	@Test
	fun firstAppendSchedulesAndSubsequentAppendsReuseSchedule() {
		val state = RenderDebounceState()

		assertEquals(
			RenderDebounceDecision.Schedule(80L),
			state.onAppend(final = false, autoScroll = true, debounceMs = 80L)
		)
		assertEquals(
			RenderDebounceDecision.None,
			state.onAppend(final = false, autoScroll = false, debounceMs = 80L)
		)
		assertTrue(state.onScheduledRender())
	}

	@Test
	fun finalAppendRendersImmediatelyWithPendingAutoScroll() {
		val state = RenderDebounceState()

		assertEquals(
			RenderDebounceDecision.Schedule(80L),
			state.onAppend(final = false, autoScroll = true, debounceMs = 80L)
		)
		assertEquals(
			RenderDebounceDecision.RenderNow(true),
			state.onAppend(final = true, autoScroll = false, debounceMs = 80L)
		)
		assertFalse(state.onScheduledRender())
	}

	@Test
	fun zeroDebounceRendersImmediately() {
		val state = RenderDebounceState()

		assertEquals(
			RenderDebounceDecision.RenderNow(false),
			state.onAppend(final = false, autoScroll = false, debounceMs = 0L)
		)
	}

	@Test
	fun cancelDropsPendingAutoScroll() {
		val state = RenderDebounceState()

		assertEquals(
			RenderDebounceDecision.Schedule(80L),
			state.onAppend(final = false, autoScroll = true, debounceMs = 80L)
		)
		state.cancel()
		assertFalse(state.onScheduledRender())
	}
}
