# rc2: the new base

Everything merged as of 17 November (commit 2d6f39f and the patches' own commit):

- **rc1** and since: the props' and fish's CPU work (sets kept longer, a frame's commands reused, materials as lists).
- **The lake's water**: vanilla's texture redrawn at twice the texels as two scrolling layers, its colours fitted to
  vanilla's as the game shows it, lit as the RSP lights vanilla's (the sun's angle, nothing from below); vanilla's
  opacity (160, 102 on the outflow). The water fog's colour 75% toward vanilla's water, the same above and under
  the surface; fish under the surface in the water fog.
- **The collision lead's pop-in work** (claude/rc1-merge-ready): the far single cards the right size, the tiers
  recalibrated to the pop-in test, the draw distances crossing the surface with the haze, the full rig for the four
  fish nearest the camera.
- **The audio lead's work** (claude/sleepy-faraday-ve9g2u): the driver pinned in the caches and its code in the order
  an update runs it; with `--expak` only, updates built ahead and tasks sent early (the same sound: the audio check
  on the lake's route and the tour of the game).

## Patches

- `fish-oot-rc2.bps`: to play (`fishmod/build.sh --hw --texpack --colpack --meshpack --gcc-all`).
- `fish-oot-rc2-test.bps`: the same with `--testmode`.
- `fish-oot-rc2-bench.bps`: the frame-log bench (`--script pcprof_frames`, the same flags): the base later builds are
  measured against.
- `fish-oot-rc2-expak-bench.bps`: the bench with `--expak` (**needs the Expansion Pak**: the audio's updates built
  ahead and its tasks sent early).

## The queue

rc2's bench against its `--expak` bench, A B A B, 12 minutes each, Expansion Pak in: the base's numbers, and whether
`--expak` goes into it.

Checked here: all four build and their patches apply; both benches run the lake's route in ares.
