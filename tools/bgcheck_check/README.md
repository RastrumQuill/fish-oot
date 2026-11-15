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

Once the change is committed, `HEAD` has it too: compare against the revision before it (`--base HEAD~1`, or the
commit the change was made on).

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

## Results (15 November 2026)

**Same answers.** This test, at 60 frames × 300 questions a scene: OoT ntsc-1.2 and gc-eu-mq, with and without an
Expansion Pak, two seeds each: 14.5 million questions; MM n64-us, with and without the tables, three seeds: 5.5
million; fish-oot (Lake Hylia with its props): 5.5 million. No difference.

In ares (v148), builds with the collision code counting its own time and hashing every question and answer each
frame (the same counting in both builds; a fixed random seed), the title and attract sequence with no input:

| | collision a frame | moving platforms a frame | collision log |
|---|---|---|---|
| OoT ntsc-1.2 (4,000 frames) | 6.14 → 2.39 ms (−61%) | −12% | every frame identical |
| OoT gc-eu-mq (4,000 frames) | 6.74 → 2.71 ms (−60%) | −24% | every frame identical |
| MM n64-us (8,300 frames) | 4.95 → 2.76 ms (−44%) | −35% | every frame logged (2,000) identical |

**Memory.** The tables take none of the game's: the static node table's unused end, else (OoT) the Expansion Pak's
memory a retail build leaves unused, else none. The code and static data grow by about 12 KB. In OoT that comes out of
the system heap's spare (about 100 KB over what Play asks for in ntsc-1.2): Play's memory and the actors' heap are
unchanged. In MM, Play takes the whole system heap, so the actors' heap is about 12 KB smaller (8.3 KB of code, 3.8 KB
of answer caches).
