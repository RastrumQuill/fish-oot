# Test 5b: invisible fill-rate optimizations, and the second hardware bench

The first bench showed the props cost the most on the console: the RDP spends its time filling their pixels. These
builds draw the same picture for less.

## In every build
- **Prop edges without a colour-buffer read.** The cut-out edges used the RDP's anti-aliasing, which reads the colour
  buffer for every prop pixel. Now the RDP only writes the coverage, and the VI smooths the edges from it (libultra's
  "reduced aliasing" approach). Compared at 11 tour stops in ares, whose RDP and VI match the console's: no visible
  difference.
- **Tighter card outlines.** Re-fitted with more vertices allowed, now that pixels are known to cost more than
  triangles. Each card fills about 2% less on average, up to 4% (roots, rushes, reeds). The picture is the same: the
  outline still holds every texel that can show.

## 1. `fish-oot-hwbench2.bps`: the second overnight bench
It works the same way as the first: boot it, leave it, and send the `.sav`. It needs a fresh save: the old results
don't carry over, since the modes changed. Each stop is timed in 9 modes:

| Mode | What it tells us |
|---|---|
| base | as shipped, with the two changes above (compare with the first bench's base) |
| altedge | the old edge mode: what the colour-buffer read cost |
| nozupd | props without depth writes: what writing depth costs |
| 1cycle | props in 1-cycle mode, without fog or tint: is the RDP limited by its pipeline or by memory? |
| nonear / nofar | only the props past or within 500 units: where the props' cost is |
| noprops | no props |
| nosky | no props and no skybox: what the sky costs |
| noxlu | no props and no water surface: what the water costs |

The 1-cycle and no-depth-write modes change the picture slightly. They're there to measure, not to ship. A lap takes
longer than before (9 modes instead of 5), about 40 minutes.

## 2. `fish-oot-kokiri-test5b.bps`
The Kokiri / Lake Hylia test as in test 5 (D-pad Down to the lake as adult Link), with these optimizations.

## 3. `fish-oot-lake-tour-5b.bps`
The camera tour, with these optimizations.

## PC packs: `selftest.zip` and `rotate.zip`
Put both in `fishmod\pc\packs\` (replacing any older ones) and run `fishmod\pc\run_all.bat C:\roms\oot.z64`.
- **selftest:** 4 minutes of the tour. It checks that ares and the patching work.
- **rotate:** the rotation check on the new build. At each tour stop the camera turns all the way round, and the
  game's frames are compared with references, to catch props that pop in. It now waits for the under-water look to
  settle when a direction takes the camera out of the water. That transition spoiled stops 1 and 11 last time. It
  runs 90 minutes per pass; running it again carries on.

Send back `fishmod\pc\results\results_all.zip`.
