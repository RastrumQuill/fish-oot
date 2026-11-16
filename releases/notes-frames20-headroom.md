# frames20-headroom: the props' and the fish's CPU time (branch claude/fish-prop-headroom)

`fish-oot-frames20-headroom.bps`: frames20-colnew's tree (the collision work) with the props' draw, the bubbles and
the far school followers made cheaper. Built as colnew (`fishmod/build.sh --script pcprof_frames --hw --colpack
--meshpack --gcc-all`, GCC 10.5), so the two compare on the console.

**The queue** (`fishmod/pc/morning_queue.txt`): colnew, headroom, colnew, headroom, 12 minutes each.
**What to compare:** in the frame log, "fish draw" (the manager's whole draw: the props are inside it), "props draw",
"fish update", CPU draw and the hitches.

## What was found

The frame log's "fish draw" is the fish manager's draw, and the props are drawn inside it: on the route about two
thirds of it was props (`EnFishmod_ScatterDraw`), not fish. The fish's own drawing and thinking are spread thin
over many small functions that have been through several passes already; there was no one thing to fix there.

## What changed

**Props** (`fishmod_scatter.c`):

- A set (which props are drawn, and how) now holds for a turn under 4 degrees and a move under 24 units (was 1.6
  degrees and 6 units, so a walking camera made a new set nearly every frame, 6 ms each). The view tests already have
  5 degrees and each prop's own room to spare, so nothing is missing at the picture's edge; distances, tiers and the
  dissolve are at most a few frames late. Sets on the route: 46% of frames -> 20%.
- Making a set: one pass over each chunk's props in a row with a small record per kind; the tier chosen in place; the
  single cards' matrices written directly. The same sets as before (every set of the route hashed in both builds:
  1,439 sets, all the same, before the thresholds were changed).
- A frame's commands: each drawn prop is 16 bytes made with the set (its matrix, model, sway, colour), read in a row.
  A single card's dissolve is worked out with the set, so all of a frame's commands are kept while the set, the light
  and the touched plants are the same (was: only up to the first single card, which is early). Each material is
  a display list of its own, made once (a change of material is one command, not a dozen; a frame has about forty).
  A prop's colour is sent only when it differs from the one before. The buffers' cache lines are claimed before
  they're written.
- Only the sway matrices the set uses are remade each frame. Plants making way for Link: only chunks and props within
  his reach are read.

**Bubbles** (`z_eff_ss_bubble.c`): the matrix written a word at a time (the same words as before).

**Fish** (`z_en_fishmod.c`): a school member that only keeps its place behind its leader does so every 2nd frame past
800 units from attention and every 4th past 1200, with that many frames' way at once (under a pixel or two there). A
fright, a startle or its full model drawn puts it back on every frame.

## In ares (the pcprof_frames route)

| | before | after |
|---|---|---|
| props draw, by its own stage timers (ms a frame) | 5.86 | 2.09 |
| frame log: props draw, slow / full-speed frames (ms) | 8.2 / 5.0 | 3.9 / 2.0 |
| frame log: fish draw (props included), slow / full-speed (ms) | 12.3 / 7.7 | 7.5 / 4.8 |
| frame log: fish update, slow / full-speed (ms) | 5.1 / 4.4 | 4.3 / 3.9 |
| PC samples: the props' draw (share of all CPU time) | 5.8% | 2.2% |
| PC samples: EffectSsBubble_Draw | 0.72% | 0.17% |
| PC samples: under the fish's updates (the profiler's tag) | 8.1% | 7.0% |
| PC samples: idle | 12% | 16% |
| frames over 52 ms | 4% | 2-3% |

(The frame log's times include whatever interrupts them, the audio thread mostly, so they run higher than the
samples' shares would give. "Before" for the frame log and the samples is the first step of this branch, whose sets
and commands were still made as often as before.)

**The picture:** the camera tour's 22 stops (`--script tour_auto`, `fishmod/tools/tour_capture.py`) captured before
and after each step and compared side by side: the same plants in the same places and colours; the differences are
the water, the sway and the fish, which move.

## What this doesn't change

The RDP. On the console the lake's fill is close to the whole frame by itself (46.8 of 50 ms in the slow frames'
table), and nothing here draws fewer pixels: this makes the CPU's side of a frame shorter, so fewer frames are late
on the CPU's account, but more plants or more fish in view cost RDP time first.
