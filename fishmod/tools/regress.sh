#!/usr/bin/env bash
# Regression runs for refactoring: builds a fixed set of scripted scenarios, runs each in ares long enough for all
# its state fingerprints (fishmod_debug.c, StateHash_Update), and keeps their SRAM and a few screenshots.
# Compare two labels with fishmod/tools/state_hash.py <dir>/<label>/<script>/r.ram ...
#
# Usage: fishmod/tools/regress.sh <outdir> <label> [scripts...]
#   Needs ARES (the ares binary). Builds with the test build's settings (--hw --f3dex3).
set -euo pipefail
OUT="$1"; LABEL="$2"; shift 2
SCRIPTS=("$@")
[ ${#SCRIPTS[@]} -eq 0 ] && SCRIPTS=(bait lakecast invtest)
cd "$(dirname "$0")/../.."
for s in "${SCRIPTS[@]}"; do
    d="$OUT/$LABEL/$s"; mkdir -p "$d"
    fishmod/build.sh --script "$s" --hw --f3dex3 > "$d/build.log" 2>&1 || { echo "$s: build failed"; exit 1; }
    cp "fishmod/out/fish-oot-debug-$s-f3dex3-hw.z64" "$d/r.z64"
done
git checkout -q -- fishmod/assets/habitat 2>/dev/null || true  # (the build regenerates these per profile)
n=170
for s in "${SCRIPTS[@]}"; do
    d="$OUT/$LABEL/$s"; rm -rf "$d/shots" "$d/r.ram"; n=$((n + 1))
    (DISP=":$n" timeout 900 fishmod/tools/ares_capture.sh "$d/r.z64" "$d/shots" 400 60 > /dev/null 2>&1 || true) &
done
wait
for s in "${SCRIPTS[@]}"; do echo "== $s"; python3 fishmod/tools/state_hash.py "$OUT/$LABEL/$s/r.ram" | tail -2; done
