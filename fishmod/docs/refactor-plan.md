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
5. **Water, not scenes.** Few special cases: the game treats every body of water the same.
   - **The rod works everywhere.** Its controller comes with the rod when Link takes it out, in any scene; casting
     needs water where the bobber lands, not a "fishing scene". Water without fish just never bites.
   - **Fish** come from per-scene spawn tables (data); a scene without one has none. Region-specific work is only
     optimization: a water body's fish activate when its room loads (small spots like the Lost Woods shortcut to
     Zora's Domain), not code per scene.
   - **Plants, props and bed shading** are generated for every body of water by the same tools; Kokiri gets them too
     once the performance budget allows (that's a profile setting, stage 6, not a scene exception).
   - **What's left becomes generated data, not `if (scene == ...)`:** the collision memory and grid for scenes
     with added rock collision (from the collision generator's output), the beehive's place (a position in the
     data instead of a wall search), longer draw ranges for props seen across water (Lake Hylia's gossip stones).
   - Water lookups take (x, y, z) and the box; any scene with fish must have a baked water map (the build checks).
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

## Progress

- **1, 1b: done.** Fingerprints match the base in all three scenarios (bait, lakecast, invtest).
- **2: done.** `src/code/fishmod_items.c`; fingerprints match 1b.
- **3: done.** No L swap (the rod is a C item; a B left holding the rod by an old build is given back the sword
  outside the pond). The rod publishes its state every frame and resets it when destroyed and on scene load; bait is
  taken only with the rod in hand, from the bottle on the pressed button; the fight music stops whenever the line
  comes in without a catch. Test: `rodexit` (with `--testmode`). L is the minimap's again in every scene.
- **4: done.** The save block at scene 110 whatever the build, with a mark ('FM') and a version byte; a block without
  the mark is moved once from scene 101 (where every build so far kept it) on load. Size and place checked at compile
  time. Species have permanent save numbers (`saveId` in `species.json`; the generator refuses gaps and duplicates).
  `INV_CONTENT` goes through the mod's slot lookup (giving the rod or a lure wrote past the vanilla 24 slots, into
  the Deku Stick ammo). One SRAM map (`include/fishmod_sram.h`, read by the Python tools through `sram_map.py`); the
  records every script writes are all past the saves, and only the bench and LOD ROMs (blank-save only, as before)
  write over them. Test: `oldsave`.
- **5, the rod everywhere: done.** The rod's controller spawns in any scene when Link takes the rod out (from the
  start in scenes with fish) and goes again when he puts it away; casting needs only water. The rod's graphics are
  built into the overlay from the extracted assets, so it no longer needs object_fish; only the fish and their
  manager take that object (and scenes with fish load it). Beehive chunks (bait) now drop in any scene too. The
  HUD's rod-on-B special case is vanilla again. Test: `rodanywhere` (Zora's Domain, no fish).
- **5, Lake Hylia's special cases as data: done.** Collision memory and grid per scene in
  `fishmod_collision_scenes.h` (BgCheck applies them for any scene listed); the gossip stones' longer draw range in
  their own Init (every scene); the bed shading says which scene and which water level it was made for, and the dry
  check follows the scene's water levels (a lake refilling) instead of Lake Hylia's flag; beehive spots are a table;
  the dropped per-zone draw distances removed (game and tool).

## Decisions (made)

- **The rod is a C item only.** L no longer puts it on B; L is free for the debug gate (test builds only), and the
  D-pad can become extra C buttons later. (This also removes the B-button stash bugs at their root.)
- **The save block moves to scene 110**, and a save from the old builds has its block moved there once on load.
- **Few special cases** (stage 5): the rod works in every scene, every body of water gets plants and props alike
  (Kokiri too, once performance allows); per-region code is only for optimizations such as activating a water
  body's fish when its room loads.
- **Dropped:** the stop and roam benches, and the 4-bit prop texture toggle. Kept: the play bench, the area bench, the
  play log.
