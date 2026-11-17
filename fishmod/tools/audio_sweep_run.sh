#!/usr/bin/env bash
# Plays one part of the audio sweep (fishmod/docs/audio-sweep.md; a sweep_* script's ROM) in ares to its end and
# saves what the sweep's checks read:
#   OUT/ram.bin        the console's memory at the end (the sound log, the frame count, every thread's state)
#   OUT/cpu.bin        the CPU profile of the whole run, and OUT/cpu.bin.segNNNN.LEG its totals at each change of
#                      leg (cpuprof.py; audio_sweep_report.py takes the differences)
#   OUT/rsp.bin        the RSP profile of the audio tasks
# The run ends when the game says the sweep is over (the cut named 65535) or after MAXSECS (default 3600), which
# is then a failure the report shows (the game stopped or hung).
# Usage: fishmod/tools/audio_sweep_run.sh SLOT ROM ELF OUT [MAXSECS]
set -u
SLOT=$1 ROM=$2 ELF=$3 OUT=$4 MAXSECS=${5:-3600}
ARES=${ARES:-/opt/src/ares/build/rundir/bin/ares}
HERE="$(cd "$(dirname "$0")" && pwd)"
PORT=$((${SWEEP_PORT_BASE:-9123} + SLOT))  # (SWEEP_PORT_BASE: another base when two sweeps share a machine)
mkdir -p "$OUT"; rm -f "$OUT"/cpu.bin* "$OUT"/rsp.bin* "$OUT"/ram.bin
CONF=$(mktemp --suffix=.bml)
sed "s/^\(\s*Port:\s*\)9123/\1$PORT/" "$HERE/ares_debug.bml" > "$CONF"
sed -i -e '/^Video$/,/^[A-Z]/ s/^  Driver: .*/  Driver: None/' -e '/^Audio$/,/^[A-Z]/ s/^  Driver: .*/  Driver: None/' \
    -e '/^Audio$/,/^[A-Z]/ s/^  Blocking: .*/  Blocking: false/' "$CONF"
xdpyinfo >/dev/null 2>&1 || { (Xvfb "${DISPLAY:-:190}" -screen 0 1024x768x24 >/dev/null 2>&1 &); sleep 2; }
# (ELF: the build's ELF, or its symbols file from audio_sweep_report.py --syms)
case "$ELF" in
    *.sym.gz) MARK=$(zcat "$ELF" | awk '$3=="gFishmodSweepLeg"{print $2}') ;;
    *.sym) MARK=$(awk '$3=="gFishmodSweepLeg"{print $2}' "$ELF") ;;
    *) MARK=$(mips-linux-gnu-nm "$ELF" | awk '$3=="gFishmodSweepLeg"{print $1}') ;;
esac
[ -n "$MARK" ] || { echo "no gFishmodSweepLeg in $ELF"; exit 1; }
ARES_CPUPROF="$OUT/cpu.bin" ARES_CPUPROF_MARK=$MARK ARES_RSPPROF="$OUT/rsp.bin" ARES_RSPPROF_KEY=00400008 \
    timeout $((MAXSECS + 120)) "$ARES" --settings-file "$CONF" --system "Nintendo 64" --no-file-prompt "$ROM" >/dev/null 2>&1 &
APID=$!
T=0
while [ $T -lt "$MAXSECS" ] && kill -0 $APID 2>/dev/null; do
    ls "$OUT"/cpu.bin.seg*.65535 >/dev/null 2>&1 && break
    sleep 5; T=$((T + 5))
done
gdb-multiarch -q -batch -ex "set architecture mips:4000" -ex "set endian big" -ex "target remote 127.0.0.1:$PORT" \
    -ex "dump binary memory $OUT/ram.bin 0x80000000 0x80800000" >/dev/null 2>&1
kill $APID 2>/dev/null; wait $APID 2>/dev/null
rm -f "$CONF"
echo "$OUT: ${T}s, $(ls "$OUT"/cpu.bin.seg* 2>/dev/null | wc -l) cuts, end $(ls "$OUT"/cpu.bin.seg*.65535 >/dev/null 2>&1 && echo reached || echo NOT-REACHED)"
