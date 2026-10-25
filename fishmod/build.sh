#!/usr/bin/env bash
# Builds the modded ROM and creates fish-oot.bps against the original
# NTSC 1.2 (US) ROM in baseroms/ntsc-1.2/.
#
# Usage: fishmod/build.sh [--debug] [--script NAME] [--f3dex3] [extra make args]
#   --debug        build the debug harness (boots straight into Kokiri, debug text)
#   --script NAME  (implies --debug) play fishmod/scripts/NAME.inc.c as autoplay input
#   --f3dex3       use the F3DEX3 graphics microcode (see fishmod/f3dex3); outputs get an -f3dex3 suffix
#   --opt N        performance test set N (1 or 2, see the Makefile); outputs get an -optN suffix
#   --testmode     new files skip the intro and start with testing items; outputs get a -test suffix
#   --fasttrig     polynomial sin/cos instead of the sine table; outputs get a -trig suffix
#   --hw           the original-hardware version (fewer, cheaper pixels for the RDP); outputs get a -hw suffix
#   --gcc LEVEL    the game's code (src/code, overlays, the mod) is compiled with GCC (boot, libultra, libc and audio stay
#                  on IDO). The default is O2 (hwbench-4: CPU time 14-16% lower, no other change); --gcc Os is the other
#                  level. Outputs get a -gccOs suffix for Os only
#   --ido          everything with IDO, as the decompilation builds; outputs get an -ido suffix
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
F3DEX3=0
OPT=0
TESTMODE=0
FASTTRIG=0
HW=0
GCC="O2"
MAKE_ARGS=()
while [ $# -gt 0 ]; do
    case "$1" in
        --debug) DEBUG=1 ;;
        --script) DEBUG=1; SCRIPT="$2"; shift ;;
        --f3dex3) F3DEX3=1 ;;
        --opt) OPT="$2"; shift ;;
        --testmode) TESTMODE=1 ;;
        --fasttrig) FASTTRIG=1 ;;
        --hw) HW=1 ;;
        --gcc) GCC="$2"; shift ;;
        --ido) GCC="" ;;
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
MODE="debug=$DEBUG f3dex3=$F3DEX3 opt=$OPT testmode=$TESTMODE fasttrig=$FASTTRIG hw=$HW gcc=$GCC"
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

# new art (fishmod/assets/assets.json) into C; the generated files are committed, so this is optional
if python3 -c "import PIL" 2>/dev/null; then
    python3 fishmod/tools/assets.py
else
    echo "note: Pillow isn't installed, so fishmod/assets isn't regenerated (the committed C is used)"
fi

# the habitat props: textures edited from vanilla ones, models, and where they go (generated, not committed)
FISHMOD_HW=$HW python3 fishmod/tools/gen_habitat.py > /dev/null
python3 fishmod/tools/assets.py --manifest src/fishmod_data/props_manifest.json > /dev/null
python3 fishmod/tools/gen_scatter.py --c ${FISHMOD_UNTHINNED:+--unthinned}  # (FISHMOD_UNTHINNED=1: the beds before thinning, for A/B)
# Lake Hylia's bed shaded under the water (generated, not committed)
python3 fishmod/tools/gen_bedshade.py > /dev/null
# the solid props' collision (rocks), added to the scene's at load (generated, not committed)
python3 fishmod/tools/gen_collision.py
# the water floor maps, baked from the extracted scenes and the props' collision (generated, not committed)
python3 fishmod/tools/gen_watermap.py

GCC_ARGS=()
if [ -n "$GCC" ]; then
    GCC_ARGS=(GCC_CODE=1 GCC_OPT="$GCC")
fi
make VERSION=$VERSION REGION=$REGION NON_MATCHING=1 ${GCC_ARGS[@]+"${GCC_ARGS[@]}"} FISHMOD_DEBUG=$DEBUG FISHMOD_SCRIPT="$SCRIPT" F3DEX3=$F3DEX3 FISHMOD_OPT=$OPT FISHMOD_TESTMODE=$TESTMODE FISHMOD_FASTTRIG=$FASTTRIG FISHMOD_HW=$HW \
    compress -j"$(nproc)" ${MAKE_ARGS[@]+"${MAKE_ARGS[@]}"}

mkdir -p "build/$VERSION"
echo "$MODE" > "$STAMP"
echo "$SCRIPT" > "$SCRIPT_STAMP"
SUFFIX=""
if [ "$F3DEX3" = 1 ]; then
    SUFFIX="-f3dex3"
fi
if [ "$OPT" != 0 ]; then
    SUFFIX="$SUFFIX-opt$OPT"
fi
if [ "$FASTTRIG" = 1 ]; then
    SUFFIX="$SUFFIX-trig"
fi
if [ "$TESTMODE" = 1 ]; then
    SUFFIX="$SUFFIX-test"
fi
if [ "$HW" = 1 ]; then
    SUFFIX="$SUFFIX-hw"
fi
if [ -z "$GCC" ]; then
    SUFFIX="$SUFFIX-ido"
elif [ "$GCC" != "O2" ]; then
    SUFFIX="$SUFFIX-gcc$GCC"
fi

if [ "$DEBUG" = 1 ]; then
    name="fish-oot-debug${SCRIPT:+-$SCRIPT}$SUFFIX"
    cp "build/$VERSION/oot-$VERSION-compressed.z64" "$OUT_DIR/$name.z64"
    echo "Built $OUT_DIR/$name.z64"
    exit 0
fi

# The patch is always made against a big-endian .z64, whatever the input byte order.
python3 fishmod/tools/to_z64.py "$baserom" "$OUT_DIR/original.z64"
name="fish-oot$SUFFIX"
cp "build/$VERSION/oot-$VERSION-compressed.z64" "$OUT_DIR/$name.z64"
rm -f "$OUT_DIR/$name.bps"
"$FLIPS" --create --bps "$OUT_DIR/original.z64" "$OUT_DIR/$name.z64" "$OUT_DIR/$name.bps" || true
test -f "$OUT_DIR/$name.bps"

echo "Built $OUT_DIR/$name.z64 and $OUT_DIR/$name.bps"
