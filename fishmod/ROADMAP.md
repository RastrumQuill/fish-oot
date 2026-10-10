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

## Notes for later

- **More fishing spots:** some grottos have large bodies of water (at least one known), and the Water Temple could
  be interesting.
- **Fireflies:** a new bait. Small glowing bugs (fairy-style glow without the wings) that can be caught in a bottle.
- **Strength and casting:** the strength upgrades (Goron's Bracelet, Silver and Golden Gauntlets) raise the
  longest possible cast.

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
