#!/usr/bin/env bash
# The audio sweep's scenes leg for one build (fishmod/docs/audio-sweep.md): plays every variant of a sweep_scenes
# ROM in ares, four at a time, and writes each one's report.
#   fishmod/tools/audio_sweep.sh ROM ELF OUT [PARTS] [VARIANTS...]
# A variant is SOUND+AGE: s0 s1 s2 s3 (stereo, mono, headphones, surround; adult Link by day) and c0..c3 (the same
# with child Link at night). Default: all eight. PARTS (default 4): how many runs the scenes are dealt into.
# Each run is OUT/<variant>.<part>/ (audio_sweep_run.sh's files and audio_sweep_report.py's report.json, aud.bin).
# Then, between two builds' OUT folders: fishmod/tools/audio_sweep_check.sh REF_OUT NEW_OUT
set -u
ROM=$1 ELF=$2 OUT=$3 PARTS=${4:-4}
shift 3; [ $# -gt 0 ] && shift
VARIANTS=${*:-s0 s1 s2 s3 c0 c1 c2 c3}
HERE="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$OUT"
cp "$ELF" "$OUT/rom.elf"
JOBS=()
for v in $VARIANTS; do
    sound=${v:1:1}; child=0; [ "${v:0:1}" = c ] && child=1
    for ((k = 0; k < PARTS; k++)); do
        d="$OUT/$v.$k"
        mkdir -p "$d"
        # the options word in the ROM's header (0x38): bits 0-1 sound, bit 4 child at night, 8-15 parts, 16-23 part
        python3 - "$ROM" "$d/rom.z64" $((sound | child << 4 | PARTS << 8 | k << 16)) <<'PY'
import struct, sys
d = bytearray(open(sys.argv[1], 'rb').read())
struct.pack_into('>I', d, 0x38, int(sys.argv[3]))
open(sys.argv[2], 'wb').write(d)
PY
        JOBS+=("$d")
    done
done
export HERE OUT
printf '%s\n' "${JOBS[@]}" | xargs -P 4 -I{} bash -c '
    d={}; slot=$(( $(echo "$d" | cksum | cut -d" " -f1) % 4 ))
    # (a free GDB port: four runs at once, one each)
    for s in 0 1 2 3; do if mkdir "$OUT/.slot$s" 2>/dev/null; then slot=$s; break; fi; done
    "$HERE/audio_sweep_run.sh" $slot "$d/rom.z64" "$OUT/rom.elf" "$d" ${MAXSECS:-2400} > "$d/run.log" 2>&1
    rmdir "$OUT/.slot$slot" 2>/dev/null
    python3 "$HERE/audio_sweep_report.py" "$d" --elf "$OUT/rom.elf" > "$d/report.txt" 2>&1
    rm -f "$d"/cpu.bin.seg* "$d"/cpu.bin.lines "$d/rom.z64"
    head -1 "$d/report.txt"; grep STOPPED "$d/report.txt"
'
rmdir "$OUT"/.slot* 2>/dev/null
