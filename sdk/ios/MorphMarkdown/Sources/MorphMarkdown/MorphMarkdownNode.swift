import Foundation

public struct MorphMarkdownNode: Identifiable, Sendable {
	public let id: Int
	public let kind: String
	public let literal: String
	public let level: Int
	public let checked: Bool
	public let listType: String
	public let start: Int
	public let url: String
	public let children: [MorphMarkdownNode]

	public init(json: [String: Any]) {
		id = json["id"] as? Int ?? 0
		kind = json["kind"] as? String ?? "unknown"
		literal = json["literal"] as? String ?? ""
		level = json["level"] as? Int ?? 1
		checked = json["checked"] as? Bool ?? false
		listType = json["list_type"] as? String ?? "bullet"
		start = json["start"] as? Int ?? 1
		url = json["url"] as? String ?? ""
		let rawChildren = json["children"] as? [[String: Any]] ?? []
		children = rawChildren.map { MorphMarkdownNode(json: $0) }
	}
}

public enum MorphMarkdownJSON {
	public static func decode(_ json: String) -> [MorphMarkdownNode] {
		guard let data = json.data(using: .utf8) else { return [] }
		guard let object = try? JSONSerialization.jsonObject(with: data) as? [String: Any] else {
			return []
		}
		let children = object["children"] as? [[String: Any]] ?? []
		return children.map { MorphMarkdownNode(json: $0) }
	}
}
