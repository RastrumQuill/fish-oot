#!/usr/bin/env bash
# Runs a collog ROM (fishmod/scripts/collog.inc.c) in ares for a while, then saves its collision log (one record a
# frame: the hash of every collision question and answer, and the calls of each kind) from the emulator's memory
# through ares' GDB server. Compare two with fishmod/tools/collog_compare.py.
#
# Usage: fishmod/tools/collog.sh ROM ELF SECONDS OUT.bin
# Needs: ares built by fishmod/tools/build_ares.sh (ARES=...), gdb-multiarch, an X display (Xvfb) in DISPLAY.
set -euo pipefail
ROM=$1 ELF=$2 SECS=$3 OUT=$4
ARES=${ARES:-/opt/src/ares/build/rundir/bin/ares}
HERE="$(cd "$(dirname "$0")" && pwd)"
sym() { mips-linux-gnu-nm "$ELF" | awk -v n="$1" '$3==n{print $1}'; }
PTR=$(sym gFishmodColLog) CNT=$(sym gFishmodColLogFrames)
[ -n "$PTR" ] && [ -n "$CNT" ] || { echo "no collision log in $ELF (build with --script collog)"; exit 1; }
timeout $((SECS + 60)) "$ARES" --settings-file "$HERE/ares_debug.bml" --system "Nintendo 64" --no-file-prompt "$ROM" \
    >/dev/null 2>&1 &
APID=$!
sleep "$SECS"
gdb-multiarch -q -batch -ex "set architecture mips:4000" -ex "set endian big" -ex "target remote 127.0.0.1:9123" \
    -ex "set \$p = *(unsigned int*)0x$PTR" -ex "set \$n = *(unsigned int*)0x$CNT" \
    -ex "printf \"frames %u\\n\", \$n" \
    -ex "dump binary memory $OUT \$p \$p + \$n * 32" 2>&1 | grep -E "^frames|rror" || true
kill $APID 2>/dev/null || true
wait $APID 2>/dev/null || true
ls -l "$OUT"
