#!/usr/bin/env bash
# Builds ares (N64 core only) from source for the capture tools (harness.py, ares_capture.sh, regress.sh), with
# ares-autoshot.patch: ARES_AUTOSHOT=<dir> saves a screenshot every ARES_AUTOSHOT_EVERY frames (default 15), and
# ARES_FRAMEHASH=<file> writes a hash of every frame (two builds compared by their sequences of frames). Release
# v148, without SDL (the test ROMs' scripts drive the game; the audio driver is None). The system's ares (Ubuntu's
# 134) has no N64 core; ares's development head after v148 needs SDL3 (not packaged on Ubuntu 24.04) and crashed in
# its new RDRAM emulation even on the unmodified game.
#
# Usage: fishmod/tools/build_ares.sh [dir]   (default /opt/src/ares; then ARES=<dir>/build/rundir/bin/ares)
# Needs: cmake ninja-build libgtk-3-dev libgl-dev libglu1-mesa-dev libx11-dev libxrandr-dev libxi-dev
#        libasound2-dev libpulse-dev libao-dev libudev-dev libopenal-dev; to run: mesa-vulkan-drivers (N64 video is Vulkan)
set -euo pipefail
DIR="${1:-/opt/src/ares}"
REV=v148  # the release the patch is made for
PATCH="$(cd "$(dirname "$0")" && pwd)/ares-autoshot.patch"
if [ ! -d "$DIR/.git" ]; then
    git init -q "$DIR"
    git -C "$DIR" remote add origin https://github.com/ares-emulator/ares.git
fi
git -C "$DIR" fetch -q --depth 1 origin tag "$REV"
git -C "$DIR" checkout -q -f "$REV"
git -C "$DIR" apply "$PATCH"
git -C "$DIR" apply "$(dirname "$PATCH")/ares-sc64.patch" # ARES_SC64_DIR=<dir>: a minimal SummerCart64, its USB packets saved there
# the profilers (one patch: they share files): ARES_RSPPROF=<file> the RSP's instructions by PC (rspprof.py);
# ARES_CPUPROF=<file> the CPU's instructions, cycles and cache fills by PC (cpuprof.py); ARES_BUSPROF=<file> the bytes
# moved to and from RDRAM by who asked (busprof.py)
git -C "$DIR" apply "$(dirname "$PATCH")/ares-prof.patch"
# ARES_NORDP=1: the RDP's commands are read and its interrupts raised, but nothing is drawn (the audio sweep's runs,
# which read only the CPU's and the sound's results: about twice as fast)
git -C "$DIR" apply "$(dirname "$PATCH")/ares-nodraw.patch"
# ARES_AIPROF=<file>: what the audio interface played, by sweep leg: buffers taken and refused, times it ran dry,
# silence, a hash of the samples (the audio sweep's runs in real time)
git -C "$DIR" apply "$(dirname "$PATCH")/ares-aiprof.patch"
cmake -S "$DIR" -B "$DIR/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DARES_CORES=n64 -DARES_ENABLE_LIBRASHADER=OFF \
    -DARES_ENABLE_SDL=OFF
ninja -C "$DIR/build"
echo "ARES=$DIR/build/rundir/bin/ares"
