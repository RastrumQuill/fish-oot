# fish-oot

A Twilight Princess-style overworld fishing mod for Ocarina of Time (N64, NTSC 1.2 US),
built on the [zeldaret/oot](https://github.com/zeldaret/oot) decompilation.

The repository is the decomp (imported unmodified in the first commit) with the mod's
changes on top. The release is a `.bps` patch you apply to your own ROM. ROMs are never
committed.

## Playing

1. Get `fish-oot.bps` from a release.
2. Patch an **NTSC 1.2 (US)** ROM in big-endian `.z64` byte order with
   [Flips](https://github.com/Alcaro/Flips) or the [online patcher](https://www.marcrobledo.com/RomPatcher.js/).
   - Expected input MD5: `57a9719ad547c516342e1a15d5c28c3d`.
   - If your ROM is `.v64`/`.n64`, convert it first, e.g. `python3 fishmod/tools/to_z64.py in.v64 out.z64`.
3. Play on an emulator (Ares, Project64, Parallel Launcher) or on real hardware with a flashcart.

## Building

Follow the decomp's [README](../README.md) setup for `ntsc-1.2` (Linux or WSL), then:

```bash
cp /path/to/your/rom baseroms/ntsc-1.2/baserom.z64   # or .v64/.n64
make setup VERSION=ntsc-1.2
fishmod/build.sh
```

This writes `fishmod/out/fish-oot.z64` and `fishmod/out/fish-oot.bps`. It needs
[Flips](https://github.com/Alcaro/Flips) on `PATH` (or `FLIPS=/path/to/flips`).

To check the toolchain on an unmodified tree, `make VERSION=ntsc-1.2 REGION=US rom compress`
should end with `oot-ntsc-1.2-compressed.z64: OK`.

## Target hardware

Real N64 without the Expansion Pak (4 MB RAM) is the target. Memory and object budgets are
planned for that.

## Plan

1. Build and patch tooling *(done)*
2. Fishing rod usable in the overworld, obtained early in Kokiri Forest
3. Overworld fish actor with one species in Kokiri water, catchable and bottleable
4. New bottled-fish items and prices by rarity
5. Species, sizes and spawns in every body of water; larvae from hives; bait and lures
6. Fishing log in the pause menu
7. Species looks (scaled, recolored and patterned existing models, mini Jabu)
8. Stretch goal: expanded fishing hole

## Technical notes

- The rod, line, lure and hook physics and drawing live in the pond actor's owner mode
  (`src/overlays/actors/ovl_Fishing/z_fishing.c`: `Fishing_UpdateLure`, `Fishing_DrawRod`,
  `Fishing_DrawLureAndLine`). They are tied to the pond (owner NPC, props, fixed
  coordinates), so they'll be split out into a standalone rod actor.
- The player side (`ovl_player_actor/z_player.c`) is mostly generic. Casting is gated on
  pond-specific checks (e.g. `world.pos.z > 1300.0f`) and HUD handling in
  `src/code/z_parameter.c` is pond-only.
- `object_fish` (~87 KB) contains the rod, lure, bass and loach models. It's the shared
  asset for overworld fishing.
