# claude/rc1-merge-ready: what's left to merge, on main as it is now

main (9d1d2b6 when this was last brought up to it) already has the props' and fish's CPU work: f5285cd merged claude/fish-prop-headroom onto rc1 with
its own port (the entries, the one-pass set, the water fog per entry, the bending in bands). So
`claude/rc1-headroom`'s port of the same work is superseded: don't merge that branch. This branch starts from main and
has only what main doesn't:

| commit | what | touches |
|---|---|---|
| a62b17b | the far single cards the right size (impostor.py) | the props' generated meshes |
| a62b17b | the tiers recalibrated to the pop-in test (`include/fishmod_lod_pop.h`): under water at full distance; reed_clump, cattails, kelp_frond near to 1200 / 1200 / 900 units; each kind's near distance its own | fishmod_scatter.c |
| a62b17b | each kind's draw distance crosses the surface with the haze, over its ten frames | fishmod_scatter.c |
| a62b17b | the water fog's key in whole units of camera height from 40 up | fishmod_scatter.c |
| a62b17b | the pop-in test: `--script pop_ab`, `pop_ab_cal`, `fishmod/tools/pop_ab_capture.py`, `flip_metric.py`; `popin_pier` | fishmod_debug.c, fishmod.c, fishmod.h (debug builds only), tools, scripts |
| 9018b6a, 9a9ce46 | heroes and the crowd: the full rig for the four fish nearest the camera only | fishmod_manager.c |

## Checked here, on this branch

- **Collision** (merged into main at rc1, unchanged since): `tools/bgcheck_check/run.sh --extracted ntsc-1.2-colpack
  --base a8db7be`: 1,818,000 questions over the 101 scenes with the lake's props, no difference.
- **The kept commands with rc1's features** (the water fog per entry, the plants bending as ropes and stalks, in
  bands when touched): the game's own check (SCATTER_VERIFY: every kept frame's commands made again and compared),
  with the fog's key exact: the bench's route 34,951 checks, `bend_kelp` 2,282, `touch_test` 3,753, none different.
  (With the key as it ships, 20 of 43,122 on the route differ: one fog number in its last digit, the camera's height
  having moved by less than the key sees.)
- **The recalibration on main's code**: `pop_ab_cal`, eleven stops (`pop-ab-merge-ready.md`): every kind at LPIPS 1.5
  or under (the line is 2.0); the surface's crossing 0.8 or under.
- **The picture**: the tour's 22 stops, side by side with the last build of claude/rc1-headroom: the same plants (and
  main's new lake surface).

## Not done, and said plainly

- **The heroes' sheen** (a specular highlight on the nearest fish) was tried and taken out: it didn't draw, and the
  fish are too small on screen for it to matter. Heroes and the crowd is the rig's cap alone.
- **The RDP's side** of the recalibration (more models drawn under water between 250 and 500 units; reeds, cattails
  and kelp on their cut cards further) and of the far cards now drawing: ares doesn't time it. The bench pair below
  is for that.
- **MM and vanilla OoT**: their collision branches are as they were (claude/collision-opt-vanilla; mm's
  claude/laughing-ptolemy-pqsgmc has the first round only). Nothing here touches them.

## Patches

- `fish-oot-merge-ready-bench.bps`: this branch, `--script pcprof_frames --hw --texpack --colpack --meshpack --gcc-all`.
- `fish-oot-main-bench-rebuilt.bps`: main at 9d1d2b6, the same command on the same machine: the baseline for it.
- The queue (`fishmod/pc/morning_queue.txt`): main, ready, main, ready, 12 minutes each.
- `fish-oot-merge-ready-test.bps`: this branch as the test build (`--testmode`).

## To merge

One merge of this branch into main: it is main plus its own commits and one merge of main into it (9a9ce46). The only file both have been
changing is fishmod_scatter.c; this branch's changes to it are the recalibration's hooks in Scatter_Build and the
fog key, on top of main's version of it as of 9d1d2b6. fishmod_rig.c and z_en_fishmod.h are main's, unchanged.
