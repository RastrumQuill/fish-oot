# rc1: the release candidate the next tests are based on

The landmark after the frames28 set: everything that tested well together, in this branch.

- **`fish-oot-rc1.bps`**: to play (`fishmod/build.sh --hw --texpack --colpack --meshpack --gcc-all`).
- **`fish-oot-rc1-test.bps`**: the same with `--testmode` (new files skip the intro and start with testing items).
- **`fish-oot-rc1-bench.bps`**: the frame-log bench (`--script pcprof_frames`, the same flags): what later builds are
  measured against.

What's in it beyond frames25:
- **The water fog from above in the RSP's own fog**: the camera-under-water fog by the share of the view's ray in
  the water, the lake bed in 8 depth bands (whole triangles by their middle's depth, nothing cut: meshpack's default
  now) and each prop under the water by its depth. The Lens of Truth turns it off.
- **Plants**: underwater ones bend as ropes, reeds as stalks; the reed bends (its model read where object_fish is);
  nothing placed under water pokes out of it; shoreline plants only where they'd grow; no plants left out between
  the camera and Link.
- **No depth darkening.** The Lens of Truth on the D-pad stays on.
- **The collision lead's collision** (claude/collision-opt, merged) and **the audio lead's layout changes**
  (claude/sleepy-faraday-ve9g2u, merged: the note's fields in a tick's order, the driver's hot code together).

The console, the bench build as it stands here (the frames28 set's all, 20261116-0251, two runs): 18.28 and 18.33 fps
on screen, 54.7 and 54.6 ms a frame, 30% and 29% visible hitches; slow frames: CPU update 23.8 / 23.6 ms, CPU draw
27.1 / 27.0, RSP graphics 26.2 / 26.3, RDP 46.7 / 46.8.

Checked in ares: the bench build boots and draws (frame 600, the same picture as the set's all, no prop missing).
The playable builds weren't run in ares (they have no harness to stop them at a frame).

A snapshot, not the end of anything: the audio, collision and props work goes on in their branches and comes in on
top as each change proves itself on the console. Left out: the audio driver every second refresh (worse on screen:
releases/notes-audio-laps.md); the props and fish CPU work (claude/fish-prop-headroom, on the frames20 tree: its
console test first, then brought onto this one, whose fishmod_scatter.c has changed a lot since frames20).
