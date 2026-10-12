# Roadmap

Kokiri Forest comes first: fishing there has to be solid before other areas get fish. The goals below are for later
and are not being worked on yet.

## Rule: no performance cost

Visual extras only go in if they don't cost performance. Each one gets measured in ares with the debug timing line
(RSP/RDP, CPU update/draw, frame time) against the build without it, and it is dropped if the frame gets slower.

## To do for test5

- **Deep water fishing (Lake Hylia).** Fishing only works in shallow water: fish keep deep in the lake and never come
  near a bait dangling about 4 inches under the bobber. Equipment, AI or both: a way to fish deep (adjustable line
  depth, sinking baits and lures), and/or fish that rise to the surface to feed (by time of day, weather, species).
- **Longer days.** OoT's day runs about 5x faster than MM's (MM: 18 min 26 s). Aim for a 20 or 24 minute day, so
  hanging out at a spot to meet its different fish by time and weather is possible (and it makes Dampé easier).

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
