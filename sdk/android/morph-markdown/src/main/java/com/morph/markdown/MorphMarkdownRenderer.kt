package com.morph.markdown

import android.content.Context
import android.text.SpannableStringBuilder
import android.text.Spanned
import android.text.TextPaint
import android.text.method.LinkMovementMethod
import android.text.style.BackgroundColorSpan
import android.text.style.ClickableSpan
import android.text.style.RelativeSizeSpan
import android.text.style.TypefaceSpan
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject

private enum class TableCellRole {
	None,
	Header,
	Body
}

private data class LinkTarget(
	val url: String,
	val title: String?
)

private data class RenderedBlock(
	val signature: String,
	val viewCount: Int
)

private data class InlineSpanState(
	var hasLink: Boolean = false
)

private class MorphInlineLinkSpan(
	private val target: LinkTarget,
	private val color: Int,
	private val underline: Boolean,
	private val handler: MorphMarkdownLinkHandler?
) : ClickableSpan() {
	override fun onClick(widget: View) {
		handler?.invoke(target.url, target.title)
	}

	override fun updateDrawState(ds: TextPaint) {
		ds.color = color
		ds.isUnderlineText = underline
	}
}

class MorphMarkdownRenderer(
	private val context: Context,
	var theme: MorphMarkdownTheme = MorphMarkdownThemes.Normal,
	var mathRenderer: MorphMathRenderer? = null,
	var imageLoader: MorphImageLoader = FileImageLoader(),
	var onLinkClick: MorphMarkdownLinkHandler? = null
) {
	private var renderedBlocks: List<RenderedBlock> = emptyList()

	fun render(json: String, parent: LinearLayout) {
		parent.removeAllViews()
		val root = JSONObject(json)
		renderedBlocks = renderChildrenFrom(root.optJSONArray("children"), parent, 0)
	}

	fun renderReusingStablePrefix(json: String, parent: LinearLayout, stableBlockCount: Int) {
		val root = JSONObject(json)
		val children = root.optJSONArray("children")
		val nextSignatures = blockSignatures(children)
		val blockPrefix = stableBlockPrefixLength(renderedBlocks, nextSignatures, stableBlockCount)
		val childPrefix = stableChildPrefixLength(renderedBlocks, blockPrefix)
		removeTail(parent, childPrefix)
		val renderedTail = renderChildrenFrom(children, parent, blockPrefix)
		renderedBlocks = renderedBlocks.take(blockPrefix) + renderedTail
	}

	private fun renderChildren(children: JSONArray?, parent: LinearLayout) {
		if (children == null) return
		for (i in 0 until children.length()) {
			renderBlock(children.getJSONObject(i), parent)
		}
	}

	private fun renderChildrenFrom(
		children: JSONArray?,
		parent: LinearLayout,
		start: Int
	): List<RenderedBlock> {
		if (children == null) return emptyList()
		val out = mutableListOf<RenderedBlock>()
		for (i in start until children.length()) {
			val before = parent.childCount
			val block = children.getJSONObject(i)
			renderBlock(block, parent)
			out.add(RenderedBlock(canonicalJson(block), parent.childCount - before))
		}
		return out
	}

	private fun blockSignatures(children: JSONArray?): List<String> {
		if (children == null) return emptyList()
		return (0 until children.length()).map { i ->
			canonicalJson(children.getJSONObject(i))
		}
	}

	private fun stableBlockPrefixLength(
		previous: List<RenderedBlock>,
		next: List<String>,
		stableBlockCount: Int
	): Int {
		val max = minOf(previous.size, next.size, stableBlockCount.coerceAtLeast(0))
		for (i in 0 until max) {
			if (previous[i].signature != next[i]) return i
		}
		return max
	}

	private fun stableChildPrefixLength(blocks: List<RenderedBlock>, blockPrefix: Int): Int {
		return blocks.take(blockPrefix).sumOf { it.viewCount }
	}

	private fun removeTail(parent: LinearLayout, prefix: Int) {
		val removeCount = parent.childCount - prefix
		if (removeCount > 0) parent.removeViews(prefix, removeCount)
	}

	private fun canonicalJson(value: Any?): String {
		return when (value) {
			is JSONObject -> canonicalObject(value)
			is JSONArray -> canonicalArray(value)
			JSONObject.NULL, null -> "null"
			else -> JSONObject.quote(value.toString())
		}
	}

	private fun canonicalObject(obj: JSONObject): String {
		val keys = obj.keys().asSequence().filter { it != "id" }.sorted().toList()
		return keys.joinToString(prefix = "{", postfix = "}") { key ->
			JSONObject.quote(key) + ":" + canonicalJson(obj.opt(key))
		}
	}

	private fun canonicalArray(array: JSONArray): String {
		return (0 until array.length()).joinToString(prefix = "[", postfix = "]") { i ->
			canonicalJson(array.opt(i))
		}
	}

	private fun renderBlock(node: JSONObject, parent: LinearLayout) {
		when (node.optString("kind")) {
			"heading" -> parent.addView(heading(node))
			"paragraph" -> parent.addView(inlineGroup(node.optJSONArray("children")))
			"list" -> renderList(node, parent, 0)
			"table" -> parent.addView(table(node))
			"block_quote" -> parent.addView(blockQuote(node))
			"math_block" -> parent.addView(mathView(node.optString("literal"), true))
			"image" -> parent.addView(imageView(node))
			"code_block" -> parent.addView(codeBlock(node.optString("literal")))
			"thematic_break" -> parent.addView(rule())
			"soft_break", "hard_break" -> parent.addView(spacer(4))
			else -> renderUnknown(node, parent)
		}
	}

	private fun renderUnknown(node: JSONObject, parent: LinearLayout) {
		val children = node.optJSONArray("children")
		if (children != null) {
			renderChildren(children, parent)
		} else {
			parent.addView(cellText(node.optString("literal"), TableCellRole.None))
		}
	}

	private fun renderList(node: JSONObject, parent: LinearLayout, depth: Int) {
		val children = node.optJSONArray("children") ?: return
		val ordered = node.optString("list_type") == "ordered"
		var number = node.optInt("start", 1)
		for (i in 0 until children.length()) {
			val item = children.getJSONObject(i)
			if (item.optString("kind") == "tasklist") {
				parent.addView(taskItem(item))
			} else {
				parent.addView(listItem(item, ordered, number, depth))
				if (ordered) number += 1
			}
		}
	}

	private fun listItem(item: JSONObject, ordered: Boolean, number: Int, depth: Int): LinearLayout {
		val row = listRow()
		row.addView(listMarker(ordered, number, depth))
		row.addView(listItemContent(item, depth))
		return row
	}

	private fun listItemContent(item: JSONObject, depth: Int): LinearLayout {
		val group = verticalGroup(0, 0, 0, context.dp(theme.listItemSpacingDp)).apply {
			layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
		}
		group.addView(inlineGroup(inlineChildrenOf(item), compact = true))
		addNestedLists(item, group, depth)
		return group
	}

	private fun addNestedLists(item: JSONObject, group: LinearLayout, depth: Int) {
		val children = item.optJSONArray("children") ?: return
		for (i in 0 until children.length()) {
			val child = children.getJSONObject(i)
			if (child.optString("kind") == "list") {
				val nested = verticalGroup(context.dp(theme.nestedListIndentDp), 0, 0, 0)
				renderList(child, nested, depth + 1)
				group.addView(nested)
			}
		}
	}

	private fun listRow(): LinearLayout {
		return LinearLayout(context).apply {
			orientation = LinearLayout.HORIZONTAL
			gravity = Gravity.TOP
			setPadding(0, 0, 0, context.dp(theme.listItemSpacingDp))
		}
	}

	private fun listMarker(ordered: Boolean, number: Int, depth: Int): View {
		if (ordered) return orderedMarker(number)
		return ListMarkerView(context, unorderedMarker(depth), theme)
	}

	private fun orderedMarker(number: Int): TextView {
		return text("$number.", theme.bodyTextSizeSp).apply {
			gravity = Gravity.CENTER
			layoutParams = LinearLayout.LayoutParams(context.dp(theme.orderedMarkerWidthDp), -2)
		}
	}

	private fun unorderedMarker(depth: Int): MorphListMarkerStyle {
		if (theme.unorderedListMarkers.isEmpty()) return MorphListMarkerStyle.Disc
		return theme.unorderedListMarkers[depth % theme.unorderedListMarkers.size]
	}

	private fun taskItem(item: JSONObject): LinearLayout {
		val row = LinearLayout(context).apply {
			orientation = LinearLayout.HORIZONTAL
			gravity = Gravity.TOP
			setPadding(0, 0, 0, context.dp(theme.listItemSpacingDp))
		}
		row.addView(TaskMarkerView(context, item.optBoolean("checked", false), theme).apply {
			layoutParams = LinearLayout.LayoutParams(
				context.dp(theme.taskBoxSizeDp),
				context.dp(theme.taskBoxSizeDp)
			).apply {
				gravity = Gravity.TOP
				topMargin = taskBoxTopMargin()
				setMargins(0, 0, context.dp(theme.taskBoxTextGapDp), 0)
			}
		})
		row.addView(inlineGroup(inlineChildrenOf(item), compact = true).apply {
			layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
		})
		return row
	}

	private fun taskBoxTopMargin(): Int {
		val lineHeight = context.textLineHeightPx(theme.bodyTextSizeSp, theme.bodyLineHeightMultiplier)
		return ((lineHeight - context.dp(theme.taskBoxSizeDp)) / 2).coerceAtLeast(0)
	}

	private fun heading(node: JSONObject): TextView {
		val level = node.optInt("level", 1).coerceIn(1, 6)
		return text(plainText(node), theme.headingSize(level), allowCjkSpacing = true).apply {
			typeface = typefaceFor(context, theme, bold = true)
			setLineSpacing(0f, lineHeightMultiplier(theme.headingSize(level), theme.headingLineHeight(level)))
			setPadding(0, context.dp(theme.headingTopSpacingDp), 0, context.dp(theme.headingBottomSpacingDp))
		}
	}

	private fun inlineGroup(children: JSONArray?, compact: Boolean = false): View {
		spannedInlineText(children, TableCellRole.None, compact)?.let { return it }
		return inlineLayoutGroup(children, compact)
	}

	private fun inlineLayoutGroup(children: JSONArray?, compact: Boolean = false): InlineLayout {
		val row = InlineLayout(context)
		if (compact) {
			row.setPadding(0, 0, 0, 0)
		} else {
			row.setPadding(0, context.dp(theme.paragraphTopSpacingDp), 0, context.dp(theme.paragraphBottomSpacingDp))
		}
		populateInline(row, children, TableCellRole.None)
		return row
	}

	private fun spannedInlineText(
		children: JSONArray?,
		role: TableCellRole,
		compact: Boolean
	): TextView? {
		val out = SpannableStringBuilder()
		val state = InlineSpanState()
		if (!appendSpannableChildren(out, children, role, state, null)) return null
		return spannedTextView(out, role, compact, state.hasLink)
	}

	private fun appendSpannableChildren(
		out: SpannableStringBuilder,
		children: JSONArray?,
		role: TableCellRole,
		state: InlineSpanState,
		link: LinkTarget?
	): Boolean {
		if (children == null) return true
		for (i in 0 until children.length()) {
			if (!appendSpannableInline(out, children.getJSONObject(i), role, state, link)) return false
		}
		return true
	}

	private fun appendSpannableInline(
		out: SpannableStringBuilder,
		child: JSONObject,
		role: TableCellRole,
		state: InlineSpanState,
		link: LinkTarget?
	): Boolean {
		return when (child.optString("kind")) {
			"text" -> appendSpannableText(out, child.optString("literal"), role, state, link, code = false)
			"code" -> appendSpannableText(out, child.optString("literal"), role, state, link, code = true)
			"soft_break", "hard_break" -> appendSpannableText(out, "\n", role, state, link, code = false)
			"link" -> appendSpannableLink(out, child, role, state)
			else -> false
		}
	}

	private fun appendSpannableLink(
		out: SpannableStringBuilder,
		child: JSONObject,
		role: TableCellRole,
		state: InlineSpanState
	): Boolean {
		val url = child.optString("url", "")
		if (url.isEmpty()) return appendSpannableText(out, plainText(child), role, state, null, code = false)
		val target = LinkTarget(url, child.optString("title").ifEmpty { null })
		val children = child.optJSONArray("children")
		if (children == null || children.length() == 0) {
			return appendSpannableText(out, url, role, state, target, code = false)
		}
		return appendSpannableChildren(out, children, role, state, target)
	}

	private fun appendSpannableText(
		out: SpannableStringBuilder,
		value: String,
		role: TableCellRole,
		state: InlineSpanState,
		link: LinkTarget?,
		code: Boolean
	): Boolean {
		val start = out.length
		val expanded = expandTabs(value, theme.tabSize)
		out.append(processedText(expanded, theme, allowCjkSpacing = !code))
		val end = out.length
		if (end <= start) return true
		if (code) applyInlineCodeSpans(out, start, end, role)
		if (link != null) {
			state.hasLink = true
			out.setSpan(
				MorphInlineLinkSpan(link, theme.linkTextColor, theme.linkUnderline, onLinkClick),
				start,
				end,
				Spanned.SPAN_EXCLUSIVE_EXCLUSIVE
			)
		}
		return true
	}

	private fun applyInlineCodeSpans(out: SpannableStringBuilder, start: Int, end: Int, role: TableCellRole) {
		out.setSpan(TypefaceSpan("monospace"), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
		out.setSpan(BackgroundColorSpan(0xffeeeeea.toInt()), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
		val ratio = theme.inlineCodeTextSizeSp / tableTextSize(role)
		if (kotlin.math.abs(ratio - 1f) > 0.01f) {
			out.setSpan(RelativeSizeSpan(ratio), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
		}
	}

	private fun spannedTextView(
		value: CharSequence,
		role: TableCellRole,
		compact: Boolean,
		hasLinks: Boolean
	): TextView {
		return TextView(context).apply {
			text = value
			textSize = tableTextSize(role)
			setTextColor(if (role == TableCellRole.None) 0xff1b1b1b.toInt() else tableTextColor(role))
			typeface = typefaceFor(context, theme, bold = role == TableCellRole.Header && theme.tableStyle.headerBold)
			applySpannedTextMetrics(role)
			applySpannedPadding(role, compact)
			configureSpannedTableWrap(role)
			if (hasLinks) configureLinkMovement()
		}
	}

	private fun TextView.applySpannedTextMetrics(role: TableCellRole) {
		if (role == TableCellRole.None) {
			applyMorphTextMetrics(theme.bodyTextSizeSp, theme.bodyLineHeightMultiplier)
		} else {
			applyMorphTextMetrics(tableTextSize(role), theme.tableCellLineHeightMultiplier)
		}
	}

	private fun TextView.applySpannedPadding(role: TableCellRole, compact: Boolean) {
		val fontTop = paddingTop
		val fontBottom = paddingBottom
		if (role == TableCellRole.None) {
			if (!compact) {
				setPadding(
					0,
					context.dp(theme.paragraphTopSpacingDp) + fontTop,
					0,
					context.dp(theme.paragraphBottomSpacingDp) + fontBottom
				)
			}
			return
		}
		setPadding(
			context.dp(theme.tableCellPaddingHorizontalDp),
			context.dp(theme.tableCellPaddingVerticalDp) + fontTop,
			context.dp(theme.tableCellPaddingHorizontalDp),
			context.dp(theme.tableCellPaddingVerticalDp) + fontBottom
		)
	}

	private fun TextView.configureSpannedTableWrap(role: TableCellRole) {
		if (role == TableCellRole.None) return
		if (theme.tableCellWrap) {
			setSingleLine(false)
			setHorizontallyScrolling(false)
		} else {
			setSingleLine(true)
			setHorizontallyScrolling(true)
		}
	}

	private fun TextView.configureLinkMovement() {
		linksClickable = true
		movementMethod = LinkMovementMethod.getInstance()
		highlightColor = 0x1a2f6f73
	}

	private fun populateInline(row: ViewGroup, children: JSONArray?, role: TableCellRole) {
		populateInline(row, children, role, null)
	}

	private fun populateInline(row: ViewGroup, children: JSONArray?, role: TableCellRole, link: LinkTarget?) {
		if (children == null) return
		for (i in 0 until children.length()) {
			addInline(row, children.getJSONObject(i), role, link)
		}
	}

	private fun addInline(row: ViewGroup, child: JSONObject, role: TableCellRole, link: LinkTarget? = null) {
		when (child.optString("kind")) {
			"text" -> addInlineText(row, child.optString("literal"), role, link)
			"code" -> row.addView(linkedView(inlineCode(child.optString("literal"), role), link))
			"soft_break", "hard_break" -> addInlineText(row, "\n", role, link)
			"math_inline" -> row.addView(linkedView(mathView(child.optString("literal"), false, role), link))
			"math_block" -> row.addView(linkedView(mathView(child.optString("literal"), true, role), link))
			"image" -> row.addView(linkedView(imageView(child), link))
			"link" -> addLinkInline(row, child, role)
			else -> addInlineText(row, plainText(child), role, link)
		}
	}

	private fun addLinkInline(row: ViewGroup, child: JSONObject, role: TableCellRole) {
		val url = child.optString("url", "")
		if (url.isEmpty()) {
			addInlineText(row, plainText(child), role, null)
			return
		}
		val title = child.optString("title").ifEmpty { null }
		val children = child.optJSONArray("children")
		if (children == null || children.length() == 0) {
			addInlineText(row, url, role, LinkTarget(url, title))
			return
		}
		populateInline(row, children, role, LinkTarget(url, title))
	}

	private fun addInlineText(row: ViewGroup, value: String, role: TableCellRole, link: LinkTarget? = null) {
		val expanded = expandTabs(value, theme.tabSize)
		val spaced = processedText(expanded, theme, allowCjkSpacing = true).toString()
		for (fragment in InlineTextFragmenter.fragments(spaced)) {
			row.addView(inlineTextFragment(fragment, role, link))
		}
	}

	private fun inlineTextFragment(value: String, role: TableCellRole, link: LinkTarget?): TextView {
		if (link == null) return cellText(value, role, expand = false, allowCjkSpacing = false)
		return linkText(value, role).also {
			linkedView(it, link)
		}
	}

	private fun linkedView(view: View, link: LinkTarget?): View {
		if (link == null) return view
		view.isClickable = true
		view.setOnClickListener {
			onLinkClick?.invoke(link.url, link.title)
		}
		return view
	}

	private fun linkText(value: String, role: TableCellRole): LinkTextView {
		return LinkTextView(context).apply {
			text = processedText(value, theme, allowCjkSpacing = false)
			textSize = tableTextSize(role)
			setTextColor(theme.linkTextColor)
			linkColor = theme.linkTextColor
			drawLinkUnderline = theme.linkUnderline
			typeface = linkTypeface(role)
			applyMorphTextMetrics(tableTextSize(role), linkLineHeightMultiplier(role))
			setSingleLine(true)
			if (role != TableCellRole.None && theme.tableCellWrap) {
				setHorizontallyScrolling(false)
			} else if (role != TableCellRole.None) {
				setHorizontallyScrolling(true)
			}
		}
	}

	private fun linkTypeface(role: TableCellRole): android.graphics.Typeface {
		return typefaceFor(
			context,
			theme,
			bold = role == TableCellRole.Header && theme.tableStyle.headerBold
		)
	}

	private fun linkLineHeightMultiplier(role: TableCellRole): Float {
		return if (role == TableCellRole.None) {
			theme.bodyLineHeightMultiplier
		} else {
			theme.tableCellLineHeightMultiplier
		}
	}

	private fun blockQuote(node: JSONObject): View {
		val box = LinearLayout(context).apply {
			orientation = LinearLayout.HORIZONTAL
			setPadding(
				0,
				context.dp(theme.blockquoteVerticalPaddingDp),
				0,
				context.dp(theme.blockquoteBottomSpacingDp)
			)
		}
		box.addView(View(context).apply {
			background = fill(0xff767676.toInt())
			layoutParams = LinearLayout.LayoutParams(context.dp(4), ViewGroup.LayoutParams.MATCH_PARENT)
		})
		box.addView(text(expandTabs(plainText(node).trim(), theme.tabSize), theme.bodyTextSizeSp, true).apply {
			setPadding(context.dp(theme.blockquoteIndentDp), 0, 0, 0)
		})
		return box
	}

	private fun table(node: JSONObject): View {
		val table = MarkdownTableView(context, theme)
		populateTable(table, node.optJSONArray("children"))
		table.layoutParams = ViewGroup.LayoutParams(
			ViewGroup.LayoutParams.WRAP_CONTENT,
			ViewGroup.LayoutParams.WRAP_CONTENT
		)
		if (!theme.tableHorizontalScroll) return table
		return MarkdownTableScrollView(context).apply {
			isHorizontalScrollBarEnabled = true
			isFillViewport = true
			overScrollMode = View.OVER_SCROLL_IF_CONTENT_SCROLLS
			setPadding(0, context.dp(theme.tableTopSpacingDp), 0, context.dp(theme.tableBottomSpacingDp))
			addView(table)
		}
	}

	private fun populateTable(table: MarkdownTableView, rows: JSONArray?) {
		if (rows == null) return
		for (i in 0 until rows.length()) {
			val rowNode = rows.getJSONObject(i)
			table.beginRow(i == 0)
			addTableCells(table, rowNode.optJSONArray("children") ?: JSONArray(), i == 0)
		}
	}

	private fun addTableCells(table: MarkdownTableView, cells: JSONArray, header: Boolean) {
		val role = if (header) TableCellRole.Header else TableCellRole.Body
		for (j in 0 until cells.length()) {
			val cellNode = cells.getJSONObject(j)
			val cell = spannedInlineText(inlineChildrenOf(cellNode), role, compact = true) ?: InlineLayout(context).apply {
				lineSpacingPx = context.dp(theme.tableCellLineSpacingDp)
				minLineHeightPx = tableLineHeightPx(role)
				setPadding(
					context.dp(theme.tableCellPaddingHorizontalDp),
					context.dp(theme.tableCellPaddingVerticalDp),
					context.dp(theme.tableCellPaddingHorizontalDp),
					context.dp(theme.tableCellPaddingVerticalDp)
				)
				populateInline(this, inlineChildrenOf(cellNode), role)
			}
			table.addCell(cell)
		}
	}

	private fun mathView(
		latex: String,
		display: Boolean,
		role: TableCellRole = TableCellRole.None
	): View {
		val renderTheme = if (role == TableCellRole.None) theme else theme.copy(
			mathTextScale = theme.tableMathTextScale
		)
		val rendered = mathRenderer?.render(context, latex, display, renderTheme)
		return rendered ?: text(latex, theme.bodyTextSizeSp)
	}

	private fun imageView(node: JSONObject): View {
		val url = node.optString("url", node.optString("literal"))
		val loaded = imageLoader.load(context, url, theme)
		if (loaded != null) return loaded
		return text("invalid image: $url", theme.inlineCodeTextSizeSp).apply {
			setPadding(
				context.dp(theme.codeBlockPaddingHorizontalDp),
				context.dp(theme.codeBlockPaddingVerticalDp),
				context.dp(theme.codeBlockPaddingHorizontalDp),
				context.dp(theme.codeBlockPaddingVerticalDp)
			)
			background = context.border(false)
		}
	}

	private fun inlineCode(code: String, role: TableCellRole = TableCellRole.None): TextView {
		return text(expandTabs(code, theme.tabSize), theme.inlineCodeTextSizeSp).apply {
			typeface = android.graphics.Typeface.MONOSPACE
			applyTableTextStyle(role)
			if (role != TableCellRole.None && theme.tableCellWrap) {
				maxWidth = context.dp((theme.tableCellMaxWidthDp * 0.78f).toInt())
			}
			setPadding(
				context.dp(theme.inlineCodePaddingHorizontalDp),
				context.dp(theme.inlineCodePaddingVerticalDp),
				context.dp(theme.inlineCodePaddingHorizontalDp),
				context.dp(theme.inlineCodePaddingVerticalDp)
			)
			background = fill(0xffeeeeea.toInt())
		}
	}

	private fun cellText(
		value: String,
		role: TableCellRole,
		expand: Boolean = true,
		allowCjkSpacing: Boolean = true
	): TextView {
		val source = if (expand) expandTabs(value, theme.tabSize) else value
		return text(source, tableTextSize(role), allowCjkSpacing = allowCjkSpacing).apply {
			applyTableTextStyle(role)
			setSingleLine(true)
			if (role != TableCellRole.None && theme.tableCellWrap) {
				setHorizontallyScrolling(false)
			} else if (role != TableCellRole.None) {
				setHorizontallyScrolling(true)
			}
		}
	}

	private fun TextView.applyTableTextStyle(role: TableCellRole) {
		if (role == TableCellRole.None) return
		setTextColor(tableTextColor(role))
		if (role == TableCellRole.Header && theme.tableStyle.headerBold) {
			typeface = typefaceFor(context, theme, bold = true)
		}
		applyMorphTextMetrics(tableTextSize(role), theme.tableCellLineHeightMultiplier)
	}

	private fun tableTextColor(role: TableCellRole): Int {
		return if (role == TableCellRole.Header) {
			theme.tableStyle.headerTextColor
		} else {
			theme.tableStyle.bodyTextColor
		}
	}

	private fun tableTextSize(role: TableCellRole): Float {
		return if (role == TableCellRole.Header) {
			theme.tableStyle.headerTextSizeSp ?: theme.bodyTextSizeSp
		} else if (role == TableCellRole.Body) {
			theme.tableStyle.bodyTextSizeSp ?: theme.bodyTextSizeSp
		} else {
			theme.bodyTextSizeSp
		}
	}

	private fun tableLineHeightPx(role: TableCellRole): Int {
		if (role == TableCellRole.None) return 0
		return context.textLineHeightPx(tableTextSize(role), theme.tableCellLineHeightMultiplier)
	}

	private fun codeBlock(code: String): TextView {
		return text(expandTabs(code, theme.codeBlockTabSize), theme.codeBlockTextSizeSp).apply {
			typeface = android.graphics.Typeface.MONOSPACE
			setPadding(
				context.dp(theme.codeBlockPaddingHorizontalDp),
				context.dp(theme.codeBlockPaddingVerticalDp),
				context.dp(theme.codeBlockPaddingHorizontalDp),
				context.dp(theme.codeBlockPaddingVerticalDp)
			)
			background = fill(0xffeeeeea.toInt())
		}
	}

	private fun firstParagraphText(item: JSONObject): String {
		val children = item.optJSONArray("children") ?: return plainText(item).trim()
		for (i in 0 until children.length()) {
			val child = children.getJSONObject(i)
			if (child.optString("kind") == "paragraph") return plainText(child).trim()
		}
		return plainText(item).trim()
	}

	private fun inlineChildrenOf(node: JSONObject): JSONArray? {
		val children = node.optJSONArray("children") ?: return null
		for (i in 0 until children.length()) {
			val child = children.getJSONObject(i)
			if (child.optString("kind") == "paragraph") return child.optJSONArray("children")
		}
		return children
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

	private fun text(value: String, sizeSp: Float, allowCjkSpacing: Boolean = false): TextView {
		return TextView(context).apply {
			text = processedText(value, theme, allowCjkSpacing)
			textSize = sizeSp
			setTextColor(0xff1b1b1b.toInt())
			typeface = typefaceFor(context, theme)
			applyMorphTextMetrics(sizeSp, theme.bodyLineHeightMultiplier)
		}
	}

	private fun lineHeightMultiplier(sizeSp: Float, lineHeightSp: Float): Float {
		return if (sizeSp <= 0f) 1f else lineHeightSp / sizeSp
	}

	private fun spacer(height: Int): View {
		return View(context).apply {
			layoutParams = LinearLayout.LayoutParams(1, context.dp(height))
		}
	}

	private fun rule(): View {
		return View(context).apply {
			background = fill(0xffb7b7b0.toInt())
			layoutParams = LinearLayout.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT,
				context.dp(1)
			).apply {
				setMargins(0, context.dp(14), 0, context.dp(14))
			}
		}
	}

	private fun verticalGroup(left: Int, top: Int, right: Int, bottom: Int): LinearLayout {
		return LinearLayout(context).apply {
			orientation = LinearLayout.VERTICAL
			setPadding(left, top, right, bottom)
		}
	}
}
