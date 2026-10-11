# Roadmap

Kokiri Forest comes first: fishing there has to be solid before other areas get fish. The goals below are for later
and are not being worked on yet.

## Rule: no performance cost

Visual extras only go in if they don't cost performance. Each one gets measured in ares with the debug timing line
(RSP/RDP, CPU update/draw, frame time) against the build without it, and it is dropped if the frame gets slower.

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

### Optimization to-do for kokiri-test4

Planned for kokiri-test4 if test 3 has no major issues. Each one is measured (debug timing line, test-mode FPS
counter, Kokiri vs Lake Hylia) and dropped if it doesn't help.

1. **Billboards for distant fish:** past some distance, draw a fish as a camera-facing quad or an 8-vertex
   "stretched hexagon" in solid vertex colours, with no texture: no texture loads (TMEM). It must be opaque or
   alpha-test only; blended edges would bring the fill cost back.
2. **More AI throttling by distance:** far fish already think every other frame (build B). Add further steps:
   every 4th frame, then every 8th, for fish further out.
3. **Sub-biome unloading:** unload fish (keeping each zone's count) when their zone is far from Link, and stop
   processing them (the "fish near Link only" part of fishmod/LAKE_HYLIA.md).
4. **More matrix skipping:** extend the limb shortcuts and pose caching (builds A/B) to every fish that isn't
   changing shape, not just distant ones.
5. **Folded-polynomial sin/cos:** Kaze's polynomial over 1/8 of the wave (symmetry for the rest, sqrtf for the
   other half) in place of the sine table, behind a flag. Measure first: it only helps if trig cache misses
   show up in CPU time.
6. **Techniques from Majora's Mask:** borrow MM engine optimizations where they apply (MM runs a busier world on
   the same hardware; its decomp, zeldaret/mm, is the reference).
7. **Expansion Pak:** OoT doesn't use the extra 4 MB on its own. Change the memory setup to use it when present
   (bigger arenas and caches, e.g. the water floor cache), keeping the game working without it if possible.
8. **F3DEX3 on hardware:** the build flag exists (`fishmod/build.sh --f3dex3`) but has only been tested in ares.
   Test it on the flashcart.

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
- **Fireflies:** a new bait. Small glowing bugs (fairy-style glow without the wings) that can be caught in a bottle.
- **Strength and casting:** the strength upgrades (Goron's Bracelet, Silver and Golden Gauntlets) raise the
  longest possible cast.
- **Lake Hylia:** planning has started (see [LAKE_HYLIA.md](LAKE_HYLIA.md)): sub-biomes that fish are anchored
  to (shallows, weed beds, a dry dock, drop-offs, deep water, the inflow), the performance benchmark plan, and
  ideas for the other areas.
- **Size of water:** choose species to fit the size of the water body (big fish need big water). Kokiri's
  larger fish are fine for testing.
- **Line tension:** it feels like a hair trigger. Re-check after the new fight choreography (it may already feel
  better) and in larger waters than Kokiri Forest. If it still needs a change, scale the tension by the fish's
  weight.

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
Lake Hylia.

### 3. Expanded Fishing Pond

A much larger version of the Fishing Pond (a new scene, likely built with Fast64 in Blender).

### 4. Lake Hylia lab aquarium

Much later: turn the Lake Hylia laboratory into an aquarium where you can keep your favourite catches, like a
living version of the fishing journal.

### 5. New items: bug net, fishing rod, pictograph box, journal

Last in line: after the sub-biomes, foliage for small bodies of water, the boat and the aquarium.

Four new items, which grows the item page from 6 × 4 (24) to 7 × 4 (28): the same four rows, one more column, so
the cursor math only changes its column count and icons keep their height (about 86% width). The layout stays
in the spirit of vanilla (ammo top left, arrows and tools right, bottles along the bottom).

- **Fishing rod:** a real item to equip on a C button. Given before the game starts by default; later the
  Kokiri tutorial gives it (shoot down the hive for the rod, learn that larvae make bait). The Fishing Pond uses
  your rod.
- **Bug net:** catch bugs (and fireflies) for the compendium and for bait.
- **Pictograph box:** in the style of MM's: first-person photos, logging every NPC, enemy type and the passive
  wildlife that's in the shot (actors on screen and close enough when the shutter fires), plus a selfie
  camera with a caught fish. Only a few photos are stored (each is about 9 KB at MM's 160 × 112, 4-bit); entries
  are "logged" flags shown with the live model or a pre-made image.
  - MM's code is fine to learn from (zeldaret/mm). MM's assets (the box model, its icon, the viewfinder
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
