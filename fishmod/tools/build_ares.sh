#!/usr/bin/env bash
# Builds ares (N64 core only) from source for the capture tools (harness.py, ares_capture.sh, regress.sh), with
# ares-headless.patch: builds without SDL3 (silent audio, no host input: the test ROMs' scripts drive the game; Ubuntu
# 24.04 doesn't package SDL3) and ARES_AUTOSHOT=<dir> (a screenshot every ARES_AUTOSHOT_EVERY frames, default 15).
# The system's ares (Ubuntu's 134) has no N64 core.
#
# Usage: fishmod/tools/build_ares.sh [dir]   (default /opt/src/ares; then ARES=<dir>/build/rundir/bin/ares)
# Needs: cmake ninja-build libgtk-3-dev libgl-dev libglu1-mesa-dev libx11-dev libxrandr-dev libxi-dev
#        libasound2-dev libpulse-dev libao-dev libudev-dev libopenal-dev
set -euo pipefail
DIR="${1:-/opt/src/ares}"
REV=dd737665221a6f58c6c4d2547d2f38b0abe3aef0  # the commit the patch is made for
PATCH="$(cd "$(dirname "$0")" && pwd)/ares-headless.patch"
if [ ! -d "$DIR/.git" ]; then
    git init -q "$DIR"
    git -C "$DIR" remote add origin https://github.com/ares-emulator/ares.git
fi
git -C "$DIR" fetch -q --depth 1 origin "$REV"
git -C "$DIR" checkout -q -f "$REV"
git -C "$DIR" apply "$PATCH"
cmake -S "$DIR" -B "$DIR/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DARES_CORES=n64 -DARES_ENABLE_LIBRASHADER=OFF \
    -DARES_ENABLE_SDL=OFF
ninja -C "$DIR/build"
echo "ARES=$DIR/build/rundir/bin/ares"
