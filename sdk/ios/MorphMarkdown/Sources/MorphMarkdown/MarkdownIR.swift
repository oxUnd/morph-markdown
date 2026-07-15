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
}
