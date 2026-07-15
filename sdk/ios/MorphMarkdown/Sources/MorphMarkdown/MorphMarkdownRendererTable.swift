#if canImport(UIKit)
import UIKit

extension MorphMarkdownRenderer {
	func table(_ node: MarkdownNode) -> UIView {
		let table = MarkdownTableView(theme: theme)
		populateTable(table, rows: node.children)
		if !theme.tableHorizontalScroll {
			configureTableSizing(table)
			return table
		}
		let scroll = UIScrollView()
		scroll.alwaysBounceHorizontal = false
		scroll.showsHorizontalScrollIndicator = true
		scroll.addSubview(table)
		let wrapper = TableScrollWrapper(scrollView: scroll, tableView: table, theme: theme)
		return wrapper
	}

	func populateTable(_ table: MarkdownTableView, rows: [MarkdownNode]) {
		for (index, row) in rows.enumerated() {
			table.beginRow(header: index == 0)
			addTableCells(table, cells: row.children, header: index == 0)
		}
	}

	func addTableCells(_ table: MarkdownTableView, cells: [MarkdownNode], header: Bool) {
		let role: TableCellRole = header ? .header : .body
		for cellNode in cells {
			let cell = InlineLayoutView()
			cell.lineSpacing = theme.tableCellLineSpacing
			cell.minLineHeight = tableTextSize(role) * theme.tableCellLineHeightMultiplier
			cell.contentInsets = UIEdgeInsets(top: theme.tableCellPaddingVertical,
							 left: theme.tableCellPaddingHorizontal,
							 bottom: theme.tableCellPaddingVertical,
							 right: theme.tableCellPaddingHorizontal)
			populateInline(cell, children: cellNode.inlineChildren, role: role)
			table.addCell(cell)
		}
	}

	private func configureTableSizing(_ table: UIView) {
		table.setContentHuggingPriority(.required, for: .vertical)
		table.setContentCompressionResistancePriority(.required, for: .vertical)
	}

	private func tableTextSize(_ role: TableCellRole) -> CGFloat {
		if role == .header {
			return theme.tableStyle.headerTextSize ?? theme.bodyTextSize
		}
		return theme.tableStyle.bodyTextSize ?? theme.bodyTextSize
	}
}

final class TableScrollWrapper: UIView {
	private let scrollView: UIScrollView
	private let tableView: MarkdownTableView
	private let theme: MorphMarkdownTheme

	init(scrollView: UIScrollView, tableView: MarkdownTableView, theme: MorphMarkdownTheme) {
		self.scrollView = scrollView
		self.tableView = tableView
		self.theme = theme
		super.init(frame: .zero)
		clipsToBounds = true
		addSubview(scrollView)
		setContentHuggingPriority(.required, for: .vertical)
		setContentCompressionResistancePriority(.required, for: .vertical)
	}

	required init?(coder: NSCoder) {
		return nil
	}

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		let width = resolvedWidth(size.width)
		tableView.viewportWidthHint = width
		let tableSize = tableView.sizeThatFits(CGSize(width: width, height: size.height))
		scrollView.contentSize = contentSize(for: tableSize, viewportWidth: width)
		return CGSize(width: width, height: tableSize.height + theme.tableTopSpacing + theme.tableBottomSpacing)
	}

	override var intrinsicContentSize: CGSize {
		let width = bounds.width > 0 ? bounds.width : UIView.noIntrinsicMetric
		return sizeThatFits(CGSize(width: width, height: UIView.noIntrinsicMetric))
	}

	override func systemLayoutSizeFitting(
		_ targetSize: CGSize,
		withHorizontalFittingPriority horizontalFittingPriority: UILayoutPriority,
		verticalFittingPriority: UILayoutPriority
	) -> CGSize {
		let width = targetSize.width > 0 ? targetSize.width : bounds.width
		return sizeThatFits(CGSize(width: width, height: targetSize.height))
	}

	override func layoutSubviews() {
		super.layoutSubviews()
		tableView.viewportWidthHint = bounds.width
		let fit = tableView.sizeThatFits(CGSize(width: bounds.width, height: CGFloat.greatestFiniteMagnitude))
		let content = contentSize(for: fit, viewportWidth: bounds.width)
		scrollView.frame = CGRect(x: 0, y: theme.tableTopSpacing,
					  width: bounds.width,
					  height: max(0, bounds.height - theme.tableTopSpacing - theme.tableBottomSpacing))
		tableView.frame = CGRect(origin: .zero, size: content)
		scrollView.contentSize = content
		scrollView.isScrollEnabled = !theme.tableCellWrap && content.width > bounds.width + 0.5
		scrollView.showsHorizontalScrollIndicator = scrollView.isScrollEnabled
		invalidateHeightIfNeeded(contentHeight: content.height)
	}

	private func resolvedWidth(_ width: CGFloat) -> CGFloat {
		if width > 0, width < CGFloat.greatestFiniteMagnitude {
			return width
		}
		if bounds.width > 0 {
			return bounds.width
		}
		return tableView.sizeThatFits(CGSize(width: UIView.noIntrinsicMetric,
						    height: CGFloat.greatestFiniteMagnitude)).width
	}

	private func contentSize(for tableSize: CGSize, viewportWidth: CGFloat) -> CGSize {
		if viewportWidth <= 0 {
			return tableSize
		}
		if theme.tableCellWrap {
			return CGSize(width: viewportWidth, height: tableSize.height)
		}
		return CGSize(width: max(tableSize.width, viewportWidth), height: tableSize.height)
	}

	private func invalidateHeightIfNeeded(contentHeight: CGFloat) {
		let expected = contentHeight + theme.tableTopSpacing + theme.tableBottomSpacing
		guard expected > bounds.height + 0.5 else {
			return
		}
		invalidateIntrinsicContentSize()
		superview?.setNeedsLayout()
	}
}
#endif
