package com.morph.markdown

import android.content.Context
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.text.SpannableString
import android.text.Spanned
import android.text.style.ScaleXSpan
import android.util.TypedValue
import android.widget.TextView
import java.io.File

internal fun Context.dp(value: Int): Int {
	return (value * resources.displayMetrics.density + 0.5f).toInt()
}

internal fun Context.dpFloat(value: Float): Float {
	return value * resources.displayMetrics.density
}

internal fun Context.sp(value: Float): Float {
	return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, value, resources.displayMetrics)
}

internal fun Context.textLineHeightPx(sizeSp: Float, multiplier: Float): Int {
	return sp(sizeSp * multiplier).toInt().coerceAtLeast(1)
}

internal fun TextView.applyMorphTextMetrics(sizeSp: Float, lineHeightMultiplier: Float) {
	includeFontPadding = false
	val metrics = paint.fontMetricsInt
	val glyphHeight = (metrics.descent - metrics.ascent).coerceAtLeast(1)
	val target = context.textLineHeightPx(sizeSp, lineHeightMultiplier)
	val extra = (target - glyphHeight).coerceAtLeast(0)
	setLineSpacing(extra.toFloat(), 1f)
	setTextVerticalPadding(extra)
}

private fun TextView.setTextVerticalPadding(extra: Int) {
	val top = extra / 2
	val bottom = extra - top
	setPadding(paddingLeft, top, paddingRight, bottom)
}

internal fun fill(color: Int): GradientDrawable {
	return GradientDrawable().apply {
		setColor(color)
		cornerRadius = 4f
	}
}

internal fun Context.border(header: Boolean): GradientDrawable {
	return GradientDrawable().apply {
		setColor(if (header) 0xffefefea.toInt() else 0x00ffffff)
		setStroke(dp(1), 0xff454545.toInt())
	}
}

internal fun expandTabs(value: String, tabSize: Int): String {
	if (!value.contains('\t')) return value
	val out = StringBuilder(value.length)
	var col = 0
	for (ch in value) {
		when (ch) {
			'\t' -> col = appendTab(out, tabSize, col)
			'\n' -> {
				out.append(ch)
				col = 0
			}
			else -> {
				out.append(ch)
				col += 1
			}
		}
	}
	return out.toString()
}

private fun appendTab(out: StringBuilder, tabSize: Int, col: Int): Int {
	var next = col
	val spaces = tabSize - (col % tabSize)
	repeat(spaces) {
		out.append(' ')
		next += 1
	}
	return next
}

internal fun typefaceFor(context: Context, theme: MorphMarkdownTheme, bold: Boolean = false): Typeface {
	val assetPath = if (bold) theme.boldFontAssetPath ?: theme.fontAssetPath else theme.fontAssetPath
	if (assetPath != null) return TypefaceCache.asset(context, assetPath)
	val style = if (bold) Typeface.BOLD else Typeface.NORMAL
	return when (theme.fontProfile) {
		MorphFontProfile.HetiLikeHei -> TypefaceCache.systemFile("/system/fonts/NotoSansCJK-Regular.ttc", style)
		MorphFontProfile.HetiLikeSong -> Typeface.create("serif", style)
		MorphFontProfile.System -> Typeface.create(Typeface.DEFAULT, style)
	}
}

private object TypefaceCache {
	private val cache = mutableMapOf<String, Typeface>()

	fun asset(context: Context, path: String): Typeface {
		return cache.getOrPut("asset:$path") {
			Typeface.createFromAsset(context.assets, path)
		}
	}

	fun systemFile(path: String, style: Int): Typeface {
		return cache.getOrPut("file:$path:$style") {
			val file = File(path)
			if (file.exists()) Typeface.create(Typeface.createFromFile(file), style)
			else Typeface.create("sans-serif", style)
		}
	}
}

internal fun processedText(value: String, theme: MorphMarkdownTheme, allowCjkSpacing: Boolean): CharSequence {
	if (!allowCjkSpacing || theme.textProcessor != MorphTextProcessor.CjkSpacing) return value
	return CjkTextProcessor.spacing(value)
}

private object CjkTextProcessor {
	private const val SPACING = "\u2006"

	fun spacing(value: String): CharSequence {
		if (value.length < 2) return value
		val out = StringBuilder(value.length + 8)
		for (i in value.indices) {
			val ch = value[i]
			out.append(ch)
			if (needsSpacing(ch, value.getOrNull(i + 1))) out.append(SPACING)
		}
		return compressSpacing(out.toString())
	}

	private fun needsSpacing(left: Char, right: Char?): Boolean {
		if (right == null || left.isWhitespace() || right.isWhitespace()) return false
		return isCjk(left) && isAsciiWord(right) || isAsciiWord(left) && isCjk(right)
	}

	private fun compressSpacing(value: String): CharSequence {
		val spannable = SpannableString(value)
		var index = value.indexOf(SPACING)
		while (index >= 0) {
			spannable.setSpan(ScaleXSpan(0.5f), index, index + SPACING.length, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
			index = value.indexOf(SPACING, index + SPACING.length)
		}
		return spannable
	}

	private fun isAsciiWord(ch: Char): Boolean {
		return ch.code in 0x21..0x7e && ch !in asciiPunctuation
	}

	private fun isCjk(ch: Char): Boolean {
		val block = Character.UnicodeBlock.of(ch)
		return block == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS ||
			block == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A ||
			block == Character.UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS ||
			block == Character.UnicodeBlock.HIRAGANA ||
			block == Character.UnicodeBlock.KATAKANA ||
			block == Character.UnicodeBlock.HANGUL_SYLLABLES
	}

	private val asciiPunctuation = setOf(
		'.', ',', ':', ';', '!', '?', '\'', '"', '`', '/', '\\', '|',
		'(', ')', '[', ']', '{', '}', '<', '>', '+', '-', '*', '=', '_'
	)
}
