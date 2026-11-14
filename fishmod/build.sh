#!/usr/bin/env bash
# Builds the modded ROM and creates fish-oot.bps against the original
# NTSC 1.2 (US) ROM in baseroms/ntsc-1.2/.
#
# Usage: fishmod/build.sh [--debug] [--script NAME] [--f3dex3] [extra make args]
#   --debug        build the debug harness (boots straight into Kokiri, debug text)
#   --script NAME  (implies --debug) play fishmod/scripts/NAME.inc.c as autoplay input
#   --f3dex3       use the F3DEX3 graphics microcode (see fishmod/f3dex3) whatever the profile says; outputs get an
#                  -f3dex3 suffix
#   --opt N        ignored (the performance levels it chose are now the only game; kept so old commands still work)
#   --testmode     new files skip the intro and start with testing items; outputs get a -test suffix
#   --profile NAME the machine it's made for (fishmod/data/profiles.json): emu (the default: HLE emulators) or hw
#                  (the original hardware: F3DEX3, the RDP's limits); outputs get the profile's suffix (-hw)
#   --hw           the same as --profile hw
#   --gcc LEVEL    the game's code (src/code, overlays, the mod) is compiled with GCC (boot, libultra, libc and audio stay
#                  on IDO). The default is O2 (hwbench-4: CPU time 14-16% lower, no other change); --gcc Os is the other
#                  level. Outputs get a -gccOs suffix for Os only
#   --ido          everything with IDO, as the decompilation builds; outputs get an -ido suffix
#   --clang        the mod's own code (src/code/fishmod*.c, the fish actor) with clang (LLVM) instead of GCC; outputs get
#                  a -clang suffix. CLANG_EXTRA (in the environment) passes it more options
#   --clang-game   (implies --clang) the whole game's C code with clang (src/code and every overlay), not just the
#                  mod's; outputs get a -clanggame suffix
#   --clang-audio  the audio driver alone on clang (IDO otherwise; --clang-game includes it): to measure it, or prove it
#                  sounds the same (the audlog script); outputs get a -clangaudio suffix
#   --gcc-audio    the audio driver on GCC too (IDO otherwise); --gcc-libultra libultra's C (os, io, gu); --gcc-all both:
#                  the whole game's C on GCC. Outputs get -gccaudio / -gccultra suffixes
#   --clang-opt L  (implies --clang) clang's optimisation level for the mod's code (Os, Oz, O3...; default the GCC
#                  level, O2); outputs get it in the suffix (-clangOs)
#   --texpack      lossless CI8 textures and leaner display lists for the RDP (tools/texpack.py): the build reads a
#                  converted copy of the extracted assets (extracted/VERSION-texpack); outputs get a -texpack suffix
#   --colpack      fewer collision triangles, the same surfaces (tools/colpack.py): the build reads a merged copy of the
#                  extracted assets (after --texpack's, if both); outputs get a -colpack suffix
#   --meshpack     the lake's widest mesh pieces cut into areas, each culled by the RSP on its own (tools/meshpack.py):
#                  the build reads a copy (after the other packs); outputs get a -meshpack suffix
#   --pgo          (implies --clang) clang optimises with the console's sample profile, PGO_PROFILE (default
#                  n64prof/profiles/oot-console2.prof; see its README); outputs get a -clang-pgo suffix
# The release patch: fishmod/tools/bps.py (Flips' format; any BPS patcher applies it), or FLIPS=/path/to/flips. Flips'
# own patches of these ROMs came out several times larger (30 MB against 4.7 MB: it misses the moved files).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION=ntsc-1.2
REGION=US
BASEROM_DIR="baseroms/$VERSION"
OUT_DIR="fishmod/out"
FLIPS="${FLIPS:-fishmod/tools/bps.py}"

DEBUG=0
CLANG=0
CLANG_OPT=""
CLANG_GAME=0
CLANG_AUDIO=0
GCC_AUDIO=0
GCC_LIBULTRA=0
TEXPACK=0
COLPACK=0
MESHPACK=0
PGO=0
SCRIPT=""
F3DEX3=0
TESTMODE=0
PROFILE=emu
GCC="O2"
MAKE_ARGS=()
while [ $# -gt 0 ]; do
    case "$1" in
        --debug) DEBUG=1 ;;
        --script) DEBUG=1; SCRIPT="$2"; shift ;;
        --f3dex3) F3DEX3=1 ;;
        --opt) shift ;; # (ignored)
        --testmode) TESTMODE=1 ;;
        --hw) PROFILE=hw ;;
        --profile) PROFILE="$2"; shift ;;
        --gcc) GCC="$2"; shift ;;
        --ido) GCC="" ;;
        --clang) CLANG=1 ;;
        --pgo) CLANG=1; PGO=1 ;;
        --clang-game) CLANG=1; CLANG_GAME=1 ;;
        --clang-audio) CLANG_AUDIO=1 ;;
        --gcc-audio) GCC_AUDIO=1 ;;
        --gcc-libultra) GCC_LIBULTRA=1 ;;
        --gcc-all) GCC_AUDIO=1; GCC_LIBULTRA=1 ;;
        --texpack) TEXPACK=1 ;;
        --colpack) COLPACK=1 ;;
        --meshpack) MESHPACK=1 ;;
        --clang-opt) CLANG=1; CLANG_OPT="$2"; shift ;;
        *) MAKE_ARGS+=("$1") ;;
    esac
    shift
done

if ! command -v "$FLIPS" >/dev/null 2>&1; then
    # no Flips: the repository's own BPS tool (the same patch format, a little larger)
    FLIPS="fishmod/tools/bps.py"
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

# the profile: its C header (rewritten only when it changes), and the settings it brings
python3 fishmod/tools/build_profile.py "$PROFILE" > /dev/null
export FISHMOD_PROFILE="$PROFILE"
if [ "$(python3 fishmod/tools/build_profile.py "$PROFILE" --get f3dex3)" = 1 ]; then
    F3DEX3=1
fi
PROFILE_SUFFIX="$(python3 fishmod/tools/build_profile.py "$PROFILE" --get suffix)"
# F3DEX3's variant: with the occlusion plane (BrW) or without it (BrW_NOC, a little faster where none is used)
F3DEX3_OCC="$(python3 fishmod/tools/build_profile.py "$PROFILE" --get f3dex3_occlusion)"
HW=0
if [ "$PROFILE" = hw ]; then
    HW=1
fi

# make doesn't track flag changes, so rebuild from clean whenever the build mode changes
# (including the first build after a matching build).
STAMP="build/$VERSION/.fishmod"
# (after the -O the Makefile gives, so it wins)
export CLANG_OPT  # (tools/fishmod_clang_cc.sh adds it after the Makefile's -O; libultra keeps -O2)
if [ "$PGO" = 1 ]; then
    PGO_PROFILE="$(realpath "${PGO_PROFILE:-n64prof/profiles/oot-console2.prof}")"
    export CLANG_EXTRA="-fprofile-sample-use=$PGO_PROFILE ${CLANG_EXTRA:-}"
fi
# (a clang build also rebuilds when its compiler options change: the wrapper's and the profile's contents count)
CLANG_SUM=""; [ "$CLANG" = 1 ] && CLANG_SUM="$(md5sum < tools/fishmod_clang_cc.sh | cut -c1-8)"
[ "$PGO" = 1 ] && CLANG_SUM="$CLANG_SUM-$(md5sum < "$PGO_PROFILE" | cut -c1-8)"
TEXPACK_ARGS=()
if [ "$TEXPACK" = 1 ]; then
    TP_SRC="extracted/$VERSION"
    TP_OUT="extracted/$VERSION-texpack"
    TP_SUM="$(md5sum < tools/texpack.py | cut -c1-8)"
    if [ "$(cat "$TP_OUT/.texpack" 2>/dev/null)" != "$TP_SUM" ]; then
        python3 tools/texpack.py "$TP_SRC" "$TP_OUT"
        echo "$TP_SUM" > "$TP_OUT/.texpack"
    fi
    TEXPACK_ARGS=(EXTRACTED_DIR="$TP_OUT")
fi
if [ "$COLPACK" = 1 ]; then
    CP_SRC="extracted/$VERSION"
    [ "$TEXPACK" = 1 ] && CP_SRC="$TP_OUT"
    CP_OUT="$CP_SRC-colpack"
    CP_SUM="$({ cat tools/colpack.py; cat "$CP_SRC/.texpack" 2>/dev/null || true; } | md5sum | cut -c1-8)"
    if [ "$(cat "$CP_OUT/.colpack" 2>/dev/null)" != "$CP_SUM" ]; then
        python3 tools/colpack.py "$CP_SRC" "$CP_OUT"
        echo "$CP_SUM" > "$CP_OUT/.colpack"
    fi
    TEXPACK_ARGS=(EXTRACTED_DIR="$CP_OUT")
fi
if [ "$MESHPACK" = 1 ]; then
    MP_SRC="extracted/$VERSION"
    [ "$TEXPACK" = 1 ] && MP_SRC="$TP_OUT"
    [ "$COLPACK" = 1 ] && MP_SRC="$CP_OUT"
    MP_OUT="$MP_SRC-meshpack"
    MP_SUM="$({ cat tools/meshpack.py; cat "$MP_SRC/.texpack" "$MP_SRC/.colpack" 2>/dev/null || true; } | md5sum | cut -c1-8)"
    if [ "$(cat "$MP_OUT/.meshpack" 2>/dev/null)" != "$MP_SUM" ]; then
        python3 tools/meshpack.py "$MP_SRC" "$MP_OUT" --report
        echo "$MP_SUM" > "$MP_OUT/.meshpack"
    fi
    TEXPACK_ARGS=(EXTRACTED_DIR="$MP_OUT")
fi
MODE="texpack=$TEXPACK${TP_SUM:+-$TP_SUM} colpack=$COLPACK${CP_SUM:+-$CP_SUM} meshpack=$MESHPACK${MP_SUM:+-$MP_SUM} debug=$DEBUG f3dex3=$F3DEX3 occ=$F3DEX3_OCC prof=${F3DEX3_PROF:-} testmode=$TESTMODE profile=$PROFILE gcc=$GCC clang=$CLANG$CLANG_SUM game=$CLANG_GAME audio=$CLANG_AUDIO$([ "$CLANG_AUDIO" = 1 ] && md5sum < tools/fishmod_clang_cc.sh | cut -c1-8) gccaudio=$GCC_AUDIO gccultra=$GCC_LIBULTRA$(md5sum < tools/fishmod_gcc_cc.sh | cut -c1-8) opt=$CLANG_OPT ${CLANG_EXTRA:-}"
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

# art made from the game's own (generated, not committed): the species' textures, the bait bottles' icons
python3 -W ignore::DeprecationWarning fishmod/tools/gen_textures.py > /dev/null
python3 fishmod/tools/gen_bait_icons.py
python3 fishmod/tools/gen_dpad_tex.py >/dev/null
python3 fishmod/tools/gen_clock_tex.py >/dev/null
python3 fishmod/tools/assets.py --manifest src/fishmod_data/bait_manifest.json > /dev/null

# the habitat props: textures edited from vanilla ones, models, and where they go (generated, not committed)
FISHMOD_HW=$HW python3 fishmod/tools/gen_habitat.py > /dev/null
python3 fishmod/tools/assets.py --manifest src/fishmod_data/props_manifest.json > /dev/null
python3 fishmod/tools/gen_scatter.py --c ${FISHMOD_UNTHINNED:+--unthinned}  # (FISHMOD_UNTHINNED=1: the beds before thinning, for A/B)
# the bed cards: each chunk's distant plants as one card, from the scatter and the plants' meshes (not committed)
FISHMOD_HW=$HW python3 fishmod/tools/bedcard.py > /dev/null
# Lake Hylia's bed shaded under the water (generated, not committed)
python3 fishmod/tools/gen_bedshade.py > /dev/null
# the solid props' collision (rocks), added to the scene's at load (generated, not committed)
python3 fishmod/tools/gen_collision.py
# occluder candidates for the occlusion plane, from the collision (generated, not committed)
python3 fishmod/tools/gen_occluders.py > /dev/null
# the item page's graphics, made from the game's own (generated, not committed)
python3 fishmod/tools/gen_item_page.py
# the water floor maps, baked from the extracted scenes and the props' collision (generated, not committed)
python3 fishmod/tools/gen_watermap.py
# the animation check's case table: every one of Link's animations (generated, not committed)
python3 fishmod/tools/gen_animcheck_cases.py > /dev/null

GCC_ARGS=()
if [ -n "$GCC" ]; then
    GCC_ARGS=(GCC_CODE=1 GCC_OPT="$GCC" CLANG_AUDIO=$CLANG_AUDIO GCC_AUDIO=$GCC_AUDIO GCC_LIBULTRA=$GCC_LIBULTRA)
    if [ "$CLANG" = 1 ]; then
        GCC_ARGS+=(CLANG_MOD=1 CLANG_GAME=$CLANG_GAME)
    fi
elif [ "$CLANG" = 1 ]; then
    echo "error: --clang goes with the GCC build (not --ido)" >&2
    exit 1
fi
make VERSION=$VERSION REGION=$REGION NON_MATCHING=1 ${GCC_ARGS[@]+"${GCC_ARGS[@]}"} ${TEXPACK_ARGS[@]+"${TEXPACK_ARGS[@]}"} FISHMOD_DEBUG=$DEBUG FISHMOD_SCRIPT="$SCRIPT" F3DEX3=$F3DEX3 F3DEX3_OCC=$F3DEX3_OCC F3DEX3_PROF=${F3DEX3_PROF:-} FISHMOD_TESTMODE=$TESTMODE \
    compress -j"$(nproc)" ${MAKE_ARGS[@]+"${MAKE_ARGS[@]}"}

mkdir -p "build/$VERSION"
echo "$MODE" > "$STAMP"
echo "$SCRIPT" > "$SCRIPT_STAMP"
SUFFIX=""
if [ "$F3DEX3" = 1 ]; then
    SUFFIX="-f3dex3"
fi
if [ "$TESTMODE" = 1 ]; then
    SUFFIX="$SUFFIX-test"
fi
if [ -n "${F3DEX3_PROF:-}" ]; then
    SUFFIX="$SUFFIX-prof$F3DEX3_PROF"
fi
SUFFIX="$SUFFIX$PROFILE_SUFFIX"
if [ -z "$GCC" ]; then
    SUFFIX="$SUFFIX-ido"
elif [ "$GCC" != "O2" ]; then
    SUFFIX="$SUFFIX-gcc$GCC"
fi
if [ "$CLANG" = 1 ]; then
    SUFFIX="$SUFFIX-clang$([ "$CLANG_GAME" = 1 ] && echo game)$CLANG_OPT"
fi
if [ "$PGO" = 1 ]; then
    SUFFIX="$SUFFIX-pgo"
fi
if [ "$TEXPACK" = 1 ]; then
    SUFFIX="$SUFFIX-texpack"
fi
if [ "$COLPACK" = 1 ]; then
    SUFFIX="$SUFFIX-colpack"
fi
if [ "$MESHPACK" = 1 ]; then
    SUFFIX="$SUFFIX-meshpack"
fi
if [ "$CLANG_AUDIO" = 1 ]; then
    SUFFIX="$SUFFIX-clangaudio"
fi
if [ "$GCC_AUDIO" = 1 ]; then
    SUFFIX="$SUFFIX-gccaudio"
fi
if [ "$GCC_LIBULTRA" = 1 ]; then
    SUFFIX="$SUFFIX-gccultra"
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
