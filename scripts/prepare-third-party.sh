#!/bin/sh
set -eu

DEFAULT_MATHJAX_GIT_URL="https://github.com/oxUnd/mathjax-c.git"
DEFAULT_MATHJAX_GIT_REF="ea692adccc0eb56ac53261c5880d93094d22e43e"

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
THIRD_PARTY_DIR="$REPO_DIR/.third_party"
MATHJAX_DIR="$THIRD_PARTY_DIR/mathjax-c"

MATHJAX_GIT_URL="${MORPH_MATHJAX_GIT_URL:-$DEFAULT_MATHJAX_GIT_URL}"
MATHJAX_GIT_REF="${MORPH_MATHJAX_GIT_REF:-$DEFAULT_MATHJAX_GIT_REF}"

mkdir -p "$THIRD_PARTY_DIR"

if [ ! -d "$MATHJAX_DIR/.git" ]; then
	rm -rf "$MATHJAX_DIR"
	git clone "$MATHJAX_GIT_URL" "$MATHJAX_DIR"
fi

cd "$MATHJAX_DIR"
current_url=$(git config --get remote.origin.url || true)
if [ "$current_url" != "$MATHJAX_GIT_URL" ]; then
	git remote set-url origin "$MATHJAX_GIT_URL"
fi

git fetch --tags origin
git checkout --detach "$MATHJAX_GIT_REF"

if [ ! -f "$MATHJAX_DIR/include/mathjax.h" ]; then
	echo "ERROR: missing MathJax-C header at $MATHJAX_DIR/include/mathjax.h" >&2
	exit 1
fi

if [ ! -f "$MATHJAX_DIR/fonts/STIXTwoMath-Regular.ttf" ]; then
	echo "ERROR: missing MathJax-C font at $MATHJAX_DIR/fonts/STIXTwoMath-Regular.ttf" >&2
	exit 1
fi

echo "Prepared MathJax-C at $MATHJAX_DIR"
