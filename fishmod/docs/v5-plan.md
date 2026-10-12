# Test5 plan: biomes, time and behaviour

Goal: Lake Hylia becomes a real fishing spot. Its fish live in places (weed beds, drop-offs, the dock, deep water)
and at times (dawn, night, rain), they behave like fish (notice, follow, inspect, strike or refuse), and the
bobber reaches them (a float depth set on every cast). Days are long enough to sit and watch it happen.

Items (numbers from the roadmap): 1 deep water fishing, 2 longer days, 3 cheaper AI, 6 visible reactions, 7 bite
rhythm, 8 line tension, 10 individual looks, 11 animation polish, 12 AI v2, 13 Lake Hylia sub-biomes. Item 5 (lures,
sinkers, the sinking lure) is notes only in this version (see "Notes, not built").

**Lake Hylia is too big for 100 fish.** The goal is as much headroom as possible; the Expansion Pak may become
required instead of recommended if that buys it.

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
- **Staggered, never all at once.** Every periodic heavy job (decisions, notice checks, school updates, wake and
  sleep, prop detail, learning decay) runs in a slot from the fish's own offset, under a per-frame budget. One-off
  heavy work (rolling the day's fish, loading the map, stamping koi patterns) is spread over frames or done at scene
  load. Features are judged by the **worst frame** as well as the average.
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
6. **Most fish aren't actors.** Today each fish is a 0x400-byte actor with the engine's per-actor overhead. Fish
   that are only swimming become compact records in the fish manager (one array, a few dozen bytes each, walked in
   one loop with good cache use), drawn by the manager as now. A fish becomes a full actor only when it needs one:
   near the bobber, near Link, hooked, caught. Far fish move on a coarse schedule (whole seconds), very far ones not
   at all until the player comes near. This is what makes several hundred fish in Lake Hylia possible.

The water map is compiled at build time: the level geometry doesn't change. Water levels do (Lake Hylia drained or
full, the Water Temple's levels), which is why the map stores floor height and takes the surface from the active
water box at runtime. Moving collision (platforms, pillars) is marked by hand where it matters.

Gate: AI time in the lab tank at most half of today's; no fish stuck on banks in Kokiri or the lake (the unstick
counter in the profiler).

## Phase 2: time, light and weather (item 2)

**Day length: 20 minutes, about 12 of day and 8 of night.** `dayTime += gTimeSpeed` each frame (×2 at night);
Lake Hylia runs at speed 10, about 4 minutes a day. `gTimeSpeed` is an integer, so the change goes into the
addition: a fractional accumulator scales the step (about 0.22 by day, 0.36 by night at speed 10, so night runs
about 1.6× faster than day instead of OoT's 2×). Nothing that sets or reads `gTimeSpeed` changes (the drawbridge's
`== 50` check, the owl's 0), and Sun's Song (speed ≥ 400) is left unscaled. The Fishing Pond (speed 1, about 41
minutes) moves to the same 20-minute clock.

Event windows are in game hours, so they get longer in real time: Dampé's 18:00–21:00 becomes about 2 real minutes
instead of 30 seconds.

To check: Dampé, the drawbridge, Sun's Song, the scarecrow and Poe events, night-only enemies, cutscenes that set
the time (`z_demo.c`), and the Fishing Pond's own windows.

**Places where time is frozen** (Kokiri Forest, Castle Town, Goron City, indoors and every other place vanilla
stops the clock):
- A **hidden clock** always runs at the 20-minute rate, everywhere. It never adds days; the day counter advances
  only as it does now.
- **Barriers** come from OoT's light table (`z_kankyo.c`): **day → dusk at 16:00** and **night → dawn at 4:00**.
  Dusk into night and dawn into day are not barriers.
- **At every scene transition**, if the visible time is behind the hidden clock, it steps forward once, at most one
  game hour, and never across a barrier: it stops just before 16:00 or 4:00. Frozen places get later in the day or
  later at night but never reach dusk or dawn. The light table is flat 8:00–16:00 and 19:00–4:00, so steps there
  don't show.
- Where time flows, the visible clock also runs on its own; sunsets and sunrises only happen there. When the
  visible clock crosses a barrier the hidden clock resets to it, so nothing builds up past one phase.

**Light.** One value per frame, 0–1, from the time of day (dawn and dusk ramps), clouds and rain. It drives fish
depth, boldness and how far they see (the evidence-backed lever; pressure stays out).

**Weather.** A daily roll per scene, seeded like the roster (same day, same weather): mostly clear, sometimes
overcast, **rain rare** (about 1 day in 10). The Song of Storms starts rain regardless of the roll.
Rain uses the engine's storm request (as the Fishing Pond does) and stays within Lake Hylia's light configs 0–2.
Rain rings on the water come from a small batched ripple pool of our own (the pond does the same; EffectSs has
only 85 slots for the whole game).

## Phase 3: biomes (item 13)

**Map the sub-biomes first.** Before any modeling or regions: a map of Lake Hylia's sub-biomes drawn over the
depth map (zones, their edges, which fish, which props), for approval.

**References.** Newer Zelda games (Twilight Princess's Lake Hylia and fishing hole, Skyward Sword, Breath of the Wild
and Tears of the Kingdom, the Link's Awakening remake, Majora's Mask), the fishing games (TP's fishing hole, Sega
Bass Fishing's lakes without the modern gear), real ponds, lakes and underwater plants, and aquariums (nature
aquarium and iwagumi composition). Collected in a reference doc with colours, shapes and placement rules.

**Zones.** Some come from the map automatically, others are drawn by hand as circles and capsules in a per-scene
JSON file (baked into the habitat ids):

| Zone | Source | Who lives there |
|---|---|---|
| Shallows / beach | depth < 50 | minnow schools by day, carp, bass at dawn |
| Weed beds | hand-placed (north shore, island) | bass, pike in ambush, greengill |
| Lily field | hand-placed | bass under the pads, surface feeders at dawn and dusk |
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

**Drained lake (adult, before the Water Temple).** Fish only in the remaining pool and the Gerudo river, with
different species in each. The pool holds smaller, younger versions of the deep-lake species, staying deep: a
preview of what the full lake holds, out of reach of a bobber (the sinking lure and the hookshot are the way in,
later; once the lake refills, the Longshot's reach matters).

**Props.** The Fishing Pond's props are already in memory wherever fish are (`object_fish`): reeds, lily pads,
rocks, wood posts, an unused boulder, and the gravel texture. `gameplay_keep`'s grass blades make weed clumps;
`gameplay_field_keep` (loaded in Lake Hylia) adds floating leaves. Placed per zone from the JSON, drawn by the
manager like `Fishing_DrawPondProps` (material once per type), with a draw distance. Underwater weeds are tinted
grass blades swaying with a cheap sine (no per-vertex work).

**New assets in vanilla style.** Reuse only goes so far; some props are made from scratch: underwater weed
clumps, a sunken log, a flowering lily pad, gravel and silt patches. The rules: low vertex counts (about 10–60),
vertex colours, 16×16 or 32×32 CI4/CI8 textures, colours sampled from the scene's own textures. The pipeline:
- **style sheets first:** vanilla textures and models (Lake Hylia, the Fishing Pond, `object_fish`) rendered side by
  side with each new one, at game scale, for approval before it goes in;
- **geometry from scripts** (`fishmod/tools/`): small parametric meshes (a log is a few-sided cylinder, a weed clump
  is crossed ribbons), written as display lists through `assets.py`;
- **textures from scripts** (new `gen_habitat.py`): noise, speckle and strand patterns quantised to palettes taken
  from the vanilla textures next to them.

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
   - lateral line: short range all around, only for moving things (a struggling fish, a bait being moved);
   - sound: a cast's splash or Link's footsteps on the dock alert fish in a radius without telling them where.
4. **Mood:** hunger (rises over time, falls after eating), fear (Link close, splashes, a fish caught nearby) and
   a per-fish boldness from its seed (heritable boldness in Philipp 2009).
5. **The approach** (vanilla's sequence, with its numbers):
   - notice: turns toward, a flick of the tail;
   - follow: trails 1–2 body lengths behind a bait that moves;
   - inspect: hovers, backs off inside half a body length plus 20, comes back;
   - then either a wind-up and charge from 0.2–1.2 body lengths (Harper & Blake), or a **refusal**: turns away
     sharply and leaves.

   Each step shows, so the player reads what the fish thinks.
6. **Learning:** a fish that was hooked and got away, or was released, gets shy of that bait for the rest of the
   day. A spot's fishing pressure decays over days. The roster is per day, so learning is per fish within a day and per spot across days.
7. **Schools:** minnows and greengills become schools: a leader and up to ~12 followers at fixed offsets, drawn as
   the pond's 4-vertex group fish or the small rig up close. No neighbour queries. They swim faster in bright water
   (so they drift to shade, Berdahl 2013). A scare spreads through the school as a wave (each member reacts after
   distance / wave speed and flees ~30° off the threat). One member is promoted to a full fish when it goes for the
   bait. This makes a lot of small fish cheap.

Gate: AI v2 in the lab tank no more than today's AI cost; at the lake, a deep fish reaches a deep bait, and fish
come up at dawn.

## Phase 5: bobber and feel (items 1, 7, 8)

**Float depth (item 1): set on every cast, by eye.** Bobbers only for now.
- After the cast the bait sinks under the bobber. Press A once to stop it at that depth (the first A press during
  the sink only sets the depth; A reels as usual after that).
- **One stop per cast**; nothing is remembered between casts.
- If you never press, it stops by itself a little **above** the bottom, never on it, so shallow water isn't a
  button-mashing race.
- **No icon on screen.** The camera follows the bait down and you judge the depth by eye.
- **Camera:** today the zoom is a fraction of the distance from Link to the hook. It becomes a framing of the hook
  at a fixed maximum distance, so long casts don't leave the hook tiny and short casts aren't too close.
- Risk to check first: OoT's water fogs out quickly; if a deep bait can't be seen, read the depth from the bobber
  and the line's angle instead (still no icon).

**Bite rhythm (item 7).** Take, hold up to a second, spit and take again about one time in ten (the carp study), so
the bobber goes nibble, nibble, then under. Each nibble gets a small rumble (there is none today) and a sound; the
bite a bigger one.

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
| 5 Bobber and feel | 1, 7, 8 | M | 4 |
| 6 Looks and polish | 10, 11 | S–M | – (can be done any time) |

Phase 2 and 6 don't depend on the AI work, so they can fill gaps.

## Notes, not built (item 5)

To take note of and consider, not to implement in this version:
- **Lures and sinkers**, including the vanilla **sinking lure** (the adult Fishing Pond prize, kept once found).
  How you switch between bobbers, lures and sinkers is not designed yet.
- A sinker that sinks to the bottom and **rises** until you press A (the opposite of the float).
- Lure ideas from the research: spinner (flash and vibration, steady retrieve), crankbait (dives with reel speed),
  popper and frog (surface, dawn and dusk, over lily pads and weeds), sinking (strikes on the fall, vanilla's 1
  unit/frame). Retrieve styles (steady, twitch, stop-and-go; many strikes on the pause). Lure size limits fish size.
- **Hookshot and Longshot fishing**: the hookshot reaches the drained lake's pool; the Longshot matters once the
  water rises. Learn on the pool first.
