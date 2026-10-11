#!/usr/bin/env bash
# Builds a debug variant, runs it in ares for a while and prints its timing stats (fishmod/tools/read_stats.py).
#
# Usage: ARES=/path/to/ares fishmod/tools/bench.sh <label> <seconds> <build.sh args...>
#   e.g. fishmod/tools/bench.sh opt1 180 --script shore --opt 1
# Results also go to fishmod/out/bench/<label>.txt.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
LABEL="$1"; SECS="$2"; shift 2
fishmod/build.sh "$@" > "fishmod/out/bench-build-$LABEL.log" 2>&1 || { tail -20 "fishmod/out/bench-build-$LABEL.log"; exit 1; }
rom=$(grep -o "fishmod/out/[^ ]*\.z64" "fishmod/out/bench-build-$LABEL.log" | tail -1)
mkdir -p fishmod/out/bench
work=$(mktemp -d)
cp "$rom" "$work/$LABEL.z64"
fishmod/tools/ares_capture.sh "$work/$LABEL.z64" "$work/shots" "$SECS" "$SECS" > /dev/null 2>&1 || true
python3 fishmod/tools/read_stats.py "$work/$LABEL.ram" | tee "fishmod/out/bench/$LABEL.txt"
# the last screenshot, to look for broken graphics
last=$(ls "$work/shots"/shot_*.png 2>/dev/null | tail -1 || true)
[ -n "$last" ] && cp "$last" "fishmod/out/bench/$LABEL.png"
rm -rf "$work"
