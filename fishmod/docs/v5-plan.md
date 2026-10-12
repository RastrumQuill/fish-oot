# Test5 plan: biomes, time and behaviour

Goal: Lake Hylia becomes a real fishing spot. Its fish live in places (weed beds, drop-offs, the dock, deep water)
and at times (dawn, night, rain), they behave like fish (notice, follow, inspect, strike or refuse), and the gear
reaches them (float depth, lures that move). Days are long enough to sit and watch it happen.

Items (numbers from the roadmap): 1 deep water fishing, 2 longer days, 3 cheaper AI, 5 lure behaviour, 6 visible
reactions, 7 bite rhythm, 8 line tension, 10 individual looks, 11 animation polish, 12 AI v2, 13 Lake Hylia
sub-biomes.

Sources: [research/biomes-time-behaviour.md](research/biomes-time-behaviour.md) (new for this plan), plus
[research/fish-ai.md](research/fish-ai.md), [research/fishing-feel.md](research/fishing-feel.md),
[lake-hylia.md](lake-hylia.md).

## Why Lake Hylia doesn't work today

Read from the code (`z_en_fishmod.c`, `fishmod_rod.c`):

- The bait hangs 14 units under the bobber and fish come to `waterY - 10`. There is no way to fish deeper.
- A fish notices the bobber by **horizontal** distance only, then has 200 frames to get there while it can climb
  0.8 units a frame. A fish 300 units down gives up before it arrives.
- Fish pick a depth once from their species band (surface / mid / bottom) and keep it all day. Nothing makes them
  rise to feed.

So it's both: the gear can't go down, and the fish never come up.

## The performance rule for this version

The fish AI is the mod's biggest cost (lab tank, 36 fish, debug build: AI 5.7 ms, water queries 2.8 ms of that,
swim 0.9 ms; the frame is CPU-bound). Everything in this version is AI and assets, so:

- **Item 3 goes first and pays for the rest.** Target: halve the AI cost in the lab tank before adding behaviour.
- **Every feature is measured** on the lab bench (36 and 90 fish) and a new Lake Hylia bench, against the build
  before it, with the profiler (`read_stats.py --prof`). A feature that costs more than its budget below is
  redone or dropped.
- **Budget for the finished AI v2:** no more than today's AI cost per fish in the lab tank. New behaviour is paid for by
  phase 1's savings.
- Props and effects are drawn by the fish manager in batches (material once per type), with a draw distance, and
  each prop type is measured.

## Phase 0: benches

- **Lake Hylia bench** (`bench_lake` exists, it looks from the overlook): add a lakeside bench with Link on the
  dock, a full roster around him, and the rod cast deep. It's the scene this version is for.
- Record the baseline for the lab (36, 90) and the lake benches before any change.

## Phase 1: cheaper AI (item 3)

Where the time goes today, per think: each water query first loops over the scene's water boxes, then the grid; a
fish blocked by a bank tries up to 10 headings × 2 queries; the notice check traces 5 queries; wandering picks
targets with up to 16; floors near banks need raycasts; distances use `sqrtf` 4–7 times a fish a frame.

1. **A baked habitat map per scene (the big one).** `fishmod/tools/water_map.py` already reads the collision and
   computes depth. Make it write, per fishing scene, a small grid packed into the ROM:
   - floor height (1 byte, 5-unit steps above the scene's lowest floor), so depth = water surface − floor, and the
     drained Lake Hylia (adult, before the Water Temple) works off the same map;
   - **distance to the shore** (4 bits), and from it the direction to open water (the gradient of 4 neighbours);
   - habitat id (4 bits, phase 3).

   At 64-unit cells Lake Hylia's main water is about 120 × 105 cells, about 25 KB (fits easily with the Expansion
   Pak; DMA'd when the scene loads). No raycasts at runtime, and "which way is open water" becomes one lookup
   instead of up to 20 queries. The exact-floor cache near banks stays for fine detail where a fish touches the
   shore.
2. **Water surface cached per fish** (its water box), not searched per query.
3. **Squared distances** in all the "is it within X" checks (no `sqrtf`).
4. **Decide slowly, steer every frame:** decisions (targets, notice, mood) at 4–10 Hz staggered across fish; the
   per-frame part only steers toward the current target (today's glide already does this for far fish).
5. Notice checks use the map's depth instead of tracing the path.

Gate: AI time in the lab tank at most half of today's; no fish stuck on banks in Kokiri or the lake (the unstick
counter in the profiler).

## Phase 2: time, light and weather (item 2)

**Day length.** `dayTime += gTimeSpeed` each frame (×2 at night); Lake Hylia runs at speed 10, about 4 minutes a day.
`gTimeSpeed` is an integer, so the change goes into the addition: a fractional accumulator scales the step. Nothing
that sets or reads `gTimeSpeed` changes (Sun's Song's 400, the drawbridge's `== 50` check, the owl's 0, the
pond's 1), so those keep working. Sun's Song (speed ≥ 400) and the Fishing Pond are left unscaled.

| Option | Scale | Day | Night | Full day |
|---|---|---|---|---|
| 20 min, night stays faster | 1/5 | 13.1 min | 7.1 min | 20.2 min |
| 24 min, night stays faster | 1/6 | 15.7 min | 8.5 min | 24.2 min |
| 24 min, night as slow as day (like MM) | ~0.23 | 11.5 min | 12.5 min | 24 min |

Event windows are in game hours, so they get longer in real time: Dampé's 18:00–21:00 becomes about 3 real minutes
instead of 30 seconds.

To check: Dampé, the drawbridge, Sun's Song, the scarecrow and Poe events, night-only enemies, cutscenes that set
the time (`z_demo.c`), and the Fishing Pond's own windows (it already runs at speed 1).

**Kokiri Forest's clock is stopped in vanilla** (speed 0), so time-of-day fish can't work there as it is (see open
questions).

**Light.** One value per frame, 0–1, from the time of day (dawn and dusk ramps), clouds and rain. It drives fish
depth, boldness and how far they see (the evidence-backed lever; pressure stays out).

**Weather.** A daily roll per scene, seeded like the roster (same day, same weather): clear, overcast or rain.
Rain uses the engine's storm request (as the Fishing Pond does) and stays within Lake Hylia's light configs 0–2.
Rain rings on the water come from a small batched ripple pool of our own (the pond does the same; EffectSs has
only 85 slots for the whole game).

## Phase 3: biomes (item 13)

**Zones.** Some come from the map automatically, others are drawn by hand as circles and capsules in a per-scene
JSON file (baked into the habitat ids):

| Zone | Source | Who lives there |
|---|---|---|
| Shallows / beach | depth < 50 | minnow schools by day, carp, bass at dawn |
| Weed beds | hand-placed (north shore, island) | bass, pike in ambush, greengill |
| Lily field | hand-placed | bass, frogs on the surface (popper and frog lure water) |
| Drop-off | where depth changes fast | bass and pike holding, big fish |
| Deep basin | depth > 300 | catfish and carp by day, big fish |
| Dock / bridge pilings | hand-placed (lab, bridges) | bass and catfish in the shade |
| Inflow (Gerudo river) | the river boxes | current-loving fish, active at dawn |
| Island shallows | hand-placed | loach at night, carp |

The inflow is the Gerudo Valley river (water boxes 0 and 1), not Zora's Fountain as [lake-hylia.md](lake-hylia.md)
said.

**Species data.** Each species gets:
- a **4-period activity table** (dawn, day, dusk, night, 2 bits each; the Wurm Online layout) instead of today's
  on/off mask;
- a **depth range** in units plus a **rise rule**: how far up it comes when active and the light is low (bass up at
  dawn and dusk, catfish and loach at night, minnow schools up at dusk);
- **habitat weights** per zone;
- for carp, a per-fish offset to the clock (individuals differ).

Values come from the research table (bass dawn/dusk + a midday bump, pike twilight, catfish and loach night, carp
per fish, minnows by light). The roster places each day's fish in zones by habitat weight.

**Props.** The Fishing Pond's props are already in memory wherever fish are (`object_fish`): reeds, lily pads,
rocks, wood posts, an unused boulder, and the gravel texture. `gameplay_keep`'s grass blades make weed clumps;
`gameplay_field_keep` (loaded in Lake Hylia) adds floating leaves. Placed per zone from the JSON, drawn by the
manager like `Fishing_DrawPondProps` (material once per type), with a draw distance. Underwater weeds are tinted
grass blades swaying with a cheap sine (no per-vertex work).

**Procedural textures** (new `gen_habitat.py`, CI4, 32×32 or smaller): sand and gravel patches, silt, weed
strands (alpha cut-out, tinted per zone), lily pad variants (flowering, yellowed), a darker "deep water" decal for
drop-off edges.

**Tells.** Things a player can read: ripples where fish feed at the surface at dawn and dusk; bubbles over
bottom-feeders (`EffectSsBubble` ripples when it surfaces); the vanilla loach's habit of rising to the surface now
and then near its home; schools flickering in the shallows.

## Phase 4: AI v2 and behaviour (items 12, 1, 6)

One piece at a time, each measured.

1. **3D heading:** yaw and pitch like vanilla's pond fish (bump and slide off walls), so a fish can swim up to the
   bait instead of easing its height.
2. **Depth over the day:** a fish's target depth comes from its species range, the light, its activity and the
   zone. Deep fish rise at dawn and dusk to feed; catfish and loach come up at night.
3. **Senses:**
   - sight: a cone in front, its range set by light (and water clarity per zone);
   - lateral line: short range all around, only for moving things (lures, a struggling fish);
   - sound: a cast's splash or Link's footsteps on the dock alert fish in a radius without telling them where.
4. **Mood:** hunger (rises over time, falls after eating), fear (Link close, splashes, a fish caught nearby) and
   a per-fish boldness from its seed (heritable boldness in Philipp 2009).
5. **The approach** (vanilla's sequence, with its numbers):
   - notice: turns toward, a flick of the tail;
   - follow: trails 1–2 body lengths behind a moving lure;
   - inspect: hovers, backs off inside half a body length plus 20, comes back;
   - then either a wind-up and charge from 0.2–1.2 body lengths (Harper & Blake), or a **refusal**: turns away
     sharply and leaves.

   Each step shows, so the player reads what the fish thinks.
6. **Learning:** a fish that was hooked and got away, or was released, gets shy of that kind of bait or lure for
   the rest of the day. Lures teach faster than live bait (Beukema 1970). A spot's fishing pressure decays over
   days. The roster is per day, so learning is per fish within a day and per spot across days.
7. **Schools:** minnows and greengills become schools: a leader and up to ~12 followers at fixed offsets, drawn as
   the pond's 4-vertex group fish or the small rig up close. No neighbour queries. They swim faster in bright water
   (so they drift to shade, Berdahl 2013). A scare spreads through the school as a wave (each member reacts after
   distance / wave speed and flees ~30° off the threat). One member is promoted to a full fish when it goes for the
   bait. This makes a lot of small fish cheap.

Gate: AI v2 in the lab tank no more than today's AI cost; at the lake, a deep fish reaches a deep bait, and fish
come up at dawn.

## Phase 5: gear and feel (items 1, 5, 7, 8)

**Float depth (item 1).** A depth setting on the rod (with the rod out, before the cast): surface, 1 ft, 3 ft,
6 ft, 12 ft, bottom. The hook hangs that far under the bobber, and the bobber sits lower with more weight. "Bottom"
lets the bait sink to the floor (catfish, loach, carp).

**Lures (item 5).** Each lure gets an action, a depth class and what it's good for:

| Lure | Action | Depth | Works on |
|---|---|---|---|
| Spinner | steady retrieve, flash and vibration (sight + lateral line) | shallow–mid | bass, pike; good in low light |
| Crankbait | dives deeper the faster you reel, floats up when you stop | mid–deep | bass |
| Popper | pops and splashes on a twitch, rests between | surface | bass, pike at dawn and dusk, calm water |
| Frog | crawls over lily pads and weeds without snagging | surface | bass, pike in cover |
| Sinking | sinks (up to 1 unit/frame), hops along the bottom | any, counted down | loach, catfish, bass; strikes on the fall |

- **Retrieve styles:** steady, twitch, stop-and-go. Many strikes come on the pause (a fish following a lure takes
  it when it stops). A sinking lure attracts while it falls, as in vanilla.
- **Lure size** limits fish size (small fish don't take big lures).
- **Models:** the pond lure body and the unused old lure from `object_fish` recoloured with prim colours, the
  sinking lure's billboard, small procedural CI4 textures for blades and bodies. No new objects.
- For testing, test mode gives all lures; how players get them comes later.

**Bite rhythm (item 7).** Bait: take, hold up to a second, spit and take again about one time in ten (the carp
study), so the bobber goes nibble, nibble, then under. Each nibble gets a small rumble (there is none today) and a
sound; the bite a bigger one. Lures: no nibbles, a hit.

**Line tension (item 8).** Re-check in the lake with big fish; scale the fish's pull and the snap point by its
weight if it still feels like a hair trigger.

## Phase 6: looks and polish (items 10, 11)

**Individual fish (item 10).** Each fish has a 32-bit seed in its roster record:
- **tint:** its own copy of the species' part colours, varied a little in hue and brightness, built once at
  spawn (56 bytes in the actor, no per-frame cost);
- **pattern offset and mirror:** a per-fish tile offset so stripes and spots land differently;
- **koi only (Sanke Carp):** a Spinda-style pattern: the seed stamps 4 blotches into a per-fish 32×32 CI8 texture at
  spawn (1 KB per koi, drawn with its own texture only up close).

All gated by measurement. The tint and offset are expected to be free; the koi texture costs one texture load per
koi.

**Animation (item 11).** Limit the catfish's turn curl (a per-model limit on the curve), speed up small fish's tail
beat (phase ×1.25 under 9 inches), check the reduced rig against the full one in the studio.

## Release

**lake-test5:** Kokiri Forest and Lake Hylia (child and adult), with the test mode and animation studio patches as
before. Release notes list the measured costs.

## Order and size

| Phase | Items | Size | Depends on |
|---|---|---|---|
| 0 Benches | – | S | – |
| 1 Cheaper AI | 3 | M–L | 0 |
| 2 Time, light, weather | 2 | M | – |
| 3 Biomes | 13 | L | 1 (the map), 2 (activity) |
| 4 AI v2 | 12, 1, 6 | L | 1, 3 |
| 5 Gear and feel | 1, 5, 7, 8 | M–L | 4 |
| 6 Looks and polish | 10, 11 | S–M | – (can be done any time) |

Phase 2 and 6 don't depend on the AI work, so they can fill gaps.

## Open questions (for the user)

1. **Day length:** 20 or 24 minutes, and should night stay faster than day (OoT) or run at the same speed (MM)?
2. **Kokiri Forest's clock** is stopped in vanilla. Let it run (so Kokiri fish change over the day), or keep Kokiri
   always daytime and make it the "simple" spot?
3. **Float depth control:** the D-pad does nothing in vanilla gameplay (test mode uses up/down for warps). D-pad
   up/down with the rod out, shown next to the bait, or a choice in the bait menu?
4. **Weather:** a random daily roll at Lake Hylia, or only rain from the Song of Storms?
5. **Adult Lake Hylia before the Water Temple** is drained to a small pool. Fish only in the river and that pool,
   or skip adult Lake Hylia until the lake is refilled?
