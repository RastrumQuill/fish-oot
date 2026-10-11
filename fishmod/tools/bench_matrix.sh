#!/usr/bin/env bash
# Builds every benchmark scene with every optimization variant, runs them in ares (a few at a time, each on its own
# virtual display) and writes a table of the timing stats to fishmod/out/bench/matrix.md, plus a screenshot of each
# run (fishmod/out/bench/<scene>-<variant>.png) to look for broken graphics.
#
# Usage: ARES=/path/to/ares fishmod/tools/bench_matrix.sh [seconds per run] [parallel runs]
# Emulated time doesn't depend on how fast the host is, so runs in parallel give the same numbers, only fewer frames.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
SECS="${1:-200}"
JOBS="${2:-3}"
SCENES="${SCENES:-bench_lab bench_kokiri bench_lake}"
VARIANTS="${VARIANTS:-opt0 opt1 opt2 opt3 opt3-trig opt3-f3dex3}"
OUT=fishmod/out/bench
mkdir -p "$OUT/roms"

args_for() {
    case "$1" in
        opt3-trig) echo "--opt 3 --fasttrig" ;;
        opt3-f3dex3) echo "--opt 3 --f3dex3" ;;
        opt*) echo "--opt ${1#opt}" ;;
    esac
}

# build (one at a time: they share the build directory)
for scene in $SCENES; do
    for v in $VARIANTS; do
        label="${scene#bench_}-$v"
        fishmod/build.sh --script "$scene" $(args_for "$v") > "$OUT/build-$label.log" 2>&1 ||
            { echo "build failed: $label"; tail -20 "$OUT/build-$label.log"; exit 1; }
        cp "$(grep -o "fishmod/out/[^ ]*\.z64" "$OUT/build-$label.log" | tail -1)" "$OUT/roms/$label.z64"
        echo "built $label"
    done
done

run_one() {
    local label="$1" disp="$2" work
    work=$(mktemp -d)
    cp "$OUT/roms/$label.z64" "$work/$label.z64"
    DISP="$disp" fishmod/tools/ares_capture.sh "$work/$label.z64" "$work/shots" "$SECS" "$SECS" > /dev/null 2>&1 || true
    python3 fishmod/tools/read_stats.py "$work/$label.ram" > "$OUT/$label.txt" 2>&1 || true
    last=$(ls "$work/shots"/shot_*.png 2>/dev/null | tail -1 || true)
    [ -n "$last" ] && cp "$last" "$OUT/$label.png"
    rm -rf "$work"
    echo "ran $label"
}

n=0
for scene in $SCENES; do
    for v in $VARIANTS; do
        run_one "${scene#bench_}-$v" ":$((100 + n % JOBS))" &
        n=$((n + 1))
        if [ $((n % JOBS)) -eq 0 ]; then wait; fi
    done
done
wait

python3 fishmod/tools/read_stats.py --table $(for s in $SCENES; do for v in $VARIANTS; do echo "$OUT/${s#bench_}-$v.txt"; done; done) \
    > "$OUT/matrix.md"
cat "$OUT/matrix.md"
