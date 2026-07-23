import Foundation

struct MarkdownNode: Decodable {
	let id: Int?
	let kind: String
	let literal: String?
	let url: String?
	let title: String?
	let info: String?
	let sourcepos: String?
	let level: Int?
	let listType: String?
	let start: Int?
	let checked: Bool?
	let children: [MarkdownNode]

	enum CodingKeys: String, CodingKey {
		case id
		case kind
		case literal
		case url
		case title
		case info
		case sourcepos
		case level
		case listType = "list_type"
		case start
		case checked
		case children
	}

	init(from decoder: Decoder) throws {
		let container = try decoder.container(keyedBy: CodingKeys.self)
		id = try container.decodeIfPresent(Int.self, forKey: .id)
		kind = try container.decode(String.self, forKey: .kind)
		literal = try container.decodeIfPresent(String.self, forKey: .literal)
		url = try container.decodeIfPresent(String.self, forKey: .url)
		title = try container.decodeIfPresent(String.self, forKey: .title)
		info = try container.decodeIfPresent(String.self, forKey: .info)
		sourcepos = try container.decodeIfPresent(String.self, forKey: .sourcepos)
		level = try container.decodeIfPresent(Int.self, forKey: .level)
		listType = try container.decodeIfPresent(String.self, forKey: .listType)
		start = try container.decodeIfPresent(Int.self, forKey: .start)
		checked = try container.decodeIfPresent(Bool.self, forKey: .checked)
		children = try container.decodeIfPresent([MarkdownNode].self, forKey: .children) ?? []
	}
}

extension MarkdownNode {
	var plainText: String {
		let own = literal ?? ""
		return children.reduce(own) { $0 + $1.plainText }
	}

	var inlineChildren: [MarkdownNode] {
		if let paragraph = children.first(where: { $0.kind == "paragraph" }) {
			return paragraph.children
		}
		return children
	}

	var renderedPlainText: String {
		switch kind {
		case "document":
			return joinedBlocks(children)
		case "heading", "paragraph", "block_quote":
			return children.map(\.renderedPlainText).joined()
		case "list":
			let ordered = listType == "ordered"
			let first = start ?? 1
			return children.enumerated().map { index, child in
				let marker = ordered ? "\(first + index)." : "•"
				return "\(marker) \(child.renderedPlainText)"
			}.joined(separator: "\n")
		case "item":
			return joinedBlocks(children)
		case "table":
			let headerCells = children.prefix { $0.kind == "table_cell" }
			let rows = children.dropFirst(headerCells.count)
			var lines: [String] = []
			if !headerCells.isEmpty {
				lines.append(headerCells.map(\.renderedPlainText).joined(separator: "\t"))
			}
			lines.append(contentsOf: rows.map(\.renderedPlainText))
			return lines.joined(separator: "\n")
		case "table_row":
			return children.map(\.renderedPlainText).joined(separator: "\t")
		case "table_header":
			return children.map(\.renderedPlainText).joined(separator: "\t")
		case "table_body":
			return children.map(\.renderedPlainText).joined(separator: "\n")
		case "table_cell":
			return children.map(\.renderedPlainText).joined()
		case "soft_break", "hard_break":
			return "\n"
		case "image", "thematic_break":
			return ""
		default:
			if let literal {
				return literal
			}
			return children.map(\.renderedPlainText).joined()
		}
	}

	private func joinedBlocks(_ nodes: [MarkdownNode]) -> String {
		nodes
			.map(\.renderedPlainText)
			.filter { !$0.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty }
			.joined(separator: "\n\n")
	}
}
