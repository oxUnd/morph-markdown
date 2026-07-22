#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
	echo "usage: $0 <version> <release-base-url>" >&2
	exit 2
fi

VERSION=$1
BASE_URL=${2%/}
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PACKAGE_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
XCFRAMEWORK="$PACKAGE_DIR/.build/native/MorphMarkdownNativeCore.xcframework"
DIST_DIR="$PACKAGE_DIR/.build/release"
ARCHIVE="$DIST_DIR/MorphMarkdownNativeCore-$VERSION.zip"
MANIFEST="$DIST_DIR/Package.swift"

if [ ! -d "$XCFRAMEWORK" ]; then
	echo "missing $XCFRAMEWORK; run scripts/build-native-ios.sh first" >&2
	exit 1
fi

mkdir -p "$DIST_DIR"
ditto -c -k --sequesterRsrc --keepParent "$XCFRAMEWORK" "$ARCHIVE"
CHECKSUM=$(swift package compute-checksum "$ARCHIVE")
BINARY_URL="$BASE_URL/MorphMarkdownNativeCore-$VERSION.zip"
sed \
	-e "s|@BINARY_URL@|$BINARY_URL|g" \
	-e "s|@BINARY_CHECKSUM@|$CHECKSUM|g" \
	"$PACKAGE_DIR/Package.release.swift.template" > "$MANIFEST"

echo "archive: $ARCHIVE"
echo "checksum: $CHECKSUM"
echo "release manifest: $MANIFEST"
