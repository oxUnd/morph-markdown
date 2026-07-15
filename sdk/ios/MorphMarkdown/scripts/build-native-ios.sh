#!/bin/sh
set -eu

FREETYPE_VERSION=2.14.3
HARFBUZZ_VERSION=14.2.1
MIN_IOS_VERSION=15.0

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PACKAGE_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
REPO_DIR=$(CDPATH= cd -- "$PACKAGE_DIR/../../.." && pwd)
MORPH_ROOT_DIR=$(CDPATH= cd -- "$REPO_DIR/../.." && pwd)
MATHJAX_DIR="$MORPH_ROOT_DIR/vendor/mathjax-c"
SRC_DIR="$PACKAGE_DIR/.build/vendor-src"
VENDOR_DIR="$PACKAGE_DIR/.build/vendor-ios"
NATIVE_DIR="$PACKAGE_DIR/.build/native"
NATIVE_IOS_DIR="$PACKAGE_DIR/.build/native-ios"

mkdir -p "$SRC_DIR" "$VENDOR_DIR" "$NATIVE_DIR/include" "$NATIVE_IOS_DIR/combined"

download() {
	url=$1
	out=$2
	if [ ! -f "$out" ]; then
		curl -L "$url" -o "$out"
	fi
}

extract_once() {
	archive=$1
	dir=$2
	if [ ! -d "$dir" ]; then
		tar -xf "$archive" -C "$SRC_DIR"
	fi
}

sdk_path() {
	xcrun --sdk "$1" --show-sdk-path
}

make_cross_file() {
	out=$1
	target=$2
	sdkroot=$3
	cpu_family=$4
	cpu=$5
	sed \
		-e "s|@CC@|/usr/bin/clang|g" \
		-e "s|@CXX@|/usr/bin/clang++|g" \
		-e "s|@AR@|/usr/bin/ar|g" \
		-e "s|@STRIP@|/usr/bin/strip|g" \
		-e "s|@PKG_CONFIG@|$(command -v pkg-config)|g" \
		-e "s|@TARGET@|$target|g" \
		-e "s|@SDKROOT@|$sdkroot|g" \
		-e "s|@MIN_VERSION@|$MIN_IOS_VERSION|g" \
		-e "s|@CPU_FAMILY@|$cpu_family|g" \
		-e "s|@CPU@|$cpu|g" \
		"$SCRIPT_DIR/meson-ios-cross-template.ini" > "$out"
}

build_freetype() {
	name=$1
	sysroot=$2
	arch=$3
	prefix="$VENDOR_DIR/freetype-$name"
	build="$VENDOR_DIR/freetype-$name-build"
	cmake -S "$SRC_DIR/freetype-$FREETYPE_VERSION" -B "$build" -G Ninja \
		-DCMAKE_SYSTEM_NAME=iOS \
		-DCMAKE_OSX_SYSROOT="$sysroot" \
		-DCMAKE_OSX_ARCHITECTURES="$arch" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_IOS_VERSION" \
		-DCMAKE_INSTALL_PREFIX="$prefix" \
		-DBUILD_SHARED_LIBS=OFF \
		-DFT_DISABLE_ZLIB=TRUE \
		-DFT_DISABLE_BZIP2=TRUE \
		-DFT_DISABLE_PNG=TRUE \
		-DFT_DISABLE_HARFBUZZ=TRUE \
		-DFT_DISABLE_BROTLI=TRUE
	cmake --build "$build" --target install
}

build_freetype_macos() {
	prefix="$VENDOR_DIR/freetype-macos"
	build="$VENDOR_DIR/freetype-macos-build"
	cmake -S "$SRC_DIR/freetype-$FREETYPE_VERSION" -B "$build" -G Ninja \
		-DCMAKE_INSTALL_PREFIX="$prefix" \
		-DCMAKE_OSX_ARCHITECTURES=arm64 \
		-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
		-DBUILD_SHARED_LIBS=OFF \
		-DFT_DISABLE_ZLIB=TRUE \
		-DFT_DISABLE_BZIP2=TRUE \
		-DFT_DISABLE_PNG=TRUE \
		-DFT_DISABLE_HARFBUZZ=TRUE \
		-DFT_DISABLE_BROTLI=TRUE
	cmake --build "$build" --target install
}

build_harfbuzz() {
	name=$1
	target=$2
	sysroot=$3
	cpu_family=$4
	cpu=$5
	ft_prefix="$VENDOR_DIR/freetype-$name"
	prefix="$VENDOR_DIR/harfbuzz-$name"
	build="$VENDOR_DIR/harfbuzz-$name-build"
	cross="$VENDOR_DIR/ios-$name-cross.ini"
	make_cross_file "$cross" "$target" "$sysroot" "$cpu_family" "$cpu"
	if [ -d "$build/meson-info" ]; then
		setup_action=--reconfigure
	else
		setup_action=
	fi
	PKG_CONFIG_LIBDIR="$ft_prefix/lib/pkgconfig" meson setup $setup_action "$build" "$SRC_DIR/harfbuzz-$HARFBUZZ_VERSION" \
		--cross-file "$cross" \
		--prefix="$prefix" \
		--default-library=static \
		--buildtype=release \
		-Dglib=disabled \
		-Dgobject=disabled \
		-Dicu=disabled \
		-Dcairo=disabled \
		-Dchafa=disabled \
		-Dgraphite2=disabled \
		-Ddocs=disabled \
		-Dtests=disabled \
		-Dutilities=disabled \
		-Dintrospection=disabled \
		-Dbenchmark=disabled \
		-Dfreetype=enabled \
		-Dpng=disabled \
		-Dzlib=disabled \
		-Dsubset=disabled
	meson compile -C "$build"
	meson install -C "$build"
}

build_harfbuzz_macos() {
	ft_prefix="$VENDOR_DIR/freetype-macos"
	prefix="$VENDOR_DIR/harfbuzz-macos"
	build="$VENDOR_DIR/harfbuzz-macos-build"
	if [ -d "$build/meson-info" ]; then
		setup_action=--reconfigure
	else
		setup_action=
	fi
	PKG_CONFIG_LIBDIR="$ft_prefix/lib/pkgconfig" meson setup $setup_action "$build" "$SRC_DIR/harfbuzz-$HARFBUZZ_VERSION" \
		--prefix="$prefix" \
		--default-library=static \
		--buildtype=release \
		-Dc_args=-mmacosx-version-min=14.0 \
		-Dcpp_args=-mmacosx-version-min=14.0 \
		-Dc_link_args=-mmacosx-version-min=14.0 \
		-Dcpp_link_args=-mmacosx-version-min=14.0 \
		-Dglib=disabled \
		-Dgobject=disabled \
		-Dicu=disabled \
		-Dcairo=disabled \
		-Dchafa=disabled \
		-Dgraphite2=disabled \
		-Ddocs=disabled \
		-Dtests=disabled \
		-Dutilities=disabled \
		-Dintrospection=disabled \
		-Dbenchmark=disabled \
		-Dfreetype=enabled \
		-Dpng=disabled \
		-Dzlib=disabled \
		-Dsubset=disabled
	meson compile -C "$build"
	meson install -C "$build"
}

build_morph_ios() {
	name=$1
	sysroot=$2
	arch=$3
	ft_prefix="$VENDOR_DIR/freetype-$name"
	hb_prefix="$VENDOR_DIR/harfbuzz-$name"
	build="$NATIVE_IOS_DIR/$name-build"
	PKG_CONFIG_LIBDIR="$ft_prefix/lib/pkgconfig:$hb_prefix/lib/pkgconfig" cmake -S "$REPO_DIR" -B "$build" -G Ninja \
		-DCMAKE_SYSTEM_NAME=iOS \
		-DCMAKE_OSX_SYSROOT="$sysroot" \
		-DCMAKE_OSX_ARCHITECTURES="$arch" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_IOS_VERSION" \
		-DCMAKE_MACOSX_BUNDLE=OFF \
		-DMORPH_MARKDOWN_BUILD_KITTY=ON \
		-DMORPH_MARKDOWN_BUILD_TESTS=OFF
	cmake --build "$build" --target morph-markdown-render morph-mathjax
}

combine_slice() {
	name=$1
	out=$2
	ft_prefix="$VENDOR_DIR/freetype-$name"
	hb_prefix="$VENDOR_DIR/harfbuzz-$name"
	build="$NATIVE_IOS_DIR/$name-build"
	libtool -static -o "$out" \
		"$build/libmorph-markdown-render.a" \
		"$build/libmorph-mathjax.a" \
		"$build/_deps/cmark-gfm-build/src/libcmark-gfm.a" \
		"$build/_deps/cmark-gfm-build/extensions/libcmark-gfm-extensions.a" \
		"$ft_prefix/lib/libfreetype.a" \
		"$hb_prefix/lib/libharfbuzz.a"
}

build_macos_slice() {
	ft_prefix="$VENDOR_DIR/freetype-macos"
	hb_prefix="$VENDOR_DIR/harfbuzz-macos"
	build="$NATIVE_IOS_DIR/macos-build"
	PKG_CONFIG_LIBDIR="$ft_prefix/lib/pkgconfig:$hb_prefix/lib/pkgconfig" cmake -S "$REPO_DIR" -B "$build" \
		-DMORPH_MARKDOWN_BUILD_KITTY=ON \
		-DMORPH_MARKDOWN_BUILD_TESTS=OFF \
		-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
	cmake --build "$build" --target morph-markdown-render morph-mathjax
	libtool -static -o "$NATIVE_IOS_DIR/combined/libMorphMarkdownNativeCore-macos.a" \
		"$build/libmorph-markdown-render.a" \
		"$build/libmorph-mathjax.a" \
		"$build/_deps/cmark-gfm-build/src/libcmark-gfm.a" \
		"$build/_deps/cmark-gfm-build/extensions/libcmark-gfm-extensions.a" \
		"$ft_prefix/lib/libfreetype.a" \
		"$hb_prefix/lib/libharfbuzz.a"
}

download "https://downloads.sourceforge.net/project/freetype/freetype2/$FREETYPE_VERSION/freetype-$FREETYPE_VERSION.tar.xz" \
	"$SRC_DIR/freetype-$FREETYPE_VERSION.tar.xz"
download "https://github.com/harfbuzz/harfbuzz/releases/download/$HARFBUZZ_VERSION/harfbuzz-$HARFBUZZ_VERSION.tar.xz" \
	"$SRC_DIR/harfbuzz-$HARFBUZZ_VERSION.tar.xz"
extract_once "$SRC_DIR/freetype-$FREETYPE_VERSION.tar.xz" "$SRC_DIR/freetype-$FREETYPE_VERSION"
extract_once "$SRC_DIR/harfbuzz-$HARFBUZZ_VERSION.tar.xz" "$SRC_DIR/harfbuzz-$HARFBUZZ_VERSION"

SIM_SDK=$(sdk_path iphonesimulator)
OS_SDK=$(sdk_path iphoneos)

build_freetype sim-arm64 "$SIM_SDK" arm64
build_harfbuzz sim-arm64 "arm64-apple-ios${MIN_IOS_VERSION}-simulator" "$SIM_SDK" aarch64 arm64
build_morph_ios sim-arm64 iphonesimulator arm64
combine_slice sim-arm64 "$NATIVE_IOS_DIR/combined/libMorphMarkdownNativeCore-sim-arm64.a"

build_freetype sim-x86_64 "$SIM_SDK" x86_64
build_harfbuzz sim-x86_64 "x86_64-apple-ios${MIN_IOS_VERSION}-simulator" "$SIM_SDK" x86_64 x86_64
build_morph_ios sim-x86_64 iphonesimulator x86_64
combine_slice sim-x86_64 "$NATIVE_IOS_DIR/combined/libMorphMarkdownNativeCore-sim-x86_64.a"

lipo -create -output "$NATIVE_IOS_DIR/combined/libMorphMarkdownNativeCore-simulator.a" \
	"$NATIVE_IOS_DIR/combined/libMorphMarkdownNativeCore-sim-arm64.a" \
	"$NATIVE_IOS_DIR/combined/libMorphMarkdownNativeCore-sim-x86_64.a"

build_freetype ios-arm64 "$OS_SDK" arm64
build_harfbuzz ios-arm64 "arm64-apple-ios$MIN_IOS_VERSION" "$OS_SDK" aarch64 arm64
build_morph_ios ios-arm64 iphoneos arm64
combine_slice ios-arm64 "$NATIVE_IOS_DIR/combined/libMorphMarkdownNativeCore-ios-arm64.a"

build_freetype_macos
build_harfbuzz_macos
build_macos_slice

cp "$REPO_DIR/include/morph_markdown.h" "$NATIVE_DIR/include/"
cp "$MATHJAX_DIR/include/mathjax.h" "$NATIVE_DIR/include/"
rm -rf "$NATIVE_DIR/MorphMarkdownNativeCore.xcframework"
xcodebuild -create-xcframework \
	-library "$NATIVE_IOS_DIR/combined/libMorphMarkdownNativeCore-simulator.a" -headers "$NATIVE_DIR/include" \
	-library "$NATIVE_IOS_DIR/combined/libMorphMarkdownNativeCore-ios-arm64.a" -headers "$NATIVE_DIR/include" \
	-library "$NATIVE_IOS_DIR/combined/libMorphMarkdownNativeCore-macos.a" -headers "$NATIVE_DIR/include" \
	-output "$NATIVE_DIR/MorphMarkdownNativeCore.xcframework"

echo "Prepared $NATIVE_DIR/MorphMarkdownNativeCore.xcframework"
