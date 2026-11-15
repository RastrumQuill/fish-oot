# The prop sweep

**Console, 15 November (run cut short at lap 78 of 84: the PC restarted; the second packet file recovered from its
complete members):** fishmod/tools/prop_sweep_report.py:

223428 frames, laps 0-77; shipped laps' RDP from 44.5 to 44.7 ms (first and last: the night's drift)

## Saved by each test (ms a frame, mean over its laps; RDP / drawing / texture loading; hitch points)

| test | laps | RDP | drawing | loading | frame | hitches |
|---|---|---|---|---|---|---|
| bush_weed off | 2 | 1.17 | 1.16 | 0.02 | 1.39 | +8.3 |
| bush_weed whole | 2 | 0.46 | 0.45 | -0.04 | 0.55 | +3.3 |
| bush_weed low | 2 | 0.33 | 0.33 | 0.02 | -0.01 | -0.1 |
| grass_tuft off | 2 | 1.02 | 1.02 | 0.11 | 0.75 | +4.6 |
| grass_tuft whole | 2 | -0.20 | -0.20 | 0.01 | -0.03 | -0.1 |
| grass_tuft low | 2 | -0.21 | -0.22 | 0.02 | -0.00 | +0.0 |
| broadleaf off | 2 | 0.26 | 0.26 | 0.00 | 0.51 | +3.0 |
| broadleaf whole | 2 | 0.03 | 0.03 | -0.04 | 0.01 | +0.1 |
| broadleaf low | 2 | -0.07 | -0.07 | -0.00 | 0.18 | +1.1 |
| big_pondweed off | 2 | 0.11 | 0.11 | -0.00 | 0.24 | +1.5 |
| big_pondweed whole | 2 | 0.38 | 0.39 | 0.03 | 0.09 | +0.5 |
| big_pondweed low | 2 | 0.24 | 0.24 | 0.00 | 0.31 | +1.8 |
| kelp_frond off | 2 | 0.86 | 0.85 | 0.09 | 0.40 | +2.4 |
| kelp_frond whole | 2 | -0.20 | -0.20 | 0.00 | 0.10 | +0.6 |
| kelp_frond low | 2 | 0.18 | 0.17 | -0.02 | 0.01 | +0.0 |
| ribbon_weed off | 2 | 0.36 | 0.36 | 0.02 | 0.33 | +2.0 |
| ribbon_weed whole | 2 | 0.26 | 0.26 | 0.03 | 0.24 | +1.4 |
| tall_weed off | 2 | -0.04 | -0.04 | 0.02 | -0.22 | -1.3 |
| tall_weed whole | 1 | 0.74 | 0.73 | 0.07 | 0.36 | +2.1 |
| vine_curtain off | 1 | 0.91 | 0.91 | 0.03 | 0.52 | +3.1 |
| rock off | 1 | 1.55 | 1.55 | 0.09 | 0.71 | +4.3 |

## By leg: RDP ms saved (spread across its laps)

| test | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 21 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bush_weed off | 2.8 | 1.3 | 0.6 | 1.4 | 3.2 | 1.4 | 0.6 | 0.8 | 1.3 | 0.4 | -0.2 | 0.4 |
| bush_weed whole | 0.4 | 0.1 | 0.1 | 0.3 | 1.2 | -1.2 | 0.3 | 0.6 | 0.6 | 0.5 | 0.0 | 2.6 |
| bush_weed low | -0.4 | -0.4 | -0.6 | -0.3 | -0.2 | 0.5 | 0.4 | -0.1 | 2.1 | 0.7 | 0.3 | 1.8 |
| grass_tuft off | 1.6 | 0.7 | 2.2 | 1.3 | 1.5 | 0.3 | -0.2 | 0.2 | 3.9 | 0.7 | 0.6 | -0.6 |
| grass_tuft whole | 0.1 | 0.2 | 0.3 | -0.1 | -0.1 | 0.3 | 0.6 | 0.2 | -1.4 | 0.0 | -0.7 | -1.9 |
| grass_tuft low | 0.4 | -0.3 | 0.6 | -0.1 | 0.2 | -0.0 | 0.0 | 0.5 | -2.7 | 0.5 | 0.5 | -2.1 |
| broadleaf off | 2.0 | 0.0 | 0.4 | 0.2 | 0.4 | 0.7 | -0.7 | 1.1 | -0.3 | -0.6 | -0.5 | 0.4 |
| broadleaf whole | 0.9 | 0.1 | -0.2 | -0.3 | -0.1 | -0.1 | 0.2 | 0.7 | -1.0 | -0.2 | 0.4 | -0.1 |
| broadleaf low | -0.5 | -0.1 | -0.9 | 0.2 | 0.5 | 0.6 | -0.0 | 0.8 | -1.2 | 0.2 | -0.2 | -0.2 |
| big_pondweed off | 1.7 | 1.1 | 0.3 | 0.0 | -0.2 | -0.0 | -0.4 | -0.7 | 0.7 | -0.0 | -0.1 | -1.2 |
| big_pondweed whole | 0.6 | 0.2 | 0.5 | -0.1 | -0.0 | -0.0 | 0.3 | -0.1 | 1.0 | 0.9 | 0.1 | 1.3 |
| big_pondweed low | 1.2 | 0.6 | -0.2 | 0.2 | 0.2 | -0.1 | 0.2 | -0.9 | 0.3 | -0.7 | 0.4 | 1.7 |
| kelp_frond off | 0.3 | 0.0 | 0.6 | 0.1 | 0.5 | 0.5 | -0.2 | 0.3 | 3.2 | 0.2 | 0.3 | 4.5 |
| kelp_frond whole | 0.3 | 0.3 | -0.7 | 0.2 | -0.1 | 0.7 | -0.6 | -0.2 | 0.2 | -0.6 | -0.1 | -1.6 |
| kelp_frond low | -0.6 | 0.4 | -0.6 | 0.5 | 0.5 | 0.2 | -0.1 | 0.2 | 0.3 | 0.4 | -0.0 | 1.0 |
| ribbon_weed off | 1.1 | 0.9 | 1.0 | 0.6 | 0.2 | -0.6 | 3.4 | -1.6 | 0.4 | -0.5 | -0.8 | 0.1 |
| ribbon_weed whole | 0.2 | -0.4 | 0.2 | -0.2 | 0.4 | 0.7 | 1.0 | -0.6 | 0.0 | -0.6 | 0.1 | 2.3 |
| tall_weed off | -0.2 | -0.6 | -0.7 | -0.5 | -0.2 | -0.6 | 0.0 | -2.9 | 3.7 | 0.3 | -0.8 | 4.4 |
| tall_weed whole | 0.7 | 0.0 | -0.5 | -0.0 | 0.7 | 0.1 | 0.6 | 1.8 | -0.2 | 1.1 | 1.2 | 3.6 |
| vine_curtain off | 1.2 | 1.0 | 2.1 | 3.5 | 2.5 | 0.9 | 0.7 | -0.7 | -0.9 | 0.9 | -0.4 | 0.1 |
| rock off | 1.9 | 2.2 | 1.3 | 0.9 | 0.7 | 0.1 | 2.1 | 1.6 | 1.5 | 3.9 | 1.2 | 1.1 |

Reading it: each test has two laps (the last three, one), and the shipped laps beside them differ by about 0.3 ms:
smaller differences are noise. What each kind costs in all (off): the rocks 1.55 ms, bush weed 1.17, grass tuft
1.02, vine curtain 0.91, kelp 0.86, ribbon weed 0.36, broadleaf 0.26, big pondweed 0.11, tall weed about 0. Whole
cards up close cost less than the cut for bush weed (0.46 saved), tall weed (0.74, one lap) and ribbon weed (0.26);
the cut is better for grass tuft and kelp (whole: 0.2 more). The low split: bush weed 0.33 saved, big pondweed
0.24, kelp 0.18; grass tuft 0.2 worse.

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
