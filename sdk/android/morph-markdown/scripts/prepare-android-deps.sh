#!/bin/sh
set -eu

FREETYPE_VERSION=2.14.3
HARFBUZZ_VERSION=14.2.1
DEFAULT_API_LEVEL=26

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SDK_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
BUILD_DIR="$SDK_DIR/.build/vendor-android-src"
OUT_ROOT="$SDK_DIR/.build/vendor-android"
NDK_PATH="${ANDROID_NDK_ROOT:-${ANDROID_NDK_HOME:-}}"
API_LEVEL="${MORPH_ANDROID_API_LEVEL:-$DEFAULT_API_LEVEL}"
ABIS="${MORPH_ANDROID_ABIS:-arm64-v8a}"

if [ -z "$NDK_PATH" ]; then
	for candidate in "$HOME/Library/Android/sdk/ndk"/*; do
		if [ -d "$candidate/toolchains/llvm/prebuilt" ]; then
			NDK_PATH="$candidate"
		fi
	done
fi

if [ ! -d "$NDK_PATH/toolchains/llvm/prebuilt" ]; then
	echo "ERROR: Android NDK not found. Set ANDROID_NDK_ROOT or ANDROID_NDK_HOME." >&2
	exit 1
fi

case "$(uname -s)" in
	Darwin)
		if [ -d "$NDK_PATH/toolchains/llvm/prebuilt/darwin-$(uname -m)" ]; then
			HOST_TAG="darwin-$(uname -m)"
		else
			HOST_TAG="darwin-x86_64"
		fi
		;;
	Linux)
		HOST_TAG="linux-x86_64"
		;;
	*)
		echo "ERROR: unsupported host OS $(uname -s)" >&2
		exit 1
		;;
esac

TOOLCHAIN="$NDK_PATH/toolchains/llvm/prebuilt/$HOST_TAG"
if [ ! -d "$TOOLCHAIN" ]; then
	echo "ERROR: Android NDK toolchain not found at $TOOLCHAIN" >&2
	exit 1
fi

NPROC=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

mkdir -p "$BUILD_DIR" "$OUT_ROOT"

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
		tar -xf "$archive" -C "$BUILD_DIR"
	fi
}

abi_to_triple() {
	case "$1" in
		arm64-v8a) echo "aarch64-linux-android" ;;
		armeabi-v7a) echo "armv7a-linux-androideabi" ;;
		x86_64) echo "x86_64-linux-android" ;;
		x86) echo "i686-linux-android" ;;
		*) echo "ERROR: unsupported ABI $1" >&2; return 1 ;;
	esac
}

abi_to_cmake_arch() {
	case "$1" in
		arm64-v8a) echo "aarch64" ;;
		armeabi-v7a) echo "arm" ;;
		x86_64) echo "x86_64" ;;
		x86) echo "i686" ;;
		*) echo "ERROR: unsupported ABI $1" >&2; return 1 ;;
	esac
}

make_cross_file() {
	out=$1
	cpu_family=$2
	cpu=$3
	sed \
		-e "s|@TOOLCHAIN@|$TOOLCHAIN|g" \
		-e "s|@CPU_FAMILY@|$cpu_family|g" \
		-e "s|@CPU@|$cpu|g" \
		"$SCRIPT_DIR/meson-android-cross-template.ini" > "$out"
}

download "https://downloads.sourceforge.net/project/freetype/freetype2/$FREETYPE_VERSION/freetype-$FREETYPE_VERSION.tar.xz" \
	"$BUILD_DIR/freetype-$FREETYPE_VERSION.tar.xz"
download "https://github.com/harfbuzz/harfbuzz/releases/download/$HARFBUZZ_VERSION/harfbuzz-$HARFBUZZ_VERSION.tar.xz" \
	"$BUILD_DIR/harfbuzz-$HARFBUZZ_VERSION.tar.xz"
extract_once "$BUILD_DIR/freetype-$FREETYPE_VERSION.tar.xz" "$BUILD_DIR/freetype-$FREETYPE_VERSION"
extract_once "$BUILD_DIR/harfbuzz-$HARFBUZZ_VERSION.tar.xz" "$BUILD_DIR/harfbuzz-$HARFBUZZ_VERSION"

for ABI in $ABIS; do
	TRIPLE=$(abi_to_triple "$ABI")
	CMAKE_ARCH=$(abi_to_cmake_arch "$ABI")
	PREFIX="$OUT_ROOT/$ABI"
	FT_BUILD="$BUILD_DIR/freetype-build-$ABI"
	HB_BUILD="$BUILD_DIR/harfbuzz-build-$ABI"
	CROSS_FILE="$BUILD_DIR/android-$ABI-cross.ini"

	if [ -f "$PREFIX/lib/pkgconfig/freetype2.pc" ] &&
		[ -f "$PREFIX/lib/pkgconfig/harfbuzz.pc" ]; then
		echo "Android freetype/harfbuzz already prepared for $ABI at $PREFIX"
		continue
	fi

	echo "Building freetype for $ABI"
	cmake -S "$BUILD_DIR/freetype-$FREETYPE_VERSION" -B "$FT_BUILD" -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
		-DANDROID_ABI="$ABI" \
		-DANDROID_PLATFORM="android-$API_LEVEL" \
		-DCMAKE_INSTALL_PREFIX="$PREFIX" \
		-DBUILD_SHARED_LIBS=OFF \
		-DFT_DISABLE_ZLIB=TRUE \
		-DFT_DISABLE_BZIP2=TRUE \
		-DFT_DISABLE_PNG=TRUE \
		-DFT_DISABLE_HARFBUZZ=TRUE \
		-DFT_DISABLE_BROTLI=TRUE
	cmake --build "$FT_BUILD" --target install --parallel "$NPROC"

	echo "Building harfbuzz for $ABI"
	rm -rf "$HB_BUILD"
	make_cross_file "$CROSS_FILE" "$CMAKE_ARCH" "$CMAKE_ARCH"
	PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig" meson setup "$HB_BUILD" "$BUILD_DIR/harfbuzz-$HARFBUZZ_VERSION" \
		--cross-file "$CROSS_FILE" \
		--prefix="$PREFIX" \
		--default-library=static \
		--buildtype=release \
		-Dc_args="--target=$TRIPLE$API_LEVEL --sysroot=$TOOLCHAIN/sysroot -fPIC" \
		-Dcpp_args="--target=$TRIPLE$API_LEVEL --sysroot=$TOOLCHAIN/sysroot -fPIC" \
		-Dc_link_args="--target=$TRIPLE$API_LEVEL --sysroot=$TOOLCHAIN/sysroot" \
		-Dcpp_link_args="--target=$TRIPLE$API_LEVEL --sysroot=$TOOLCHAIN/sysroot" \
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
	meson compile -C "$HB_BUILD" -j "$NPROC"
	meson install -C "$HB_BUILD"

	echo "Prepared Android native dependencies for $ABI at $PREFIX"
done
