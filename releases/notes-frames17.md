# frames17

**Console, 14 November (run 20261114-1035), against frames16:** hitches 32.7% -> 30.5% of pictures, frame period
-0.6 ms, RDP -0.3 ms overall. The legs past the dock: leg 3 (north beach) RDP 47.8 -> 43.6 ms, frame 58.2 -> 53.3,
slow frames 59% -> 39%; leg 4 RDP 52.1 -> 48.8, frame 63.9 -> 57.3, slow frames 95% -> 69%. Elsewhere: CPU update
+0.7 to +2.4 ms on most legs (the enemies' updates among it: leg 7 +1.9), leg 5's RSP +5.3 ms: not the dock. Likely
the cache-placement effect again (frames16's notes): the fish overlay grew, so every overlay loaded after it lands at
other addresses; to be pinned down before the next comparison.

**`fish-oot-frames17.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --meshpack --gcc-all`): frames16
with the north-beach dock rebuilt. The PC profile and frame log as in every bench build. Compare with frames16
(14 November, run 20261114-0923): the dock legs (6, "Pillar ruins (under) -> Lab shore and dock", and those that pass
the north beach) are where it shows.

**The dock** (fishmod/tools/gen_habitat.py, mesh_dock_*; the designs: fishmod/docs/dock-designs.png; in the game,
before and after: fishmod/docs/dock-before-after.png). Its six-sided pilings ran 150 units down from the deck, most
of that through the lake bed, with their back faces drawn: tall thin triangles, which the RDP pays for scanline by
scanline (rdp_cost.py: up to 5 ms a frame for the dock's posts on the bench route). Now:
- four pairs of posts at the deck's edges, standing 22 above it (as the island bridge's), each a pentagon with its top
  turned half a side (round as a true circle within 1%, in 10 triangles: fishmod/docs/low-poly-shapes.md), shaded
  from round normals, stained darker below the waterline; twisted triangles (6 triangles) past 500 units;
- each post cut 10 units under the lake bed where it stands (the scene's collision);
- back faces culled (the posts and the deck: closed solids), the deck closed at both ends with a rim board each side;
- its collision from the same numbers (the posts stand above the deck: Link can't walk through them).

ares, the dock tour's five cameras (fishmod/scripts/dock_tour.inc.c), the dock's RDP time by the cost model: from the
beach 4.6 -> 1.7 ms, side on 6.4 -> 1.7, under the water 6.5 -> 2.5, from across the lake 2.6 -> 0.6, close on a post
8.4 -> 4.0. The bench route: the same legs at the same frames as frames16; the props' CPU draw 5.05 -> 4.73 ms.
