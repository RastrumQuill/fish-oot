#!/usr/bin/env bash
# Builds the modded ROM and creates fish-oot.bps against the original
# NTSC 1.2 (US) ROM in baseroms/ntsc-1.2/.
#
# Usage: fishmod/build.sh [extra make args]
# Needs `flips` on PATH (or FLIPS=/path/to/flips).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION=ntsc-1.2
REGION=US
BASEROM_DIR="baseroms/$VERSION"
OUT_DIR="fishmod/out"
FLIPS="${FLIPS:-flips}"

if ! command -v "$FLIPS" >/dev/null 2>&1; then
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

# The patch is always made against a big-endian .z64, whatever the input byte order.
python3 fishmod/tools/to_z64.py "$baserom" "$OUT_DIR/original.z64"

# make doesn't track flag changes, so start from a clean build the first time
# (or after a matching build of the same version).
STAMP="build/$VERSION/.fishmod"
if [ ! -f "$STAMP" ]; then
    make VERSION=$VERSION clean
fi
make VERSION=$VERSION REGION=$REGION NON_MATCHING=1 compress -j"$(nproc)" "$@"

touch "$STAMP"

cp "build/$VERSION/oot-$VERSION-compressed.z64" "$OUT_DIR/fish-oot.z64"
rm -f "$OUT_DIR/fish-oot.bps"
"$FLIPS" --create --bps "$OUT_DIR/original.z64" "$OUT_DIR/fish-oot.z64" "$OUT_DIR/fish-oot.bps"

echo "Built $OUT_DIR/fish-oot.z64 and $OUT_DIR/fish-oot.bps"
