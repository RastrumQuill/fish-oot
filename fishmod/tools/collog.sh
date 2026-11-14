#!/usr/bin/env bash
# Runs a collog ROM (fishmod/scripts/collog.inc.c) in ares for a while, then saves its collision log (one record a
# frame: the hash of every collision question and answer, and the calls of each kind) from the emulator's memory
# through ares' GDB server. Compare two with fishmod/tools/collog_compare.py.
#
# Usage: fishmod/tools/collog.sh [--audio] ROM ELF SECONDS OUT.bin [GFX_OUT.bin]
#   --audio: the audio check's log instead (fishmod/scripts/audlog.inc.c; compare with audlog_compare.py)
#   --anim: the animation check's log (fishmod/scripts/animcheck.inc.c; compare with animcheck_compare.py)
#   GFX_OUT.bin: also the graphics log (graph.c: each frame's display lists and their allocations hashed, one word a
#   frame): two builds that draw exactly the same give the same file (cmp, or collog_compare.py --gfx)
# Needs: ares built by fishmod/tools/build_ares.sh (ARES=...), gdb-multiarch, an X display (Xvfb) in DISPLAY.
# ARES_SLOT=0-3 (default 0): which GDB port (9123 + slot), so several runs can go at once.
set -euo pipefail
REC=32 PTRSYM=gFishmodColLog CNTSYM=gFishmodColLogFrames
if [ "${1:-}" = "--audio" ]; then
    REC=16 PTRSYM=gFishmodAudLog CNTSYM=gFishmodAudLogCount
    shift
fi
if [ "${1:-}" = "--anim" ]; then
    # the animation check's log (fishmod/scripts/animcheck.inc.c; compare with animcheck_compare.py)
    REC=32 PTRSYM=gFishmodAnimLog CNTSYM=gFishmodAnimLogCount
    shift
fi
ROM=$1 ELF=$2 SECS=$3 OUT=$4 GFXOUT=${5:-}
ARES=${ARES:-/opt/src/ares/build/rundir/bin/ares}
HERE="$(cd "$(dirname "$0")" && pwd)"
sym() { mips-linux-gnu-nm "$ELF" | awk -v n="$1" '$3==n{print $1}'; }
PTR=$(sym $PTRSYM) CNT=$(sym $CNTSYM)
[ -n "$PTR" ] && [ -n "$CNT" ] || { echo "no $PTRSYM in $ELF (build with --script collog or audlog)"; exit 1; }
xdpyinfo >/dev/null 2>&1 || { echo "no X display in DISPLAY ($DISPLAY): start Xvfb"; exit 1; }
PORT=$((9123 + ${ARES_SLOT:-0}))
CONF=$(mktemp --suffix=.bml)
sed "s/^\(\s*Port:\s*\)9123/\1$PORT/" "$HERE/ares_debug.bml" > "$CONF"
# ARES_FAST=1: no video or audio output (as fishmod/tools/ares_run.sh): about twice as fast, the same emulation
if [ "${ARES_FAST:-0}" = 1 ]; then
    sed -i -e '/^Video$/,/^[A-Z]/ s/^  Driver: .*/  Driver: None/' -e '/^Audio$/,/^[A-Z]/ s/^  Driver: .*/  Driver: None/' \
        -e '/^Audio$/,/^[A-Z]/ s/^  Blocking: .*/  Blocking: false/' "$CONF"
fi
timeout $((SECS + 60)) "$ARES" --settings-file "$CONF" --system "Nintendo 64" --no-file-prompt "$ROM" \
    >/dev/null 2>&1 &
APID=$!
GFXARGS=()
if [ -n "$GFXOUT" ]; then
    GFXARGS=(-ex "set \$g = *(unsigned int*)0x$(sym gFishmodGfxLog)" -ex "set \$m = *(unsigned int*)0x$(sym gFishmodGfxLogCount)"
             -ex "printf \"gfx frames %u\\n\", \$m" -ex "dump binary memory $GFXOUT \$g \$g + \$m * 36"
             -ex "set \$q = *(unsigned int*)0x$(sym gFishmodGfxDump)" -ex "dump binary memory $GFXOUT.pool \$q \$q + 0x12410")
fi
sleep "$SECS"
gdb-multiarch -q -batch -ex "set architecture mips:4000" -ex "set endian big" -ex "target remote 127.0.0.1:$PORT" \
    -ex "set \$p = *(unsigned int*)0x$PTR" -ex "set \$n = *(unsigned int*)0x$CNT" \
    -ex "printf \"records %u\\n\", \$n" \
    -ex "dump binary memory $OUT \$p \$p + \$n * $REC" \
    "${GFXARGS[@]}" \
    2>&1 | grep -E "^records|^gfx|rror" || true
kill $APID 2>/dev/null || true
wait $APID 2>/dev/null || true
rm -f "$CONF"
ls -l "$OUT"
