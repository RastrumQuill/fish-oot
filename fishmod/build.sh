#!/usr/bin/env bash
# Builds the modded ROM and creates fish-oot.bps against the original
# NTSC 1.2 (US) ROM in baseroms/ntsc-1.2/.
#
# Usage: fishmod/build.sh [--debug] [--script NAME] [extra make args]
#   --debug        build the debug harness (boots straight into Kokiri, debug text)
#   --script NAME  (implies --debug) play fishmod/scripts/NAME.inc.c as autoplay input
# Needs `flips` on PATH (or FLIPS=/path/to/flips) for the release patch.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION=ntsc-1.2
REGION=US
BASEROM_DIR="baseroms/$VERSION"
OUT_DIR="fishmod/out"
FLIPS="${FLIPS:-flips}"

DEBUG=0
SCRIPT=""
MAKE_ARGS=()
while [ $# -gt 0 ]; do
    case "$1" in
        --debug) DEBUG=1 ;;
        --script) DEBUG=1; SCRIPT="$2"; shift ;;
        *) MAKE_ARGS+=("$1") ;;
    esac
    shift
done

if [ "$DEBUG" = 0 ] && ! command -v "$FLIPS" >/dev/null 2>&1; then
    echo "error: flips not found (https://github.com/Alcaro/Flips); set FLIPS=/path/to/flips" >&2
    exit 1
fi

baserom=""
for ext in z64 n64 v64; do
    if [ -f "$BASEROM_DIR/baserom.$ext" ]; then
        baserom="$BASEROM_DIR/baserom.$ext"
        break
    fi
done
if [ -z "$baserom" ]; then
    echo "error: put your NTSC 1.2 (US) ROM at $BASEROM_DIR/baserom.z64 and run 'make setup VERSION=$VERSION'" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

# make doesn't track flag changes, so rebuild from clean whenever the build mode changes
# (including the first build after a matching build).
STAMP="build/$VERSION/.fishmod"
MODE="debug=$DEBUG"
if [ ! -f "$STAMP" ] || [ "$(cat "$STAMP")" != "$MODE" ]; then
    make VERSION=$VERSION clean
fi
# The autoplay script is #included by fishmod_debug.c; force it to rebuild when the script changes.
SCRIPT_STAMP="build/$VERSION/.fishmod-script"
if [ ! -f "$SCRIPT_STAMP" ] || [ "$(cat "$SCRIPT_STAMP")" != "$SCRIPT" ]; then
    rm -f "build/$VERSION/src/code/fishmod_debug.o"
fi
if [ -n "$SCRIPT" ]; then
    touch "fishmod/scripts/$SCRIPT.inc.c"
fi

make VERSION=$VERSION REGION=$REGION NON_MATCHING=1 FISHMOD_DEBUG=$DEBUG FISHMOD_SCRIPT="$SCRIPT" \
    compress -j"$(nproc)" ${MAKE_ARGS[@]+"${MAKE_ARGS[@]}"}

mkdir -p "build/$VERSION"
echo "$MODE" > "$STAMP"
echo "$SCRIPT" > "$SCRIPT_STAMP"

if [ "$DEBUG" = 1 ]; then
    name="fish-oot-debug${SCRIPT:+-$SCRIPT}"
    cp "build/$VERSION/oot-$VERSION-compressed.z64" "$OUT_DIR/$name.z64"
    echo "Built $OUT_DIR/$name.z64"
    exit 0
fi

# The patch is always made against a big-endian .z64, whatever the input byte order.
python3 fishmod/tools/to_z64.py "$baserom" "$OUT_DIR/original.z64"
cp "build/$VERSION/oot-$VERSION-compressed.z64" "$OUT_DIR/fish-oot.z64"
rm -f "$OUT_DIR/fish-oot.bps"
"$FLIPS" --create --bps "$OUT_DIR/original.z64" "$OUT_DIR/fish-oot.z64" "$OUT_DIR/fish-oot.bps" || true
test -f "$OUT_DIR/fish-oot.bps"

echo "Built $OUT_DIR/fish-oot.z64 and $OUT_DIR/fish-oot.bps"
