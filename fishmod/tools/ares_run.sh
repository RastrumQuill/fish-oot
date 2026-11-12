#!/usr/bin/env bash
# Runs a ROM in ares headless for a while, several at once if wanted: each run gets its own GDB port and its own
# SummerCart64 USB folder, so runs don't collide (two ares sharing a port or folder log nothing).
#
# Usage: fishmod/tools/ares_run.sh SLOT ROM SECONDS [USB_DIR] [-- GDB commands...]
#   SLOT 0-3 picks the GDB port (9123 + SLOT). With USB_DIR, the frame log and profile go there (ARES_SC64_DIR).
#   After SECONDS, the GDB commands (each one an -ex argument) run against the emulator, then it's stopped.
# Needs ares (ARES=...), an X display in DISPLAY (Xvfb), gdb-multiarch for the commands.
set -u
SLOT=$1 ROM=$2 SECS=$3 USB=${4:-}
[ "$USB" = "--" ] && USB=
shift 3; [ $# -gt 0 ] && [ "$1" != "--" ] && shift
[ "${1:-}" = "--" ] && shift
ARES=${ARES:-/opt/src/ares/build/rundir/bin/ares}
HERE="$(cd "$(dirname "$0")" && pwd)"
PORT=$((9123 + SLOT))
CONF=$(mktemp --suffix=.bml)
sed "s/^\(\s*Port:\s*\)9123/\1$PORT/" "$HERE/ares_debug.bml" > "$CONF"
# ARES_FAST=1: no video or audio output (the emulation unchanged: the RDP still draws into RDRAM): about twice as fast
if [ "${ARES_FAST:-0}" = 1 ]; then
    sed -i -e '/^Video$/,/^[A-Z]/ s/^  Driver: .*/  Driver: None/' -e '/^Audio$/,/^[A-Z]/ s/^  Driver: .*/  Driver: None/' \
        -e '/^Audio$/,/^[A-Z]/ s/^  Blocking: .*/  Blocking: false/' "$CONF"
fi
xdpyinfo >/dev/null 2>&1 || { (Xvfb "${DISPLAY:-:190}" -screen 0 1024x768x24 >/dev/null 2>&1 &); sleep 2; }
if [ -n "$USB" ]; then mkdir -p "$USB"; fi
ARES_SC64_DIR=$USB timeout $((SECS + 120)) "$ARES" --settings-file "$CONF" --system "Nintendo 64" --no-file-prompt "$ROM" \
    >/dev/null 2>&1 &
APID=$!
sleep "$SECS"
if [ $# -gt 0 ]; then
    ARGS=()
    for c in "$@"; do ARGS+=(-ex "$c"); done
    gdb-multiarch -q -batch -ex "set architecture mips:4000" -ex "set endian big" -ex "target remote 127.0.0.1:$PORT" \
        "${ARGS[@]}" 2>&1 | grep -v "^warning\|^The target\|^0x\|Inferior\|determining" || true
fi
kill $APID 2>/dev/null; wait $APID 2>/dev/null
rm -f "$CONF"
