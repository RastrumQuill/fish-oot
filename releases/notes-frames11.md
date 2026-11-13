# frames11: the sky and clears, collision and matrices, the fish's list and edges; with and without run-ahead

Two builds of the same code (`fishmod/build.sh --script pcprof_frames --hw --colpack --gcc-all`):
- **`fish-oot-frames11.bps`**: with run-ahead (the third frame buffer), as frames10.
- **`fish-oot-frames11-norun.bps`**: without it (`GCC_EXTRA_OPT=-DFISHMOD_NO_RUNAHEAD`; touch `src/code/sys_cfb.c` before
  and after, or make keeps the other object).

Both on frames10 (see [its notes](notes-frames10.md)) plus:

**Graphics (the RDP is what limits the route now):**
- **No colour clear under Lake Hylia's sky.** The sky and the scene cover the screen and the sky's render mode reads
  nothing back; the fill (0.91 ms of RDP on the bench) is turned into a no-op once the sky is drawn. Under water (no
  sky) the fill stays. The sky now writes full coverage outright (what the fill's full coverage clamped to before).
- **The Z buffer cleared by the RSP** (F3DEX3's memset) right after the sky's commands, while the RDP draws the sky,
  instead of an RDP fill at the frame's start.
- **The sky in 1-cycle with one texture while the two skies aren't blending** (the 2-cycle combiner gave exactly the
  first texture then): half the sky's texture loads.
- **The fish's list: matrix loads grouped by bone** within each vertex load, and the loaded bone kept from one limb to
  the next. (The research's "every bone's vertices once" doesn't fit: the models have 139 and 238 vertices against
  F3DEX3's 56.)
- **The fish in the near props' edge mode** (anti-aliased without reading the colour buffer: RA_ZB_OPA_SURF2). This one
  changes the picture slightly at fish edges.

**CPU:**
- **The wall tests' clearance bound**: after a scene wall test that finds nothing, any position within the distance
  every wall in the cell stays rejected gets "no wall" without the walk.
- **The display lists' next cache lines claimed** before each actor, room and fish writes them.
- **The limb matrices skip the bottom row** of an affine matrix.

**Measurement:**
- **Press to picture** in the frame log (words 58-59): from the controller read a frame used to the refresh its
  picture goes up at, and how often another finished frame was waiting behind it. `framelog.py` prints it. The point
  of the pair: what run-ahead costs in latency against what it buys in hitches.
- **The calibration loops** (memory or computing) once near the start: `fishmod/tools/inst_profile.py FOLDER --elf ELF
  --func PcCalib_LoadMiss` (and `_Alu`, `_LoadHit`, `_FetchMiss`, `_Div`) shows how each kind of waiting lands in the
  profile, before reading the game's functions the same way.

**Checked before the console** (in ares, for exactness, not timing):
- The sky and clears: A/B screenshots at 6 tour stops above and below water: the same to one level of dither, but for
  about 27 pixels at one stop that differ the same whichever single change is switched (something there moves).
- The wall bound: the collision logs with and without it, the same over 5,469 frames.
- The matrices: the old and new step on 2,000,000 random matrices give the same fixed-point output.
- The player builds (IDO, console and emulator) compile.

**Not in it: the batched audio (S7).** The game's sound effects would start up to two refreshes later (not provable
with the audio check), and the audio frames would have to triple in length (the audio heap's output, reverb and DMA
buffers resized): too much risk to ride in this build. It comes as its own build after this one.

## Results (console, 12 Nov, run 20261112-1748: frames11 and -norun twice each, frames10 between)

| | on screen | hitches | RDP (slow frames) | CPU update | CPU draw | press to picture |
|---|---|---|---|---|---|---|
| frames10 | 18.40 fps | 26% | 48.8 ms | 23.8 | 23.7 | (not measured) |
| frames11 (x2) | 18.21-18.29 | 30-31% | 46.4 | 25.6 | 26.7 | 121 ms mean, 140 ms 90% |
| frames11-norun (x2) | 18.30-18.38 | 28-29% | 46.1 | 26.0 | 26.5 | 120 ms mean, 132 ms 90% |

- The graphics items took about 2.4 ms off the RDP in the slow frames (estimated 3-5).
- The CPU got 4-5 ms slower: nearly every actor's update 5-15% slower at the same counts (the actor report), Link's
  collision 15% slower despite the wall bound. Something global (code layout in the instruction cache, or memory
  traffic): frames11b bisects it.
- Run-ahead buys nothing now: no picture ever waited behind another (0%), the latency is the same (121 against 120 ms),
  and the hitches no fewer.
- The runs froze in their last seconds: not the game's fault. The runner uploads the next ROM into the cart while the
  finished game still runs, and the game reads the new ROM's bytes as its own (the last run of a queue never froze:
  nothing after it). From frames11b on, the game halts itself a second after DONE.
