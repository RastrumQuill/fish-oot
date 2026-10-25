# Roadmap

Kokiri Forest comes first: fishing there has to be solid before other areas get fish. "The order from here" is the
plan after test 5; the sections after it are ideas for later, not being worked on yet.

## Rule: no performance cost

Visual extras only go in if they don't cost performance. Each one gets measured in ares with the debug timing line
(RSP/RDP, CPU update/draw, frame time) against the build without it, and it is dropped if the frame gets slower.

## Test5: biomes, time and behaviour

The plan is in [v5-plan.md](v5-plan.md). Items:

1. **Deep water fishing (Lake Hylia).** Fishing only works in shallow water: fish keep deep in the lake and never come
   near a bait dangling about 4 inches under the bobber. Gear (float depth, sinking baits and lures) and fish that
   rise to feed (by time of day, weather, species).
2. **Longer days.** OoT's day runs about 5x faster than MM's (MM: 18 min 26 s). Aim for a 20 or 24 minute day, so
   hanging out at a spot to meet its different fish by time and weather is possible (and it makes Dampé easier).
3. **Cheaper AI hot path**, to pay for the rest.
6. **Visible reactions:** notice, follow, inspect, strike.
7. **Bite rhythm:** nibble, nibble, bite on the bobber, with sound and rumble.
8. **Line tension re-check** in bigger water, scaled by the fish's weight if needed.
10. **Individual fish looks:** per-fish texture offsets and palettes.
11. **Animation polish:** catfish curl on sharp turns, small-fish swim speed.
12. **AI v2:** heading with pitch, schools for small species, habitats as shapes, senses and mood.
13. **Lake Hylia** sub-biomes with their own fish.

Notes only, not built in test5: lures, sinkers and the sinking lure (item 5), hookshot fishing.

## The order from here

1. **Finish test 5** (the items above, the test 5 feedback, the lake at 20 fps on the console).
2. **Before the refactor: lures, bottles, fish as items.**
3. **The refactor.**
4. **Content:** fish and foliage in Hyrule's other waters.
5. **Quests**, once everything looks and feels good.

### 2. Before the refactor: lures, bottles, fish as items
- **Lures (and the sinker) as a C-button item tied to the rod**, the way the magic arrows go with the bow.
- **The coral hook** as an upgrade to the rod (like Hookshot to Longshot), not a separate item.
- **The item grid redone completely:** the rod, lures and sinker break the vanilla grid, so everything shrinks in
  proportion to make room. **More bottles** fill the gaps that leaves (how many isn't fixed: they're there to fill
  the voids).
- **Fish as real items:** a bottled fish shows its species and size in the inventory, and the buyer you sell bottled
  things to says what he's getting. Never just "fish" any more: size and species matter. (Selling stays a small
  extra: the economy is broken enough already. One fish at a time is fine.)
- **Fishing quality of life:**
  - **"New record!"** when a catch beats your best for its species.
  - **Depth handled for you:** the float and the sinker settle at a sane depth for where the bait lands (from the
    water's depth there and what lives at that spot), and fish look further up and down for food, so depth matters
    less than it does now. Not a setting to remember: the C buttons pick the lure, the game picks the depth.
  - **The time of day without a big clock** (ideas, none chosen): a small sun/moon mark only while the rod is out; or
    on the pause screen only; or in the fishing log's notes ("bites best at dusk").
  - **The fishing log from the pause menu: A on the rod.** On the item page A does nothing in vanilla (C assigns,
    Z/R turn the page; A only equips on the equipment page and plays a song on the quest page), so A on the rod opens
    the log (catches, records, species seen), the way A on a song shows it. A on a bottled fish could show that
    fish (species, size, where it was caught) the same way.

### 3. The refactor
Mostly consolidation: keep what works, as one system, instead of the "million compiler flags" way it grew.

**This mod may become one of three parts.** It has to stay manageable as one module among others:
- its own folders (code, data, tools, docs), with vanilla files touched only at a few clearly marked hooks, each a
  one-line call into the mod;
- its own block of the save, its own item and flag numbers taken from one table, so another part can claim others
  without clashing;
- its own build settings in one place, built alone or together with the other parts;
- shared pieces (the pause-menu grid, the save framework, the D-pad C buttons, the build profiles) written as
  framework the other parts use too, not inside the fishing code.
- **One framework** for everything (settings, data, tools), instead of a flag per experiment; everything not used
  any more removed.
- **Notes cleaned up** a lot: one current set, older notes removed.
- **Two versions, as a first-class split** (one build profile, not scattered `FISHMOD_HW` checks):
  - **Original hardware** (`--hw`): the constraining platform, so the one the work is tuned on: thinned beds, fewer
    cards, the tall plants as accents, the coloured bed (F3DEX3), a solid 20 fps on the console.
  - **Emulator** (the definitive version): lusher, with the tall kelp forests and plants with more cards, since
    emulators don't pay the RDP's fill and per-triangle costs. Its own placement, density and draw distances; the
    same game otherwise.
  The profile picks the placement data, plant models, draw distances and budgets together; each tool (gen_habitat,
  gen_scatter, thin_beds, the LOD tables) takes it; both versions are built and checked every release.
- **Code to tidy:** fishmod_scatter.c and fishmod_debug.c have grown large (split by job); the bench, tour and capture
  scripts share a lot; the habitat tools (gen_scatter, thin_beds, section_budget, shot_search, rdp_estimate) load
  each other ad hoc; experiments left switches behind (FISHMOD_BEDSKIN, the bench modes): keep or drop each.
- **Quality of life:**
  - the D-pad as more C buttons (bottles, boots, tunics, swords: test 5 feedback 6);
  - faster text: hold B to fill a box at once, and skip text already read (item gets, repeated owl talk);
  - strength scales Link's strength everywhere, and more than now: casting distance (50% further across the upgrades
    isn't nearly enough: Link throws stone pillars at his strongest) and how fast he pushes and pulls blocks;
  - others to be found.
- **Debug only in test builds, behind holding L** (an unused button) plus the D-pad, so the D-pad is free for the C
  buttons. The release builds (hardware and emulator) have no debug features.
- **A bigger save, verified:** the mod's data (fish records, the log, the lab's tank, new items) in a larger save
  (flashcarts and emulators take it). Each part gets a version and a checksum; a part that fails is taken from the
  backup copy if that one is good, and otherwise loaded anyway with a warning, never wiped (vanilla deletes a save
  whose checksum fails). Someone editing the file by hand isn't punished: a small tool recomputes the checksums.

### 4. Content
Fish and foliage in every decent-sized natural water (not, say, Kakariko's well):
- Hyrule Field's waters; inside the Great Deku Tree; Kokiri Forest and the Lost Woods.
- Zora's River at the Zora's Domain entrance; inside Zora's Domain (fish, little foliage); the region behind King
  Zora.
- Gerudo Valley; the desert oasis; the Hyrule Castle moat; anywhere else with a natural water of size.
Also:
- **The beehive in Kokiri Forest** moves somewhere that makes more sense for a hive.
- **Drinking larvae** for a very small heal, as in Twilight Princess (the drinking animation).

### 5. Quests (ideas so far; more needed)
1. **The rod:** a new Kokiri gives Link the fishing rod with a bobber, for shooting down the beehive, saying he uses
   the larvae for bait. Link has no bottle yet, but has just finished the first dungeon, so the first bottle is
   close, and he realises it can hold bait.
2. **The Lake Hylia lab:** give your fish to the lab, which keeps them in its water (the aquarium moonshot); enough
   variety earns something. The scientist already keeps flags on what you do outside the conversation: what he says
   depends on how the tank was when you last spoke and how it is now (how many fish, how many species).
3. **The sinking lure at the Fishing Pond:** the owner is still upset, but in effect tells you to take it with you.
4. **Fishing-related bottle quests.**
5. **Ways to get the other lures and sinkers.**
6. **Bait in the Fishing Pond's quest.**

## Planned

### Individual fish looks

Make every fish look a little different from others of its species, cheapest options first:

1. **Per-fish texture offset/flip:** shift where the pattern lands on each fish. No memory, almost no time.
2. **Per-fish palettes:** switch the patterns to palette (CI4/CI8) textures, so fish share a pattern but each
   has its own colors (about 32 bytes per fish). It also makes textures smaller than today's RGBA16 ones, which
   means less memory traffic.
3. **Blending two textures on the RDP** per fish (needs the RDP's slower two-pass mode; only if measurements say
   it's free).
4. **A texture generated per fish at spawn** (4–8 KB of RAM per fish, CPU once at spawn, never per frame), for
   species where individuality is the point, like koi.

Options 1 and 2 are the target. 3 and 4 only if they pass the performance rule.

## Engine and performance

Target hardware is a real N64 with a flashcart (EverDrive, SummerCart64), so a bigger save than vanilla's is
fine, and the Expansion Pak (8 MB) may be used. Lake Hylia and the long-term goals below need a lot of headroom.

### kokiri-test4 optimizations (status)

1. **Billboards for distant fish:** done (opt3).
2. **More AI throttling by distance:** done (opt3: every 2nd/4th/8th frame, dormant past 1600 units).
3. **Sub-biome unloading:** done as the fish manager's roster (opt4): fish exist only near their homes.
4. **More matrix skipping:** done as fish rigs (opt4): only moving limbs get matrices.
5. **Folded-polynomial sin/cos:** built behind `--fasttrig`; a small win only, not in the releases.
6. **Techniques from Majora's Mask:** not started.
7. **Expansion Pak:** done (opt3): the extra 4 MB holds the framebuffers, a bigger scene and the water cache.
8. **F3DEX3 on hardware:** the only flag that helps fish up close in ares; still to be tested on a flashcart.

### Measured (kokiri-test4 work, ares, `fishmod/tools/bench_matrix.sh`)

CPU numbers only: ares doesn't time the RSP or RDP accurately, and its audio time reads 0, so render and sound time
wait for the hardware readout. Every variant looked right (no broken graphics).

| Change | Where it shows | Result |
|---|---|---|
| Distance AI levels + dormant far fish (opt3, items 2–3) | Lake Hylia overlook | fish update 9.4 → **0.07 ms**, CPU update 18.7 → 9.4 ms |
| Billboards for far fish (opt3, item 1) | Kokiri moat | fish draw 5.2 → **1.4 ms** |
| Fast trig (item 5) | Kokiri moat | fish draw 1.4 → 0.5 ms; elsewhere within noise |
| F3DEX3 (item 8) | Lab tank, 36 fish up close | fish draw 15.5 → **10.6 ms**, CPU draw 23.3 → 19.5 ms (fewer commands to build) |
| Shared water floor cache | Lab tank (no rod) | fish update 88 → **12 ms** (fish had been raycasting every query) |
| Expansion Pak memory (item 7) | — | frees room (water cache ×4, +2 MB for the scene); no speed change expected |

Up close (the lab tank: 36 fish around Link, all within LOD range) opt1 and opt3 cost the same, ~44 of the 50 ms
frame on the CPU. That case needed the fish manager and rigs (opt4, below).

**Opt level 4** (`fishmod/build.sh --opt 4`): the fish manager, fish rigs, reduced rigs, fixed whiskers and cheaper
water lookups. Profiled with `read_stats.py --prof`, ares, ms per frame, debug builds (the debug readout block,
about 5 ms in the lab tank, isn't in release builds):

| | Lab tank opt3 | Lab tank opt4 | Kokiri opt3 | Kokiri opt4 |
|---|---|---|---|---|
| Fish draw | 13.0 | **1.8** | 1.8 | 0.7 |
| — per drawn fish (close) | 1,263 µs | **~158 µs** | 1,323 µs (one fish) | ~200 µs |
| — catfish whiskers | 774 µs each | 187 µs each, close only | — | — |
| Fish update | 15.5 | 13.6 | 4.0 | 4.6* |
| — water lookup | 31.5 µs | 22.5 µs | | |
| — bank raycasts | 2.1 per frame | 0.6 per frame | | |
| CPU draw (whole frame) | 20.9 | **8.2** | 23.6 | 24.3 |
| CPU update (whole frame) | 25.5 | 24.6 | 23.0 | 19.7 |
| Worst frame | 60.0 | 53.8 | 65.8 | 52.3 |

\* Kokiri opt4 runs the day's roster: the 5 fish whose homes are far from Link are dormant (not spawned), so it's a
different set of fish from opt3's random ones; per frame the update is within noise of opt3.

Renders: the full rig is identical to the old skeleton (photo booth, every species, side and three-quarter views);
the reduced rig is identical at rest. Whiskers are now fixed to the head.

Lab tank capacity at opt4 (all fish close): about 60–65 fish at 20 fps in debug builds, about 90 in release builds
(the debug readout costs ~0.16 ms a fish). The AI itself (~170 µs per fish) is now the biggest fish cost.

**Spine rig** (after kokiri-test4: 5–7 body bones instead of 3 joints), opt4 debug builds, 400 s each, ms per frame:

| | Lab 36 fish, old rig | Lab 36, spine rig | Lab 90, old rig | Lab 90, spine rig |
|---|---|---|---|---|
| Fish draw | 2.07 | 2.74 | 5.19 | 6.77 |
| — per drawn fish (close) | 88 µs | 151 µs | 99 µs | 168 µs |
| CPU draw | 8.17 | 9.83 | 13.07 | 14.72 |
| RSP | 14.40 | 14.13 | 23.86 | 24.06 |
| Frame (avg / worst) | 50.15 / 53.85 | 50.14 / 54.29 | 62.99 / 75.67 | 63.06 / 75.08 |

The extra bones cost about 65 µs of CPU per close fish, but frame times didn't change: the AI dominates the frame.

**Test5 phase 1 so far** (opt 5: baked water floor maps, cheaper water queries, open water map, a 600-fish roster
checked and woken a slice at a time). Debug builds, ares:

| Bench | Water query | AI | Fish update avg (worst) | Frame avg / worst |
|---|---|---|---|---|
| Lab tank 36, opt4 | 19.2 µs | 5.7 ms | 13.2 ms | 50.1 / 54.3 ms |
| Lab tank 36, opt5 | 9.8 µs | 4.4 ms | 10.9 ms | 50.1 / 54.3 ms |
| Lake Hylia north beach, 30-fish roster (opt4) | 14 µs | 0.1 ms | 0.45 ms (4.0) | 50.1 / 50.3 ms |
| Lake Hylia north beach, 300-fish roster | 12 µs | 0.9 ms | 3.3 ms (10.8) | 50.1 / 50.3 ms |
| Lake Hylia north beach, 600-fish roster | 19 µs | 1.8 ms | 5.9 ms (19.6) | 50.1 / 50.3 ms |

With 600 fish in Lake Hylia about 48 are awake around Link at the north beach; the frame stays at 20 fps.

### Next

**Fish animation** (found with the animation studio, `--script anim`):
- Larger fish pinch just behind the head when they bend: the first body joint bends too hard and the head sways in
  another phase. Real fish of this shape barely bend at the front; the wave should start near zero and build toward
  the tail.
- Turning swings the body around the head like a pendulum: the model turns about its origin, which is at the head.
  Real fish pivot about a third of the way back from the snout.
- Catfish whiskers are off (too thin to see); to be redone.

**Performance** (from [research/](research/README.md)):
1. Profile cache misses (ares' built-in profiler) and check whether frames are CPU- or RCP-bound.
2. AI hot path: one look-ahead and a cached side check instead of the per-move floor probes and bank raycasts;
   squared-distance compares instead of `sqrtf`.
3. Fewer limbs for small and far fish where it doesn't show (partly done with the reduced rig).
4. A hardware timing readout in test-mode builds (render and sound time can't be measured in ares).

**AI v2** (on the fish manager): a real yaw + pitch heading like vanilla's pond fish, a central flock array with
separation for dense packing, small species as school particles promoted to full fish when they go for the bait,
habitats and obstacles as simple shapes, then senses and mood (sight cone, lateral line, scent, hunger and fear,
shyness per bait, fishing pressure per spot).

**Fishing feel** (one at a time): visible reactions (notice, follow, inspect, strike); retrieve style as the lure
skill (steady, twitch, jerk, stop-and-go; many strikes on the pause); a nibble-nibble-bite rhythm on the bobber, a
sound and rumble; fights steered against the run with rumble and rod bend as the gauge; telegraphed jumps and head
shakes; failure messages that name the cause; rare fish gated by knowledge (time, rain, spot, bait), not chance.

**Not now:** inline assembly, fixed-point matrices, resolution tricks, engine-wide collision rewrites, occlusion
planes, triangle snakes, voxel or distance-field navigation, full boids for big fish, line wear as a consumable.


### Done

- **Lines as one vertex strip:** the fishing line (optimization 1, build A and later).
- **Limb shortcuts:** limbs skip part-scale math they don't need (optimization 2, build A and later); distant
  idle fish reuse their last pose (optimization 3, build B).
- **Far fish think every other frame** (optimization 4, build B).
- **Word-sized table clears:** the collision line-test table is cleared in large chunks (optimization 7, build A
  and later).
- **F3DEX3 build flag** (ares only so far).

## Notes for later

- **More fishing spots:** some grottos have large bodies of water (at least one known), and the Water Temple could
  be interesting.
  The Great Deku Tree's water too (with grottos, the oasis, etc.).
- **Bottle fishing:** minnows and other tiny fish can be scooped up with a bottle, larger fish (probably) can't. A way to
  fish of its own, like the hookshot.
- **Fireflies:** a new bait. Small glowing bugs (fairy-style glow without the wings) that can be caught in a bottle.
- **Strength and casting:** the strength upgrades (Goron's Bracelet, Silver and Golden Gauntlets) raise the
  longest possible cast.
- **Lake Hylia:** planning has started (see [lake-hylia.md](lake-hylia.md)): sub-biomes that fish are anchored
  to (shallows, weed beds, a dry dock, drop-offs, deep water, the inflow), the performance benchmark plan, and
  ideas for the other areas.
- **Size of water:** choose species to fit the size of the water body (big fish need big water). Kokiri's
  larger fish are fine for testing.
- **Line tension:** it feels like a hair trigger. Re-check after the new fight choreography (it may already feel
  better) and in larger waters than Kokiri Forest. If it still needs a change, scale the tension by the fish's
  weight.
- **Floating leaf mats:** pennywort-style leaves sit at the surface, not on the bed. The tiling pennywort texture
  (`fishmod/art/canopy/pennywort.i4.png`) could make flat mats on the water at shallow edges: one layer, ragged
  cut-out edges, dark silhouettes with light through the gaps from below. Fish could shelter under them. The
  thick raised bed under water was dropped (two textures don't fit in texture memory at full size, and it isn't
  how these plants grow).

### Water current (not a moonshot: small)
Flowing water, where it actually flows (the Gerudo river's inflow into the lake, the rivers, Zora's River), pushes
on what's in it; still water next to it doesn't, even where both are one body of water to the game.
- **A current map:** where the water flows and which way, as a field over the water map (flow strength and
  direction per cell), baked at build time from the rivers' courses and their mouths, fading out into the lake's
  still water.
- **Fish:** hold station facing upstream in it (drifting back when they rest), shelter behind rocks and in eddies,
  and current-loving species favour it. The hook and float drift with it.
- **Plants:** in flowing cells the sway leans downstream and moves faster (a steady lean plus the wave); still
  cells keep today's sway. Per sway phase, so it costs nothing extra to draw.

## Feature complete (after normal bait fishing is sorted out)

Not moonshots: these are needed for fishmod to count as feature complete, but they wait until plain bait
fishing works well.

- **More ways to fish:**
  - **Sinking lure:** fished differently from bait (cast, let it sink, work it back); fish react to its motion
    rather than its smell.
  - **Iron Boots:** decide how they factor in (walking the bottom, reaching deep spots, fishing from underwater?).
  - **Hookshot fishing:** some fish can be caught with the hookshot instead of the rod (a joke catch, but a real one).
- **More bait:**
  - **Cucco feathers.**
  - **Fireflies:** the fairy graphic without wings, coloured yellow and flashing; caught in a bottle.
  - **Bait shop:** the Fishing Pond doubles as a bait shop, selling bait that can't be found in the overworld.
  - **Coral earring:** becomes a prize or a lost item to find, not a legendary heirloom bait.
- **Underwater creatures:** repurpose Biri, Bari, Shell Blade, Leever and Stinger (scaled and retextured) for more
  variety under the water; not all of them fishable.

## Moonshot goals

### 1. Underwater habitats

Add foliage, stones and other features to the large bodies of water, so fish have habitats to live around and
players have landmarks to navigate and read the water by.

- Large bodies of water get the most: weed beds, reeds, rocks, sunken logs and similar.
- Small bodies of water can have some foliage, but be careful not to fill them up with solid objects.

### 2. Lake Hylia boat

A small boat you can ride, working like Epona does, to reach more of Lake Hylia for fishing. It's only available at
Lake Hylia. MM's references: `ovl_Bg_Ingate` (the swamp tour boat, which Link stands on as it follows a path) and
`ovl_Obj_Boat` (the pirates' boats).

### 3. Expanded Fishing Pond

A much larger version of the Fishing Pond (a new scene, likely built with Fast64 in Blender).

### 4. Lake Hylia lab aquarium

Much later: turn the Lake Hylia laboratory into an aquarium where you can keep your favourite catches, like a
living version of the fishing journal. MM's Marine Research Lab fish (`ovl_En_Fish2`: fish in a tank that you feed
and that grow) is the closest existing thing.

### 5. New items: bug net, fishing rod, pictograph box, journal

Last in line: after the sub-biomes, foliage for small bodies of water, the boat and the aquarium.

Four new items, which grows the item page from 6 × 4 (24) to 7 × 4 (28): the same four rows, one more column, so
the cursor math only changes its column count and icons keep their height (about 86% width). The layout stays
in the spirit of vanilla (ammo top left, arrows and tools right, bottles along the bottom).

- **Fishing rod:** a real item to equip on a C button. Given before the game starts by default; later the
  Kokiri tutorial gives it (shoot down the hive for the rod, learn that larvae make bait). The Fishing Pond uses
  your rod.
- **Bug net:** catch bugs (and fireflies) for the compendium and for bait.
  - Swings with the Megaton Hammer's animations (`gPlayerAnim_link_hammer_*`). These are in the shared Link
    animation bank, not the adult's object, and store joint rotations, so they play on the child skeleton too (same
    limb layout; the Deku Stick already swings with the sword's animations as a child). The net model attaches to the
    hand like the hammer does. To check in the game: the shorter child arms (the overhead swing clipping the head),
    the net's reach for the catch collider, and the hammer's adult-only flag in the item's age table.
- **Pictograph box:** in the style of MM's: first-person photos, logging every NPC, enemy type and the passive
  wildlife that's in the shot (actors on screen and close enough when the shutter fires), plus a selfie
  camera with a caught fish. Only a few photos are stored (each is about 9 KB at MM's 160 × 112, 4-bit); entries
  are "logged" flags shown with the live model or a pre-made image.
  - MM's code is fine to learn from (zeldaret/mm). In RastrumQuill/mm: `src/code/z_snap.c` (`Snap_RecordPictographedActors` walks
    every actor list and flags what was in the shot; `Snap_ValidatePictograph` checks distance, facing, the capture
    region on screen and a line-of-sight ray, which is the logging check the box needs). MM saves to 128 KB of
    Flash (`sys_flashrom.c`, `z_sram_NES.c`): the bigger save type the compendium needs. MM's assets (the box model, its icon, the viewfinder
    graphics, the shutter sound) aren't in the OoT ROM, so they'd be made new for the mod.
  - Pre-rendered rooms (shops, the Market and such) have a fixed camera, so first-person photos don't work there,
    and some NPCs only appear in them, so a complete log would need those rooms rebuilt in 3D. Not worth solving
    now: this is out of scope for the main goal (fishing).
- **Journal / compendium:** the log of bugs caught, fish caught and characters, monsters and animals pictographed
  (the fishing journal on Link's records plaque grows into this). Could be its own pause page (like MM's mask
  page) instead of an item, if the item page gets tight.
- **Save format:** the compendium needs a bigger save than the spare scene-flag space fishmod uses now. A new
  save layout on a flashcart's bigger save type; the inventory keeps its slots, with a table for where each
  slot shows in the grid.
- Not limited to OoT's models in the long run: new models where needed.
