# bgcheck_check: the collision code against itself, bit for bit

Builds the collision code twice, as it is at a git revision (default `HEAD`) and as it is in the working tree, and
asks both the same questions on the collision of every scene of a version, with moving platforms. Every answer must
be the same, bit for bit: each query's return value, its out positions (as raw float bits), the poly it found (by
index) and the bgId.

```
tools/bgcheck_check/run.sh [--version VERSION] [--extracted NAME] [--base REV] [--keep DIR] \
    [--frames N] [--queries N] [--scene NAME] [--seed N] [--ram 4|8] [--mem]
```

- `VERSION`: an extracted version (`make setup VERSION=...` first); default `ntsc-1.2`.
- `--frames`, `--queries`: frames per scene, queries per frame (default 60 and 300: about 1.8 million queries over the
  101 scenes).
- `--ram 4|8`: the console's memory. With 4 MB (no Expansion Pak), the scenes whose tables don't fit in their node
  table run without them, so both ways of every search are tested.
- `--mem`: prints each scene's table size against its node table's spare room.
- `--keep DIR` keeps the build; then `DIR/driver DIR/libbase.so DIR/libopt.so [args]` reruns it at once.

Needs `gcc-multilib` (the code is built 32-bit, as the game's `u32` is an `unsigned long`), and builds with the
game's float rules: single precision (SSE, not the x87), no fused multiply-adds (the VR4300 has none), no
reassociation (`-ffast-math -fno-unsafe-math-optimizations`, as the game's GCC builds).

## What it asks

The five entry points every public collision call goes through: `BgCheck_RaycastDownImpl` (floor),
`BgCheck_CheckWallImpl`, `BgCheck_CheckCeilingImpl`, `BgCheck_CheckLineImpl` and `BgCheck_SphVsFirstPolyImpl`, with
every flag combination, from points on and near the scene's triangles, exactly on and just off their corners,
anywhere in the scene's box, and around the platforms. Some questions are asked again exactly (an actor standing
still), some from the same place with other flags or sizes (what the answer caches key on), some a small step on (an
actor walking). Up to 8 platforms per scene (objects' collision from the version's own files) stand still, move, turn,
or are removed and replaced in their slots.

## Checking the check

A test that never fails proves nothing. Each optimization was broken on purpose, once (a `<` made `<=`, a field left
out of a cache's key, a flag left out of a skip mask, the line-test stamp not advanced, the lists put out of order,
...), and the test reported mismatches for every one.
