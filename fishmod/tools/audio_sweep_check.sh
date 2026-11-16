#!/usr/bin/env bash
# The audio sweep's verdict between two builds (fishmod/tools/audio_sweep.sh's OUT folders): for every run both
# have, the same sound (audlog_compare.py), both still running at the end, and the cost leg by leg, the legs where
# the new build is slower (the audio, or the rest of the game) named. Then what of the audio driver no run reached.
#   fishmod/tools/audio_sweep_check.sh REF_OUT NEW_OUT
set -u
REF=$1 NEW=$2
HERE="$(cd "$(dirname "$0")" && pwd)"
fail=0
for d in "$NEW"/[sc]?.*/; do
    r=$(basename "$d")
    [ -f "$REF/$r/report.json" ] && [ -f "$d/report.json" ] || { echo "$r: MISSING"; fail=1; continue; }
    snd=$(python3 "$HERE/audlog_compare.py" "$REF/$r/aud.bin" "$d/aud.bin" 2>&1 | tail -1)
    cmp=$(python3 "$HERE/audio_sweep_report.py" --compare "$REF/$r" "$d")
    echo "== $r: sound $snd; $(echo "$cmp" | tail -1)"
    echo "$cmp" | grep -E "SLOWER|DIFFER|NOT REACHED|STOPPED|^all "
    case "$snd" in *SAME*) ;; *) fail=1 ;; esac
    echo "$cmp" | grep -qE "DIFFER|NOT REACHED|STOPPED" && fail=1
done
python3 "$HERE/audio_sweep_report.py" --cover "$NEW/rom.elf" "$NEW"/[sc]?.*/ | head -${COVER_LINES:-60}
[ $fail = 0 ] && echo "SWEEP: sound the same and running to the end in every run" || echo "SWEEP: FAILED (see above)"
