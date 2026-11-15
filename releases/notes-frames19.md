# frames19

**Console, 15 November (run 20261115-0201), against frames18:** hitches 24.5% -> 21.7% of pictures (the fewest yet),
frames over 52 ms 38% -> 35%, frame period 53.6 -> 53.2 ms, RDP 44.0 -> 43.6 ms, RSP graphics -0.4 ms.

**`fish-oot-frames19.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --meshpack --gcc-all`): frames18
with bush weed, tall weed and ribbon weed drawn with their whole cards up close (gen_habitat.py NEAR_WHOLE): the
prop sweep (releases/notes-prop-sweep.md) measured the cut outline costing the RDP more for these three (0.46, 0.74
and 0.26 ms a frame). The picture: the same but for a few see-through texels at the cards' edges. Compare with
frames18 (run 20261114-1127). ares: the route the same (leg 7 at frame 2,400, as frames18).

**The rocks** (fishmod/tools/rock_designs.py, fishmod/docs/rock-designs.png): the sweep's biggest single kind (1.55
ms). Pruning each rock's triangles that lie wholly under the bed saves nothing (they're undersides, culled already);
cutting the rocks at the bed saves 1% at most; a new 20-triangle rock 8-10% and looks worse. Their cost is the
pixels of the rocks you see: unchanged here.
