#!/usr/bin/env bash
# bgcheck_check: the collision code as it is in the working tree against the same files at a git revision (default
# HEAD), on every scene of a version, every answer compared bit for bit (driver.c).
#
# Usage: tools/bgcheck_check/run.sh [--version VERSION] [--base REV] [--keep DIR] [driver args...]
#   VERSION: an extracted version (default ntsc-1.2; `make setup VERSION=...` first)
#   --pak-flag 0|1: the working tree's BGCHECK_EXPANSION_PAK (default 1: with --ram 8 the Pak's memory is used)
#   --extracted NAME: the scenes from extracted/NAME instead (e.g. a packed copy: ntsc-1.2-colpack)
#   REV: the revision to compare with (default HEAD)
#   driver args: --frames N --queries N --scene NAME --seed N
# Floats as the game's GCC builds compute them: no fused multiply-adds (the VR4300 has none), no reassociation, single
# precision throughout (SSE, not the x87's longer registers). 32-bit (-m32, gcc-multilib): long and pointers as the
# N64's (the game's u32 is an unsigned long).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$ROOT/tools/bgcheck_check"
VERSION=ntsc-1.2
EXTNAME=""
PAKFLAG=1
BASE=HEAD
OUT=""
ARGS=()
while [ $# -gt 0 ]; do
    case "$1" in
        --version) VERSION="$2"; shift ;;
        --base) BASE="$2"; shift ;;
        --extracted) EXTNAME="$2"; shift ;;
        --pak-flag) PAKFLAG="$2"; shift ;;
        --keep) OUT="$2"; shift ;;
        *) ARGS+=("$1") ;;
    esac
    shift
done
[ -n "$OUT" ] || OUT="$(mktemp -d)"
mkdir -p "$OUT/base/src/code" "$OUT/base/include" "$OUT/opt"
CFG="$ROOT/baseroms/$VERSION/config.yml"
EXT="$ROOT/extracted/${EXTNAME:-$VERSION}"
[ -d "$EXT" ] || { echo "no $EXT: run make setup VERSION=$VERSION"; exit 1; }

# the version's defines, as the Makefile makes them
VM=$(echo "$VERSION" | tr a-z-. A-Z__)
case "$VERSION" in
    ntsc-1.0) REV=0; PLAT=N64 ;; ntsc-1.1) REV=1; PLAT=N64 ;; ntsc-1.2) REV=2; PLAT=N64 ;;
    pal-1.0) REV=1; PLAT=N64 ;; pal-1.1) REV=2; PLAT=N64 ;;
    *) REV=15; PLAT=GC ;;
esac
case "$VERSION" in
    *-jp*|ntsc-*) REGION=JP ;; *-eu*|pal-*) REGION=EU ;; *) REGION=US ;;
esac
[ "$VERSION" = ntsc-1.2 ] && REGION=US
case "$VERSION" in *dbg*) DBG=1 ;; *) DBG=0 ;; esac
if [ "$PLAT" = N64 ]; then PDEF="-DPLATFORM_N64=1 -DPLATFORM_GC=0 -DPLATFORM_IQUE=0"; LU="-DLIBULTRA_VERSION=LIBULTRA_VERSION_I -DLIBULTRA_PATCH=1"
else PDEF="-DPLATFORM_N64=0 -DPLATFORM_GC=1 -DPLATFORM_IQUE=0"; LU="-DLIBULTRA_VERSION=LIBULTRA_VERSION_L -DLIBULTRA_PATCH=0"; fi
DEFS="-DCOMPILER_GCC -DNON_MATCHING -DAVOID_UB $PDEF -DOOT_VERSION=$VM -DOOT_REVISION=$REV -DOOT_REGION=REGION_$REGION $LU \
 -DDEBUG_FEATURES=$DBG -D_LANGUAGE_C -DF3DEX_GBI_2 -DNDEBUG"
CFLAGS="-m32 -msse2 -mfpmath=sse -O2 -fPIC -fno-stack-protector -ffp-contract=off -ffast-math -fno-unsafe-math-optimizations -fno-strict-aliasing \
 -funsigned-char -fno-builtin-sinf -fno-builtin-cosf -w"

# the revision's copies of the files that change
git -C "$ROOT" show "$BASE:src/code/z_bgcheck.c" > "$OUT/base/src/code/z_bgcheck.c"
git -C "$ROOT" show "$BASE:src/code/sys_math3d.c" > "$OUT/base/src/code/sys_math3d.c"

COMMON="src/code/z_lib.c src/code/z_skin_matrix.c src/code/sys_math_atan.c src/libc64/qrand.c src/libultra/gu/sins.c \
 src/libultra/gu/coss.c"
build_lib() { # name, dir with src/code/z_bgcheck.c and sys_math3d.c
    local name=$1 dir=$2 objs=()
    for f in "$dir/src/code/z_bgcheck.c" "$dir/src/code/sys_math3d.c"; do
        gcc -c $CFLAGS $DEFS -DBGCHECK_EXPANSION_PAK=$PAKFLAG -include "$HERE/tables_ram.h" -I"$HERE" -I"$ROOT/include" -I"$ROOT/include/libc" \
            -I"$ROOT/src" -I"$ROOT" -I"$EXT" "$f" -o "$OUT/$name-$(basename "$f" .c).o"
        objs+=("$OUT/$name-$(basename "$f" .c).o")
    done
    # (stubs*.c: what the rest of the game provides; a tree with more in its collision code adds a stubs_<name>.c)
    for f in $COMMON $(cd "$ROOT" && ls tools/bgcheck_check/stubs*.c); do
        gcc -c $CFLAGS $DEFS -I"$HERE" -I"$ROOT/include" -I"$ROOT/include/libc" -I"$ROOT/src" -I"$ROOT" -I"$EXT" \
            "$ROOT/$f" -o "$OUT/$name-$(basename "$f" .c).o"
        objs+=("$OUT/$name-$(basename "$f" .c).o")
    done
    gcc -m32 -shared -Wl,-Bsymbolic -o "$OUT/lib$name.so" "${objs[@]}" -lm
}
build_lib base "$OUT/base"
build_lib opt "$ROOT"

python3 "$HERE/gen_scenes.py" "$EXT" "$OUT/scenes.c"
# (the driver with the system's C library headers; the scenes with the game's, as the game builds them)
gcc -m32 -msse2 -mfpmath=sse -c -O1 -w $DEFS -I"$HERE" -I"$ROOT/include" -I"$ROOT/src" -I"$ROOT" -I"$EXT" "$HERE/driver.c" -o "$OUT/driver.o"
gcc -m32 -msse2 -mfpmath=sse -c -O1 -w $DEFS -I"$HERE" -I"$ROOT/include" -I"$ROOT/include/libc" -I"$ROOT/src" -I"$ROOT" -I"$EXT" \
    "$OUT/scenes.c" -o "$OUT/scenes.o"
EXTRA=()
if [ -f "$HERE/scene_extra.c" ]; then # (a tree's own changes to a scene's collision before it's tested, as its game makes)
    gcc -m32 -msse2 -mfpmath=sse -c -O1 -w $DEFS -I"$HERE" -I"$ROOT/include" -I"$ROOT/include/libc" -I"$ROOT/src" -I"$ROOT" \
        -I"$EXT" "$HERE/scene_extra.c" -o "$OUT/scene_extra.o"
    EXTRA+=("$OUT/scene_extra.o")
fi
gcc -m32 "$OUT/driver.o" "$OUT/scenes.o" "${EXTRA[@]}" -o "$OUT/driver" -ldl -lm
echo "$OUT/driver $OUT/libbase.so $OUT/libopt.so ${ARGS[*]:-}"
"$OUT/driver" "$OUT/libbase.so" "$OUT/libopt.so" "${ARGS[@]}"
