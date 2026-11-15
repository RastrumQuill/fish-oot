# terrain sweep

**Console, 15 November (18 laps), against the shipped laps beside each:** terrain RA saves 2.9 ms of RDP a frame and 8
hitch points, on every leg (1.5-4.4 ms); sky last costs 1.0 ms (the depth test on every sky pixel more than the covered
pixels it skips); both 1.9 ms. The terrain's RA is the game's from frames23 on; the sky stays first.

**`fish-oot-terrain-sweep.bps`** (`fishmod/build.sh --script terrain_sweep --hw --colpack --meshpack --gcc-all`):
frames21's game, 18 laps of clear dawn (about 70 minutes), every other lap as shipped, between them:

- **terrain RA**: the room's opaque render modes without the colour read (IM_RD): the RDP still works out each
  pixel's coverage and writes it (the VI smooths the edges with it, as it does for the props), but doesn't read each
  pixel's colour back first, which only its own blend at the edges used (Fishmod_TerrainRA: found in the room's lists
  at load, the bit flipped in place).
- **sky last**: the sky (and the sun and moon) drawn after everything opaque, depth-tested at a fixed depth beyond
  the scene: it fills only the pixels nothing covers, instead of being drawn first and then painted over (the cost
  model's frames: 40-70 thousand of the sky's pixels a frame covered by the terrain).
- **both**.

ares (dock_tour_skylast against dock_tour): the picture the same but along polygons' edges (0.1-2% of the pixels: the
RDP's own edge blend gone, the VI's left).

Read it with `fishmod/tools/anatomy_sweep_report.py RUN_FOLDER --terrain`.
