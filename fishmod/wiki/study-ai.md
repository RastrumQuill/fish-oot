# Study: efficient fish AI on the N64

Part of the fishmod study (see [study.md](study.md)). The strongest material here is decomp code read directly
(vanilla OoT's pond fish, Twilight Princess's fish, Majora's Mask's lab fish) and the ares emulator's CPU timing
table. Points about Reynolds, Abzû, Subnautica and Sega Bass Fishing internals come from search summaries and are
weaker. Cycle counts are emulator figures and instruction counts, not hardware profiling.

## Where our time goes today

The lab tank bench measures ~12 ms of fish update for 36 fish (about 0.33 ms each). The current AI:

- steers **yaw only** and eases the height separately (±0.8 units a frame); body pitch is just drawn;
- asks the water-floor cache 3+ times per move (in-bank check, then the centre and the nose), and up to 10 headings ×
  2 lookups when blocked (`OpenWaterYaw`);
- near banks, does real collision raycasts (`ExactFloor`), which the code's own comment puts at ~0.5 ms each;
- re-samples up to 16 points when stuck;
- calls `cosf` 4 times a frame for the swim wave.

The likely cost is the bank probing and raycasts, not the steering maths. **To confirm** with the per-call-site CPU
counters before changing anything.

## What shipped games do

**Vanilla OoT pond fish** (`ovl_Fishing`) is the Keese-style model:

- **Steering:** each frame it aims at the target with a real yaw *and* pitch (`Atan2S`), rate-limits both turns, and
  moves along that heading (`Actor_UpdateVelocityXYZ`). The body pitch is the real heading.
- **Walls:** "bump and slide". On a wall hit it offsets the target ±90° yaw / ±45° pitch for 20 frames. No
  pathfinding.
- **Noticing the lure:**
  - the lure must be in front of the fish (small fish see all around);
  - it must be within 120 units if Link is working the lure, 70 if not.
- **Bite chance:**
  - "perception" builds with each tug and wiggle, and the bite roll needs it;
  - big fish bite at ×0.3 or ×0.6;
  - dusk ×1.75, dawn ×1.5, fog ×1.5, storm ×3;
  - the sinking lure ×5.
- **Spooking:** Link's speed scales the spook radius.

**Vanilla schools** (`Fishing_UpdateGroupFishes`) are the cheapest believable schooling, and they shipped on this
hardware:

- The pond's 60 small fish **are not actors**: 72-byte structs in one array, updated in one loop.
- They swim in 3 schools of ~20. Each school's anchor slides around a big circle, and each fish heads for a random
  point near its anchor, re-picking it now and then.
- There are no neighbour queries at all. Fish scatter upward when Link or the lure comes near.

**Twilight Princess** (`d_a_mg_fish`):

- **Wandering:** targets around a home point, at the middle of the depth ±20%. It re-aims only **every 4 frames**,
  with fast approximate square roots and capped turn steps. Some fish follow 43 hand-placed path nodes around the
  lake.
- **One fish searches the lure at a time.** The detection radius grows when the lure is worked.
- **Learning:** each fish has **learning bits per lure type** that make it ignore lures it has learned.
- **Bite chance:** ×1.5 outside the middle of the day, ×1.75 in rain.

**Majora's Mask lab fish** (`En_Fish2`):

- **Walls:** a "whisker" wall test 25 units ahead; on a hit it turns 90° to a per-fish side.
- **Depth:** a random pitch held for 30–100 frames.
- **Food:** chases food with yaw/pitch toward it.

**Reynolds steering** (GDC 1999):

- **Behaviours:** seek, arrive, wander, obstacle avoidance, separation, alignment and cohesion, on a point with a
  max force and max speed.
- **Combining them:** a weighted sum, or by priority (avoidance overrides seek).
- **Scale:** his PS2 crowd demo ran ~300 agents using a uniform grid for neighbours. That comes from a secondhand
  summary.

**Abzû's** famous fish counts come from cheap *rendering* (vertex animation instead of skeletons), not cheap AI (from
secondary sources).

## Maths on the VR4300 (ares timing, matching the NEC manual as far as could be checked)

| op | cycles |
|---|---|
| add.s | 3 |
| mul.s | 5 |
| float↔int convert | 5 |
| div.s | 29 |
| sqrt.s | 29 |
| integer multiply | 5 |
| integer divide | 37 |

- `sqrt.s` is ~0.3 µs, and IDO emits it directly for `sqrtf`. **The fast inverse square root trick is pointless
  here:** its int/float moves plus a Newton step cost about the same.
- Compare **squared** distances and exit early, keep reciprocals instead of dividing, and use OoT's
  `Math_SinS`/`Math_CosS` instead of `cosf`.
- Fixed point gains no speed (integer multiply is also 5 cycles). Its win is **memory**: s16 positions halve what has
  to sit in the 8 KB data cache.
- One `EnFishmod` is ~2.7 KB, so a fish update starts cold in the cache. A **separate compact array** of positions
  (48 fish × 12 bytes ≈ 600 bytes) stays in the cache.
- For up to ~50 fish, **brute-force neighbour checks over that array beat a grid**: 780 pairs, most rejected by one
  subtract-and-compare. That's ~0.1–0.15 ms a frame for all fish. Grids and sort-and-sweep only pay off past ~100.

## 3D navigation without pathfinding

**Lake Hylia's water box** is 7680 × 5627 units, up to ~1090 deep.

| Representation | Memory | Verdict |
|---|---|---|
| Today's lazy floor grid | 24 KB (96 KB with the Pak) | no overhangs |
| Full voxel bitmap, 32-unit cells | ~179 KB | too big |
| 3D signed distance field | 64 KB+ per zone | not worth the RAM |
| **Floor + ceiling per column**, 32-unit cells, over a 2048² fishing zone | **16 KB** | recommended |
| Plus a **shore-distance byte** per column | **4 KB** | its gradient gives "away from the bank / toward deep water" for free |

- **Bake at build time.** Filling these at runtime by raycast (~0.5 ms each) is impossible (thousands of columns), so
  bake them from the extracted collision with the asset pipeline.
- **Habitat props as simple shapes.** Weed beds, rocks and logs are mod-placed, so describe them as spheres,
  capsules (logs) and boxes, not voxels. A fish can rest *under* a log because the column is open and only the
  capsule is avoided.
- **Whiskers become table lookups.** A look-ahead is then a column lookup (~50–100 cycles), not a raycast:
  - check ahead every frame;
  - alternate the left/right whiskers by frame;
  - use the shore-distance gradient to get unstuck instead of re-sampling.

## Behaviour structure

**Keep the state machine for the rod coupling** (inspect → nibble → bite → hooked → caught): it's a timed sequence.

**Above it, a tiny utility picker**, evaluated at think ticks:

- **Goals:** rest, cruise the habitat, follow the school, investigate the lure, flee, back to cover, chase an
  intruder.
- **Scoring:** each goal gets a score from simple curves over hunger, fear, light, distance and fishing pressure.
- **Hysteresis:** the current goal gets a bonus, so fish don't flicker between goals.
- A full behaviour tree is overkill for 7 goals.

**AI level of detail.** The level is picked by distance:

| Level | When | Think | Steer | Terrain checks | Separation |
|---|---|---|---|---|---|
| L0 | Link < 500, bobber < 300, or fighting | every 4 frames | every frame | every frame | yes |
| L1 | up to ~1200 | every 8 frames | every 2 frames (double step) | at think | school only |
| L2 (virtual) | beyond, or long off screen | — | — | — | — |

- **L2 fish** are just {habitat, species, size}. When they come back in, they're placed at a valid point in their
  habitat, off screen.
- **Global think budget:** at most ~6 thinks a frame, round-robin, so the cost stays flat as counts grow.

**Believable lure interest (senses → mood, not dice):**

- **Sight:** a cone of about ±60°, tested with a dot product (no square root). Range grows with light and for
  surface lures (silhouettes), and is shorter for bottom feeders.
- **Lateral line:** short range in every direction, triggered by lure motion (twitches, reel speed) and splashes.
  Bass are mostly sight feeders; this matters up close. The ranges are game tuning, not biology.
- **Smell (bait only):** the scent radius grows with soak time up to a cap. Larvae and worms have it; lures don't.
- **Mood:** hunger rises over time (faster in the species' active hours) and drops after feeding or a near miss.
  Fear rises with splashes and Link, then decays. Time and weather multiply both, as in the vanilla pond and TP.
- **Learning:** per-fish "shy" bits per bait type, set by a missed hook set or a break-off (TP does this).
  Per-habitat **fishing pressure** rises with catches and decays over in-game hours, so a fished-out spot goes
  quiet.
- **Territorial bass:** one big bass owns a piece of cover and chases fish and lures that cross it (aggression, not
  hunger).
- **Competition:** a second fish arriving makes the first more likely to bite (angling lore; our suggestion, the
  opposite of TP's one-at-a-time rule).

## Proposed AI v2

**Movement:** a yaw + pitch heading and a speed, moved like the vanilla pond fish. Each frame:

1. Build a desired direction vector from seek, separation and avoidance (avoidance wins when urgent).
2. Turn it into a yaw and pitch, clamping pitch to about ±45° for most species.
3. Step toward it at per-species turn rates.

Body pitch becomes real.

**Data:**

| Struct | What | Size |
|---|---|---|
| `FishmodBrain` (in the actor) | target, wanted yaw/pitch, goal, habitat, school, hunger, fear, shy bits, LOD, think phase, whisker side, timers, cruise speed | ~32 B |
| `FishmodFlock` (global, rebuilt each frame) | s16 positions, yaw, radius, school per fish; separation push out | ~790 B for 48 |
| `FishmodSpanField` (baked per zone) | floor + ceiling + shore distance per 32-unit column | ~20 KB per 2048² zone |
| `FishmodObstacle` (≤32 per scene) | capsule or sphere | 16 B each |
| `FishmodHabitat` (≤32) | position, radius, height, kind, species mask, capacity, occupants, owner, cover direction, pressure | 16 B each |
| `FishmodSchool` (≤4) | anchor, heading, speed, habitat, count, mood | ~24 B each |

**Estimated cost:** ~25 µs per fish at L0, so **~1.2–1.5 ms a frame for 40 fish**. That compares with ~12 ms
measured today. It's an estimate from instruction counts and could be off by 2×. L1 fish cost about a third; L2
nothing.

**Ranked by value / cost:**

1. **Replace the per-move floor probes, open-water search and bank raycasts with one look-ahead and a cached whisker.**
   Biggest CPU win, low risk. Measure first, then do it.
2. **Real yaw + pitch heading** (dives and climbs along curves). Small change, big visual gain, precedented in
   vanilla.
3. **Central flock array with separation,** pushing overlapping pairs apart, with a two-sphere body for long fish.
   This is what makes dense packing work.
4. **Small fish as school particles** (the vanilla pond's trick), promoted to a full fish actor when one goes for the
   bait. Removes actor and skeleton cost for most small fish.
5. **Habitat anchors + simple-shape obstacles:** weed beds, rocks, logs, cover and territories with no new collision
   tech.
6. **Baked floor/ceiling + shore-distance field:** removes runtime raycasts entirely. It needs a build-time tool;
   until then the lazy grid sits behind the same interface.
7. **Senses and mood rework** (sight cone, lateral line, scent, hunger/fear, shy bits, pressure).
8. **Virtual L2 fish and the think budget,** once counts or areas grow.
9. **Skip:** distance fields, voxel bricks, full boids alignment/cohesion for big fish, fast inverse square root.

## Sources

- OoT pond fish: https://github.com/zeldaret/oot/blob/main/src/overlays/actors/ovl_Fishing/z_fishing.c
- TP fish: https://github.com/zeldaret/tp/blob/main/src/d/actor/d_a_mg_fish.cpp
- MM lab fish: https://github.com/zeldaret/mm/blob/main/src/overlays/actors/ovl_En_Fish2/z_en_fish2.c
- ares VR4300 timing: https://github.com/ares-emulator/ares/blob/master/ares/n64/cpu/interpreter-fpu.cpp
- N64brew VR4300: https://n64brew.dev/wiki/VR4300 ; NEC datasheet: https://bitsavers.mirrorservice.org/components/nec/mips/Vr4300-ds_200011.pdf
- Reynolds 1999: https://ics-websites.science.uu.nl/docs/vakken/mcrws/papers_new/Reynolds%20-%201999%20-%20Steering%20behaviors%20for%20autonomous%20characters.pdf
- Abzû rendering: https://www.gdcvault.com/play/1024409/Creating-the-Art-of-ABZ
- Utility AI (Dill & Mark): https://gdcvault.com/play/1012410/Improving-AI-Decision-Modeling-Through
- AI level of detail: https://www.lirmm.fr/~fmichel/publi/pdfs/mahdi13icaart.pdf
- Bass senses: https://www.in-fisherman.com/editorial/sensory-muskies/154167 , https://americansportfish.com/how-do-bass-use-their-lateral-line/
- Sega Bass Fishing interview: https://itsfantastic.moe/sega-bass-fishing-developer-interview/
