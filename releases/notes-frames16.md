# frames16

**`fish-oot-frames16.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --meshpack --gcc-all`): frames15
(the flat fish, memmove a word at a time) with the fish update's code made compact. The PC profile and frame log run
as in every bench build. Compare with frames14c (13 November) and frames15/15L (14 November).

**Why frames15 was slower than frames14c** (hitches 38% against 28%, the fish update 7.4 ms against 4.9 on the
console): not anything the fish do. The fish update's speed depended on where its code happened to land in the CPU's
16 KB instruction cache, against the engine's own code it runs between: rebuilt and timed in ares, every commit
since 14c, the same 2,000 frames, the same 44.7 fish updates a frame:

| build | fish update, ms a frame |
|---|---|
| 14c | 4.21 |
| 7919d8a (the fish drawing work) | 4.06 |
| e24fb94 (the actor animation check) | 3.35 |
| ee69107 (profile slots) | 4.52 |
| frames15 | 5.88 |
| frames15, its fish code moved by 128 bytes | 5.25 |
| frames15, by 256 bytes | 5.09 |
| frames15, the fish struct 64 bytes bigger | 5.77 (data layout: no effect) |

None of those commits changed what the update does: moving the code alone moved the time by up to 0.8 ms.

**The fix** (z_en_fishmod.c): the update's code kept together and small. EnFishmod_Update 2,864 -> 1,416 bytes:
- the debug readouts (the nearest fish's state for the screen, the clip checks) out of line, in a cold function of
  their own, instead of in the middle of the update (they're skipped in the benches, but their code split the update's);
- the far school member's step (38 of the 45 fish a frame) its own small function, the school slot inlined into it,
  and the swim bookkeeping of a fish not drawn in full done there instead of calling the swim function.

ares, the same 2,000 frames: **4.32 / 4.27 / 4.37 ms** with the fish code at 0 / 128 / 256 bytes (frames15:
5.88 / 5.25 / 5.09). 1.5 ms faster where frames15 was, and the spread 0.1 ms instead of 0.8. The fish behaviour
check (fishstate, 14 fingerprints over 3,150 frames): the same as before.

**On the console, look for:** fish update (the NPC actors) back to about 5 ms or under, CPU update about 2-3 ms less
than frames15, and the hitches back toward frames14c's 28%.
