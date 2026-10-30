# Refactor plan

From a read-only audit of the whole mod (five parts: build switches and engine hooks; fish actor and AI; rod, items
and save; props and the tool pipeline; debug, benches and scripts). Roadmap section 3 has the goals; this is the
order of work and what the audit found. Each stage that shouldn't change behaviour is checked with the state
fingerprints (`fishmod/tools/regress.sh`, `state_hash.py`: deterministic script runs in ares must give the same
hashes before and after); stages that fix bugs or change behaviour say so and are checked by tests of their own.

## What the audit found (the headline assumptions)

- **`FISHMOD_OPT` is a performance switch that became the game.** 148 `#if`s over six levels; level 5 holds moods,
  schools, props, baked water maps, weather. Only level 5 is built now; a plain `build.sh` (level 0) builds an old
  game. About 890 lines of lower-level paths are dead.
- **`FISHMOD_HW` does almost nothing.** Three bench tags in C; the hardware thinning is applied to every build. The
  roadmap's main split (hardware vs emulator) doesn't exist yet.
- **Real bugs in the test builds:**
  - dying with the rod on B leaves B empty (the real B item is kept only in RAM);
  - once the rod has been held, bug and larva bottles are taken as bait in any scene (the rod's state isn't reset
    when it's put away), and releasing bugs on soil then fails;
  - the pause menu's heart-piece icon is garbage at 2 or 3 pieces (the larva icons replaced its entries);
  - with a larva bottle or a lure on a C button, `Player_ItemToItemAction` reads past its table every frame;
  - using a bait bottle can change a different bottle than the one pressed;
  - changing age leaves larva C buttons stale; the fight music keeps playing if the rod is put away mid-fight;
  - the save block's place depends on the build (`SCENE_ID_MAX` is 101 in release builds, 110 with debug assets);
  - benches and some debug writes land on the real save files in SRAM (bench ROMs must only run on a blank save).
- **Scene knowledge in code:** Lake Hylia special cases in the manager, scatter, bed shading, collision grid
  (`z_bgcheck.c`), fog; "a fishing scene" means "has a spawn table"; the drained lake has nowhere to live.
- **Water as one flat box per spot:** lookups take the first box under (x, z), ignore height, and silently fall back
  to slow raycasts if a baked map doesn't match.
- **Fish identity** is the index in a list built from the global random generator (which it also reseeds, disturbing
  vanilla's): anything that changes how many numbers are drawn reshuffles which fish was "kept today".
- **Everything is per frame at 20 fps;** time constants are bare frame counts.
- **Coupling:** the rod writes fifteen fields of the hooked fish; the fish reads the rod's tension; one 538-line AI
  function; one global rod, one global manager (and fish go invisible if the manager fails to spawn).
- **Constants duplicated between Python and C** (plant heights: one copy has drifted; the rock hash; draw distances;
  the LOD table indexed by position); scene paths repeated in ten tools.
- **ROM-derived content is committed** (the recoloured pond fish textures, the larva bottle icons and art): must be
  generated at build time instead.
- **Debug:** `fishmod_debug.c` is 3,900 lines of 20 jobs selected by script macros; SRAM debug records overlap each
  other with no shared map; several bench switches are dead (one A/B script compares two identical frames); the test
  build's D-pad warps aren't behind L; debug state (`gFishmodPhoto`) is read in release code.

## Stages

1. **One game, not six.** Fold `FISHMOD_OPT` 5 in and delete the lower levels; drop `FASTTRIG`, the 4-bit texture
   toggle, dead bench bits, `ZSAME`, `EXTRA_FISH`, unused tools. *Behaviour-preserving* for the current build
   (fingerprints).
2. **Items as a table.** One fishmod item table (id, slot, icon, name, age, item action, is-bottle) used by the icon
   loads, `Player_ItemToItemAction`, `gItemSlots`, bottle checks and the pause grid; the grid tables move out of the
   vanilla kaleido files. *Fixes* the heart-piece icon, the out-of-bounds reads, the stale age-swap buttons.
3. **The rod's hold on the buttons.** B stashed where vanilla stashes it (survives death, horse, the pond); rod state
   reset when it's put away, on scene load and when it's destroyed; bait used through the pressed bottle; fight music
   stopped on every exit. *Fixes* the B, bait and music bugs. Tests: scripts for death with the rod on B, a bug
   bottle outside a fishing scene, the horse.
4. **Save.** Fixed offset (scene 110), a version byte, size asserts, records keyed by stable species ids; one SRAM map
   (C header, mirrored for the Python tools); debug records moved off the save files.
5. **Scenes as data.** A scene record: rod allowed, spawns per water state (the drained lake), fishable water boxes,
   attention distances, hooks for scene quirks (the Lake Hylia gossip-stone cull, the dock, bed shading, collision
   grid size). Water lookups take (x, y, z) and the box; a fishing scene must have a baked map. Tools read the same
   scene configs.
6. **Profiles.** A hardware/emulator profile as one object: placement, thinning (hardware only), plant models, draw
   distances, budgets, F3DEX3; one generated header for C; every tool takes it; both built and checked.
7. **Fish.** Per-state AI functions; a narrow fish/rod interface (events and a line pull); time in seconds
   (`R_UPDATE_RATE`-aware); species and base-model tables for what's now in if-chains (koi patterns, catfish curl,
   texture sizes, draw offsets); stable fish ids with a private random generator; a manager that can fail without
   making fish invisible; arena budgets reported.
8. **Debug and tools.** `fishmod_debug.c` split by job; scripts as data (a descriptor plus steps); one capture/ares
   harness module and one SRAM reader; the habitat tools as one package with shared parsers and a kinds table that
   generates both the C and Python constants; ROM-derived art generated at build; test-mode controls behind one
   gate; debug reads out of release code.
9. **Notes.** One current set; old notes removed.

Stage 1 first (it shrinks everything after it), then 2-4 (bugs the player meets), then 5-8, each in its own commits.

## Decisions (made)

- **The rod is a C item only.** L no longer puts it on B; L is free for the debug gate (test builds only), and the
  D-pad can become extra C buttons later. (This also removes the B-button stash bugs at their root.)
- **The save block moves to scene 110**, and a save from the old builds has its block moved there once on load.
- **Dropped:** the stop and roam benches, and the 4-bit prop texture toggle. Kept: the play bench, the area bench, the
  play log.
