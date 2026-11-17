#!/usr/bin/env bash
# The sweep worker on a Linux machine (Ubuntu or Debian: a PC's WSL, a Colab session, a server). The first time it
# installs what the runs need and builds the patched ares (about 15 minutes); after that it starts at once.
#   fishmod/sweep/worker.sh --rom /path/to/oot-ntsc-1.2.z64 [worker.py's options]
# Everything it makes is under ~/.fishoot-sweep (FISHOOT_SWEEP_WORK to put it elsewhere). See README.md here.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="${FISHOOT_SWEEP_WORK:-$HOME/.fishoot-sweep}"
mkdir -p "$WORK"
SUDO=""; [ "$(id -u)" = 0 ] || SUDO="sudo"
need=""
for c in cmake ninja g++ pkg-config Xvfb gdb-multiarch git python3 xdpyinfo; do command -v $c >/dev/null 2>&1 || need="$need $c"; done
if [ -n "$need" ] || [ ! -e "$WORK/.deps" ]; then
    echo "installing packages (missing:${need:- none checked yet})"
    export DEBIAN_FRONTEND=noninteractive
    $SUDO apt-get update -qq
    $SUDO apt-get install -y -qq build-essential pkg-config cmake ninja-build git python3 xvfb x11-utils gdb-multiarch \
        libgtk-3-dev libgl-dev libglu1-mesa-dev libx11-dev libxrandr-dev libxi-dev libasound2-dev libpulse-dev \
        libao-dev libudev-dev libopenal-dev mesa-vulkan-drivers libvulkan1
    touch "$WORK/.deps"
fi
# (the patches' own stamp: a new patch set builds ares again)
STAMP=$(cat "$HERE"/../tools/ares-*.patch "$HERE/../tools/build_ares.sh" | sha1sum | cut -c1-12)
if [ ! -x "$WORK/ares/build/rundir/bin/ares" ] || [ "$(cat "$WORK/ares.stamp" 2>/dev/null)" != "$STAMP" ]; then
    echo "building ares with the repository's patches (about 15 minutes; log: $WORK/ares-build.log)"
    find "$WORK/ares/build" -delete 2>/dev/null || true
    "$HERE/../tools/build_ares.sh" "$WORK/ares" > "$WORK/ares-build.log" 2>&1 || { tail -20 "$WORK/ares-build.log"; exit 1; }
    echo "$STAMP" > "$WORK/ares.stamp"
fi
exec python3 "$HERE/worker.py" --ares "$WORK/ares/build/rundir/bin/ares" "$@"
