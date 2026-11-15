# frames21

**`fish-oot-frames21.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --meshpack --gcc-all`): frames20
fixed and made a little cheaper:

- **The shore's grass** in its colours again: frames20 made two of Lake Hylia's textures grey, taking both for
  caustics because they're blended the same way; the second (00018CB8) is grass. Only the caustic (000170B8) is grey
  now, and only its three lists are faded with depth.
- **Plants making way** check only the kinds that can (swaying plants, floating ones) in the chunks within reach of
  Link: frames20 went through every prop of the nine chunks round him every frame.
- The terrain and sky tests (FISHMOD_BENCH_TERRAIN_RA, FISHMOD_BENCH_SKYLAST) are in, switched off: the game as
  frames20 otherwise.

**frames20 on the console** (run 20261115-0501, one lap, against frames19's): hitches 20% -> 29% of pictures, 18.8 ->
18.4 fps, RDP in slow frames 48.2 -> 49.1 ms, CPU draw 23.6 -> 24.8 ms, RSP graphics 27.0 -> 28.5 ms: the shallow
rocks' caustics (2-cycle), their vertices (per face: 2,608) and the touch check's scan.
