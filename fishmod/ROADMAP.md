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

Not being worked on yet. Status says what already exists (several of these went into the build A/B
optimization sets, see `fishmod/test-builds/README.md`).

### Level of detail

- **Billboards for distant fish:** past some distance, draw a fish as a camera-facing quad or an 8-vertex
  "stretched hexagon" in solid vertex colours, with no texture. No texture loads (TMEM), and opaque (not
  alpha-blended) so overlapping fish cost no extra fill. Status: to do. Note: it needs to be opaque or alpha-test
  only; blended edges would bring the fill cost back.
- **Lines as one vertex strip:** status: done for the fishing line (optimization 1, in build A and later).
- **Skipping matrix work on limbs that don't change:** status: partly done (limb shortcuts, optimization 2, in
  build A and later). Remaining: cache whole poses for idle fish (optimization 3 in build B does this for
  distant fish).

### AI and culling

- **AI throttling by distance:** status: partly done (build B: far fish think every other frame). To do: further
  steps (every 4th or 8th frame) for fish further out.
- **Sub-biome unloading:** unload fish (keeping each zone's count) when their zone is far from Link, and stop
  processing them. Status: planned as part of Lake Hylia (fishmod/LAKE_HYLIA.md, "fish near Link only").

### Memory

- **Word-sized table clears:** status: done (optimization 7: the collision line-test table is cleared in large
  chunks; in build A and later).

### Long-term

- **5×5 item grid:** change the pause menu's item grid from 4×6 to 5×5, freeing a slot so the fishing rod
  becomes a normal item to equip on a C button (today the rod has no inventory slot; L puts it on B). The grid
  is full in vanilla, so the rod needs a slot from somewhere. Note: the item cursor movement, the item icon
  layout and the "Equip to C" code all assume 4×6, so this touches a lot of the pause menu.
- **F3DEX3 (Fast3D v3) microcode:** status: done as a build flag (`fishmod/build.sh --f3dex3`). It is tested in
  ares only, not yet on hardware.
- **Folded-polynomial sin/cos:** replace the sine lookup table with Kaze's polynomial over 1/8 of the wave
  (symmetry for the rest, sqrtf for the other half), so trig doesn't miss the cache. Experimental, behind a flag,
  and measure first: it only helps if trig misses are actually showing up in the CPU time.

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
