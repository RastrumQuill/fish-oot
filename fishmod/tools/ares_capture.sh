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

# Each run gets its own copy of the settings: runs side by side don't race on the shared file, and a run of another
# ares (e.g. the system's /usr/bin/ares, which rewrites it) can't break them. The copy gets the two settings such a run
# resets: an audio driver line (without it the N64 build quits at once: a black capture, no save) and Defocus: Allow
# (a headless window never has focus).
SETTINGS="$OUT/settings.bml"
cp "$HOME/.local/share/ares/settings.bml" "$SETTINGS" 2>/dev/null || : > "$SETTINGS"
sed -i 's/^  Defocus: Pause$/  Defocus: Allow/' "$SETTINGS"
if ! awk '/^Audio$/{a=1;next} a&&/^  Driver:/{f=1} /^[A-Z]/{a=0} END{exit !f}' "$SETTINGS"; then
    sed -i '/^Audio$/a\  Driver: None' "$SETTINGS"
fi

DISPLAY="$DISP" "$ARES" --settings-file "$SETTINGS" --system "Nintendo 64" --setting Audio/Driver=None --setting Input/Defocus=Allow \
    --no-file-prompt "$ROM" >"$OUT/ares.log" 2>&1 &
PID=$!
trap 'kill $PID 2>/dev/null || true' EXIT

t=0
while [ "$t" -lt "$SECS" ]; do
    sleep "$INTERVAL"
    t=$((t + INTERVAL))
    DISPLAY="$DISP" import -window root -crop 750x570+246+178 +repage "$OUT/shot_$(printf %04d "$t").png"
done
