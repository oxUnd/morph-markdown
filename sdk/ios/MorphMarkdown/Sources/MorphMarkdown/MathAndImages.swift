#if canImport(UIKit)
import UIKit
import MorphMarkdownNative

public protocol MorphMathRenderer: AnyObject {
	func render(latex: String, display: Bool, theme: MorphMarkdownTheme) -> UIView?
}

public final class MathJaxMathRenderer: MorphMathRenderer {
	public static let bundled: MathJaxMathRenderer? = MathJaxMathRenderer()
	private let fontPath: String
	private let imageCache: NSCache<NSString, UIImage> = {
		let cache = NSCache<NSString, UIImage>()
		cache.countLimit = 256
		cache.totalCostLimit = 16 * 1024 * 1024
		return cache
	}()

	public init?(fontPath: String) {
		self.fontPath = fontPath
	}

	public convenience init?(bundle: Bundle? = nil) {
		guard let path = Self.findFontPath(bundle: bundle) else {
			return nil
		}
		self.init(fontPath: path)
	}

	private static func findFontPath(bundle: Bundle?) -> String? {
		let roots = [bundle, Bundle.module, Bundle.main, Bundle(for: BundleToken.self)].compactMap { $0 } +
			Bundle.allBundles + Bundle.allFrameworks
		let candidates = roots + roots.flatMap(resourceBundles(in:))
		for candidate in candidates {
			if let path = candidate.path(forResource: "STIXTwoMath-Regular", ofType: "ttf") {
				return path
			}
		}
		return nil
	}

	private static func resourceBundles(in bundle: Bundle) -> [Bundle] {
		guard let root = bundle.resourceURL,
		      let urls = try? FileManager.default.contentsOfDirectory(
			at: root,
			includingPropertiesForKeys: nil,
			options: [.skipsHiddenFiles]
		      ) else {
			return []
		}
		return urls.compactMap { $0.pathExtension == "bundle" ? Bundle(url: $0) : nil }
	}

	public func render(latex: String, display: Bool, theme: MorphMarkdownTheme) -> UIView? {
		let scale = UIScreen.main.scale
		let pixelSize = theme.mathSize() * scale
		let key = "\(latex)\u{1f}\(display)\u{1f}\(pixelSize)\u{1f}\(theme.bodyTextColor)\u{1f}\(scale)" as NSString
		if let cached = imageCache.object(forKey: key) {
			let imageView = fixedImageView(cached)
			return display ? displayContainer(imageView) : imageView
		}
		guard let bitmap = morph_ios_render_latex(
			fontPath,
			latex,
			display ? 1 : 0,
			pixelSize,
			Self.rgbaColor(theme.bodyTextColor)
		) else {
			return nil
		}
		defer { morph_ios_bitmap_destroy(bitmap) }
		guard let image = image(from: bitmap.pointee, scale: scale) else {
			return nil
		}
		imageCache.setObject(image, forKey: key, cost: Int(bitmap.pointee.width * bitmap.pointee.height * 4))
		let imageView = fixedImageView(image)
		return display ? displayContainer(imageView) : imageView
	}

	private static func rgbaColor(_ argb: UInt32) -> UInt32 {
		return ((argb & 0x00ffffff) << 8) | ((argb >> 24) & 0xff)
	}

	private func image(from bitmap: morph_ios_bitmap, scale: CGFloat) -> UIImage? {
		guard let pixels = bitmap.pixels_rgba else {
			return nil
		}
		let width = Int(bitmap.width)
		let height = Int(bitmap.height)
		let count = width * height
		var bytes = [UInt8](repeating: 0, count: count * 4)
		for index in 0..<count {
			let pixel = pixels[index]
			bytes[index * 4] = UInt8((pixel >> 24) & 0xff)
			bytes[index * 4 + 1] = UInt8((pixel >> 16) & 0xff)
			bytes[index * 4 + 2] = UInt8((pixel >> 8) & 0xff)
			bytes[index * 4 + 3] = UInt8(pixel & 0xff)
		}
		return image(width: width, height: height, bytes: bytes, scale: scale)
	}

	private func image(width: Int, height: Int, bytes: [UInt8], scale: CGFloat) -> UIImage? {
		let data = Data(bytes)
		guard let provider = CGDataProvider(data: data as CFData) else {
			return nil
		}
		let colorSpace = CGColorSpaceCreateDeviceRGB()
		let info = CGBitmapInfo(rawValue: CGImageAlphaInfo.last.rawValue)
		guard let cgImage = CGImage(width: width, height: height, bitsPerComponent: 8,
					    bitsPerPixel: 32, bytesPerRow: width * 4,
					    space: colorSpace, bitmapInfo: info,
					    provider: provider, decode: nil,
					    shouldInterpolate: false, intent: .defaultIntent) else {
			return nil
		}
		return UIImage(cgImage: cgImage, scale: scale, orientation: .up)
	}

	private func fixedImageView(_ image: UIImage) -> UIImageView {
		let view = FixedSizeImageView(image: image, size: image.size)
		view.contentMode = .center
		view.frame.size = image.size
		return view
	}

	private func displayContainer(_ image: UIView) -> UIView {
		let size = CGSize(width: image.bounds.width, height: image.bounds.height + 24)
		let container = FixedSizeContainerView(size: size)
		container.addSubview(image)
		container.frame.size = size
		image.frame.origin = CGPoint(x: 0, y: 12)
		return container
	}
}

private final class FixedSizeImageView: UIImageView {
	private let fixedSize: CGSize

	init(image: UIImage, size: CGSize) {
		fixedSize = size
		super.init(image: image)
		frame.size = size
	}

	required init?(coder: NSCoder) {
		return nil
	}

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		return fixedSize
	}

	override var intrinsicContentSize: CGSize {
		return fixedSize
	}
}

private final class FixedSizeContainerView: UIView {
	private let fixedSize: CGSize

	init(size: CGSize) {
		fixedSize = size
		super.init(frame: CGRect(origin: .zero, size: size))
	}

	required init?(coder: NSCoder) {
		return nil
	}

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		return fixedSize
	}

	override var intrinsicContentSize: CGSize {
		return fixedSize
	}
}

private final class BundleToken {}

public protocol MorphImageLoader: AnyObject {
	func load(url: String, theme: MorphMarkdownTheme) -> UIView?
}

public final class FileImageLoader: MorphImageLoader {
	public static let shared = FileImageLoader()
	private let imageCache: NSCache<NSString, UIImage> = {
		let cache = NSCache<NSString, UIImage>()
		cache.countLimit = 64
		cache.totalCostLimit = 16 * 1024 * 1024
		return cache
	}()

	public init() {}

	public func load(url: String, theme: MorphMarkdownTheme) -> UIView? {
		let path = url.hasPrefix("file://") ? String(url.dropFirst("file://".count)) : url
		let attributes = try? FileManager.default.attributesOfItem(atPath: path)
		let modified = (attributes?[.modificationDate] as? Date)?.timeIntervalSince1970 ?? 0
		let fileSize = attributes?[.size] as? NSNumber ?? 0
		let key = "\(path)\u{1f}\(modified)\u{1f}\(fileSize)" as NSString
		let decoded = imageCache.object(forKey: key) ?? UIImage(contentsOfFile: path)
		guard let image = decoded else {
			return nil
		}
		let cost = Int(image.size.width * image.size.height * image.scale * image.scale * 4)
		imageCache.setObject(image, forKey: key, cost: cost)
		let view = ScalableImageView(image: image, maxSize: CGSize(width: theme.imageMaxWidth,
									   height: theme.imageMaxHeight))
		view.contentMode = .scaleAspectFit
		return ImagePaddingView(content: view, verticalPadding: 6)
	}
}

private final class ScalableImageView: UIImageView, TableIntrinsicOverride {
	private let originalSize: CGSize
	private let maxSize: CGSize

	init(image: UIImage, maxSize: CGSize) {
		originalSize = image.size
		self.maxSize = maxSize
		super.init(image: image)
		frame.size = fittedSize(originalSize, limit: maxSize)
	}

	required init?(coder: NSCoder) {
		return nil
	}

	var tableMinimumWidth: CGFloat {
		return fittedSize(originalSize, limit: maxSize).width
	}

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		let widthLimit = effectiveLimit(size.width, fallback: maxSize.width)
		let heightLimit = effectiveLimit(size.height, fallback: maxSize.height)
		return fittedSize(originalSize, limit: CGSize(width: widthLimit, height: heightLimit))
	}

	override var intrinsicContentSize: CGSize {
		return fittedSize(originalSize, limit: maxSize)
	}

	private func fittedSize(_ size: CGSize, limit: CGSize) -> CGSize {
		if size.width <= 0 || size.height <= 0 {
			return .zero
		}
		let scale = min(limit.width / size.width, limit.height / size.height, 1)
		return CGSize(width: size.width * scale, height: size.height * scale)
	}

	private func effectiveLimit(_ value: CGFloat, fallback: CGFloat) -> CGFloat {
		if value > 0, value < CGFloat.greatestFiniteMagnitude {
			return value
		}
		return fallback
	}
}

private final class ImagePaddingView: UIView, TableIntrinsicOverride {
	private let content: UIView
	private let verticalPadding: CGFloat

	init(content: UIView, verticalPadding: CGFloat) {
		self.content = content
		self.verticalPadding = verticalPadding
		super.init(frame: .zero)
		addSubview(content)
	}

	required init?(coder: NSCoder) {
		return nil
	}

	override func sizeThatFits(_ size: CGSize) -> CGSize {
		let contentSize = content.sizeThatFits(size)
		return CGSize(width: contentSize.width, height: contentSize.height + verticalPadding * 2)
	}

	var tableMinimumWidth: CGFloat {
		return (content as? TableIntrinsicOverride)?.tableMinimumWidth ?? content.sizeThatFits(.zero).width
	}

	override var intrinsicContentSize: CGSize {
		return sizeThatFits(.zero)
	}

	override func layoutSubviews() {
		super.layoutSubviews()
		let contentSize = content.sizeThatFits(bounds.size)
		content.frame = CGRect(x: 0, y: verticalPadding, width: contentSize.width, height: contentSize.height)
	}
}
#endif
