#!/usr/bin/env bash
# Runs ROMs already built by regress.sh again (no build): <outdir>/<from>/<script>/r.z64 -> <outdir>/<to>/<script>/.
# Usage: fishmod/tools/regress_run.sh <outdir> <from> <to> [seconds] [scripts...]   (checks the runs are repeatable)
set -euo pipefail
OUT="$1"; FROM="$2"; TO="$3"; SECS="${4:-240}"; shift 4 || shift $#
SCRIPTS=("$@"); [ ${#SCRIPTS[@]} -eq 0 ] && SCRIPTS=(bait lakecast invtest)
cd "$(dirname "$0")/../.."
n=180
for s in "${SCRIPTS[@]}"; do
    d="$OUT/$TO/$s"; mkdir -p "$d"; cp "$OUT/$FROM/$s/r.z64" "$d/r.z64"; rm -f "$d/r.ram"; n=$((n + 1))
    (DISP=":$n" timeout $((SECS + 300)) fishmod/tools/ares_capture.sh "$d/r.z64" "$d/shots" "$SECS" 60 > /dev/null 2>&1 || true) &
done
wait
for s in "${SCRIPTS[@]}"; do python3 fishmod/tools/state_hash.py "$OUT/$FROM/$s/r.ram" "$OUT/$TO/$s/r.ram" | tail -1; done
