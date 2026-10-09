#!/usr/bin/env bash
# Runs a ROM in ares on a virtual X display and captures screenshots.
#
# Usage: fishmod/tools/ares_capture.sh <rom> <outdir> <seconds> [interval]
#   Takes a screenshot every [interval] seconds (default 5) for <seconds>,
#   cropped to the game view. Needs ARES=/path/to/ares (built with the N64 core),
#   Xvfb and ImageMagick.
set -euo pipefail

ROM="$1"
OUT="$2"
SECS="$3"
INTERVAL="${4:-5}"
ARES="${ARES:-ares}"
DISP="${DISP:-:99}"

mkdir -p "$OUT"
if ! xdpyinfo -display "$DISP" >/dev/null 2>&1; then
    Xvfb "$DISP" -screen 0 1280x960x24 >/dev/null 2>&1 &
    sleep 2
fi

DISPLAY="$DISP" "$ARES" --system "Nintendo 64" --setting Audio/Driver=None --setting Input/Defocus=Allow \
    --no-file-prompt "$ROM" >"$OUT/ares.log" 2>&1 &
PID=$!
trap 'kill $PID 2>/dev/null || true' EXIT

t=0
while [ "$t" -lt "$SECS" ]; do
    sleep "$INTERVAL"
    t=$((t + INTERVAL))
    DISPLAY="$DISP" import -window root -crop 750x570+246+178 +repage "$OUT/shot_$(printf %04d "$t").png"
done
