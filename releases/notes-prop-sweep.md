# The prop sweep

**`fish-oot-prop-sweep.bps`** (`fishmod/build.sh --script prop_sweep --hw --colpack --meshpack --gcc-all`): frames18's
code on the RDP sweep's bench: 84 laps of clear dawn (about 5.5 hours), every other lap the game as shipped, between
them one plant kind drawn another way. Read it with `fishmod/tools/prop_sweep_report.py RUN_FOLDER`.

The 21 tests, each twice (fishmod_debug_bench.inc.c, FISHMOD_PROP_SWEEP):
- **off** (what the kind costs in all): bush weed, grass tuft, broadleaf, big pondweed, kelp frond, ribbon weed, tall
  weed, vine curtain, rocks;
- **whole**: the kind's whole cards up close too (no cut to the texture's outline): bush weed, grass tuft, broadleaf,
  big pondweed, kelp frond, ribbon weed, tall weed;
- **low**: the same cut outline split into the triangles of least total height (gen_habitat.py low_triangulate: the
  same pixels in fewer scanlines): bush weed, grass tuft, broadleaf, big pondweed, kelp frond.

Why: the RDP cost model (rdp_cost.py, prop_designs.py; fishmod/docs/prop-cards.png) says the cut outlines cost more
than whole cards for most kinds (the ear-clipped cut makes tall triangles, each paying every scanline), and that the
low split is cheapest up close; but the model under-counts the props (3.7 ms against the 8.5 measured), so the
console decides. Checked in ares: lap 0 as shipped, lap 1 bush weed left out (none of its lists drawn), lap 2 as
shipped, lap 3 bush weed whole (its far model drawn near).
