# remesh: the lake bed along its depth lines

The bed's water fog (by band) and its caustics' fade (by step) are set per triangle. With each whole triangle in the
band of its middle (rc1, rc2), the fog changed along triangle edges: seams on the shallow bed (19 Nov). Cutting the
triangles at the depths (frames28) moved the changes onto depth lines but made 2,101 triangles and cost about 1 fps.

`tools/meshpack_remesh.py` cuts the near-flat bed under the water at every depth line and decimates it again, its
texture coordinates projected straight down (the same density and direction as before), every point of the cut bed
within 3 units of the result (the collision, which the plants and fish stand on, is unchanged). Steep faces under the
water (pillars, the pit's walls) are cut and kept as they are; everything over the water is unchanged.

- 8 depth lines: the fog's bands' tops at 0, 20, 50, 100, 200, 300, 400, 500; the deep ones are the caustics' steps'
  edges (the fade from 150 to 550 units in 4 steps, as before), so both change along the same lines.
- The bed's lists: 1,633 triangles (609 uncut, 2,101 cut as in frames28).

## Patches

- `fish-oot-remesh-bench.bps`: the bench with the remesh.
- `fish-oot-remesh-off-bench.bps`: the same tree with `MESHPACK_NO_REMESH=1` (each whole triangle by its middle):
  the baseline, built here.

The queue: off, remesh, off, remesh, 12 minutes each, Expansion Pak in.
