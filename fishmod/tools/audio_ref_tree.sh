#!/usr/bin/env bash
# Turns the working tree into the audio sweep's reference (fishmod/docs/audio-sweep.md) and back: the mod's audio as
# it was before the audio work (commit REF: the driver, its headers, the scheduler, the link order and flags), with
# only what the audio check needs to compare two builds added to it (the lockstep's waits for loads and its
# stand-in for the random numbers, as the current tree has them, and the two switches code outside the driver sets).
# Everything else (the game, the mod, the test scripts) is the current tree's, so the reference plays the same sweep.
#   fishmod/tools/audio_ref_tree.sh on     (then build as usual, without --expak: the reference has no pak build)
#   fishmod/tools/audio_ref_tree.sh off    (the tree as committed again; do this before committing anything)
set -euo pipefail
REF=fb6601a7
cd "$(dirname "$0")/../.."
FILES="spec/spec include/audio.h include/sched.h src/code/sched.c src/audio/game/session_init.c src/audio/internal/heap.c
src/audio/internal/load.c src/audio/internal/playback.c src/audio/internal/synthesis.c src/audio/internal/thread.c Makefile"
if [ "${1:-}" = off ]; then
    git checkout -q HEAD -- $FILES
    echo "tree: as committed"
    exit 0
fi
[ "${1:-}" = on ] || { echo "usage: $0 on|off"; exit 1; }
git diff --quiet HEAD -- $FILES || { echo "uncommitted changes in the driver's files: commit them first"; exit 1; }
for f in $FILES; do
    [ "$f" = Makefile ] || git show "$REF:$f" > "$f"
done
sed -i 's/ -ffunction-sections \$(GCC_EXTRA_OPT)/ $(GCC_EXTRA_OPT)/' Makefile
sed -i '/src\/code\/sched.o: OPTFLAGS += -ffunction-sections/d' Makefile
python3 - <<'PY'
import re
p = 'src/audio/internal/load.c'
s = open(p).read()
a = "if (osRecvMesg(&gAudioCtx.preloadSampleQueue, (OSMesg*)&preloadIndex, OS_MESG_NOBLOCK) == -1) {"
b = "if (osRecvMesg(&sSampleStoreDmaQueue, NULL, OS_MESG_NOBLOCK) == -1) {"
assert s.count(a) == 1 and s.count(b) == 1
s = s.replace(a, a.replace("OS_MESG_NOBLOCK", "gFishmodAudioLockstep ? OS_MESG_BLOCK : OS_MESG_NOBLOCK"))
s = s.replace(b, b.replace("OS_MESG_NOBLOCK", "gFishmodAudioLockstep ? OS_MESG_BLOCK : OS_MESG_NOBLOCK"))
if "extern s32 gFishmodAudioLockstep" not in s:
    s = s.replace("s32 gAudioContextInitialized = false;", "s32 gAudioContextInitialized = false;\nextern s32 gFishmodAudioLockstep;", 1)
open(p, 'w').write(s)
p = 'src/audio/internal/thread.c'
s = open(p).read()
a = "        gAudioCtx.audioRandom = gAudioCtx.audioRandom + gAudioCtx.aiBuffers[index][gAudioCtx.totalTaskCount & 0xFF];\n"
assert s.count(a) == 2 or s.count(a) == 1
i = s.index("    if (gFishmodAudioLockstep) {\n        /* fish-oot: the audio check: the cycle count and the code's bytes vary")
j = s.index(a, i)
s = s[:j] + s[j + len(a):]
s += "\n/* the reference tree: the switches code outside the driver sets (fishmod/tools/audio_ref_tree.sh) */\nu8 gFishmodAudioAhead;\nvoid (*gFishmodAudioMgrRetrace)(void*) = NULL;\n"
open(p, 'w').write(s)
PY
echo "tree: the audio reference ($REF + the check's harness)"
