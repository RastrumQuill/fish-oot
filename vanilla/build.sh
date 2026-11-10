#!/usr/bin/env bash
# Builds the vanilla bench ROM (vanilla/README.md): the unmodified decomp (this repository's import commit) with only
# vanilla/apply.py's additions, for the US NTSC 1.2 ROM, compressed as the original. The tree is a git worktree
# outside the repository's own build (default: build/vanilla-bench); the ROM is written to
# fishmod/out/vanilla-bench-ntsc-1.2.z64 and its ELF next to it (.elf).
#
# Usage: vanilla/build.sh [tree]   (needs baseroms/ntsc-1.2/baserom.z64, as the mod's build)
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT="$(pwd)"
TREE="${1:-$ROOT/build/vanilla-bench}"
IMPORT=2a3450826ad068512d569e63bd605a9653997575 # "Import zeldaret/oot decompilation at ceeb6f3"

if [ ! -d "$TREE/.git" ] && [ ! -f "$TREE/.git" ]; then
    git worktree add -f --detach "$TREE" "$IMPORT"
fi
cd "$TREE"
git checkout -q -f "$IMPORT"
git clean -qfd -e baseroms -e extracted -e build -e .venv
mkdir -p baseroms/ntsc-1.2
cp "$ROOT/baseroms/ntsc-1.2/baserom.z64" baseroms/ntsc-1.2/baserom.z64
[ -e .venv ] || ln -s "$ROOT/.venv" .venv
if [ ! -d extracted/ntsc-1.2/assets ]; then
    make setup VERSION=ntsc-1.2 -j4
fi
python3 "$ROOT/vanilla/apply.py" "$TREE"
make VERSION=ntsc-1.2 REGION=US NON_MATCHING=1 COMPARE=0 -j4 rom compress
mkdir -p "$ROOT/fishmod/out"
cp build/ntsc-1.2/oot-ntsc-1.2-compressed.z64 "$ROOT/fishmod/out/vanilla-bench-ntsc-1.2.z64"
cp build/ntsc-1.2/oot-ntsc-1.2.elf "$ROOT/fishmod/out/vanilla-bench-ntsc-1.2.elf"
echo "Built fishmod/out/vanilla-bench-ntsc-1.2.z64"
