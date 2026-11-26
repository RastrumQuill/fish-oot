# The strips A/B (26 November)

**`fish-oot-stripsab-off.bps`** and **`fish-oot-stripsab-on.bps`**: the frame-log bench (`pcprof_frames`, the short
route at Lake Hylia, as the frames builds), the same build twice, with one difference: in `-on` every scene's
triangles are drawn as F3DEX3 triangle snakes (`fishmod/build.sh --script pcprof_frames --hw --texpack --colpack
--meshpack --gcc-all --strips`, tools/strippass.py).

- 88,241 of the game's 155,908 scene triangles in 15,022 snakes (about 6 each); the triangle commands 640 KB -> 457 KB
  (-29%). Each triangle keeps its winding; the picture is the same (the Temple of Time's five views in ares: at most
  80 pixels differ, from coplanar order, not the geometry).
- The question: at Lake Hylia the RDP is the limit and the RSP waits on it, so the RSP's extra work for a snake may
  hide in that wait while the RDRAM a snake saves goes to the RDP. F3DEX3's own figures for a typical scene: ~70 µs
  saved on the RDP side, ~400 µs more RSP.

**Run**: as a frames bench (SummerCart64 log). For a fair comparison, run them alternately, off then on, twice each, on
one power-on if you can (the console warms up over a session). Send the logs; what decides it is each leg's frame time
and RDP time, and the RSP's stall on the RDP.
