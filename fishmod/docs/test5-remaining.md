# Test5: the rest of the work (plan, not built yet)

What's left after the habitat props, the tour and the culling rewrite, in the order I'd do it. Each item says how,
what it costs, what could go wrong and how it's checked. Nothing here is started; the tour feedback may change the
habitat part.

Rule kept from the main plan: every change is measured on the benches (lab 36/90, lakeside 300) against the build
before it, and the frame must hold 50 ms (20 fps) with 300 fish in the lake.

## 1. Collision for rocks (new)

Adult Link walks the lake bed with the Iron Boots, so solid props must be solid; plants stay without collision.

- **How:** bake it into the scene's own collision at build time, not as actors. `gen_scatter.py` already knows every
  rock (position, turn, size). A new step writes a small hull per rock (about 6-8 triangles: a low cap and sides,
  matching the vanilla rock's shape closely enough to stand on) and appends them to Lake Hylia's collision list
  when the scene is built (the extracted scene is compiled anyway; nothing ROM-derived is committed).
- **Why baked:** static collision costs nothing per frame beyond the lookups it already does. Actors with
  collision (DynaPoly) would be limited to a few dozen and cost every frame.
- **Cost:** 276 rocks × ~7 = ~1,900 triangles on top of the lake's ~1,050. BgCheck's lookup memory for spot
  scenes is 0xF000; Lake Hylia gets a custom size (`BgCheck_TryGetCustomMemsize`), sized from a test.
- **Also:** the water map (`gen_watermap.py`) bakes floor heights from the same collision, so fish will see the
  rocks as bed too (they rest on them and avoid them) once both are made from the same list.
- **Also solid:** the sunken log under the island's tree, the old posts at the docks, the north-beach dock (item 3).
- **Risks:** a hull that doesn't match the drawn rock (floating or sinking feet); walls Link can climb oddly; a
  rock on a slope with gaps under it. Checked with Iron Boots on the bed (a script that walks Link across a few).
- **Measure:** BgCheck time in the lake bench, scene load time.

## 2. Habitat tuning (waits on your tour notes)

- Densities and draw distances per kind, from the stop numbers you send.
- **Underwater haze:** when the camera is under the surface, pull the fog in (set the underwater light setting's
  fog near/far closer than vanilla's, per scene), and shorten the props' draw distances to match (no point drawing
  what the fog hides; that pays for denser beds underwater).
- **Pond reeds sway:** vanilla sways them by turning each reed a little over time. In our matrices that's a small
  shear by a sine of time and position, per kind with a sway amount (reeds, ribbon weed, kelp). No per-vertex work.

## 3. A larger prop or two

- **The north-beach dock** (zone 15): pilings and boards, built from the bridge's own plank and post textures,
  with collision (item 1's pipeline) so Link can walk out on it and fish from the end. Placed by hand, not
  scattered. A boat at its mooring later.
- One more hero piece if wanted (your call after the tour): e.g. a big fallen ruin block beside the pillars (not
  on the walkway).

## 4. The drained lake (adult, before the Water Temple)

- Props: already hidden where the water has gone (per-chunk check); the pool and river keep theirs.
- A few props for the exposed bed (dry weed, cracked mud decal, the rocks stay) only if they read well; optional.
- **Pool species:** smaller versions of the deep-lake species living only in the pool and river, not reachable by
  a bobber in the deep parts. Spawn tables by water level: `species.json` gets a drained-lake table for the scene,
  the roster picks it when the lake is drained (the same save flag the props use).

## 5. Fishing feel

- **Nibble rhythm + rumble:** a fish taps the bait a few times (small bobber dips, a soft rumble each), then the
  real bite (the bobber goes under, a strong rumble). Strike between the taps = miss; strike on the bite = hooked.
  Species set the count and spacing (bass quick, carp slow and fussy).
- **"FISH ON!":** text when the hook is set, like Twilight Princess's: a short banner drawn with the game's font
  (or a small texture), popping in, holding a second, fading.
- **Tension by weight:** the line's strain from the fish's actual weight and how hard it's pulling (runs, head
  shakes), so a big fish has to be played and a small one can be reeled straight in.
- **Testing a deep fish taking a deep bait:** with the 180-unit cap, test a fish holding at 150 units taking a bait
  sunk to it (a script: cast, let it sink, wait).

## 6. Looks and animation

- **Per-fish looks:** each fish gets its own small hue and brightness shift of its species' colours, and its pattern
  shifted and/or mirrored, from its roster seed (so the same fish looks the same when it comes back).
- **Koi patterns (Sanke Carp):** a per-fish 32×32 pattern texture stamped from the seed at spawn (a few red and
  black blotches), only for carp near the camera.
- **Catfish curl:** a stronger body bend when a catfish turns and when it rests on the bed.
- **Small-fish swim phase:** small fish beat their tails faster and in short bursts, not the slow wave of the big ones.

## 7. Behaviour

- **Senses:** sight (a cone ahead, shorter in murky or dark water, longer by day), the lateral line (vibration:
  a bobber landing, Link wading or a fish fighting nearby, felt all round at short range).
- **Mood:** hunger rises over time and falls when a fish eats; fear rises with Link close, splashes or a fish
  caught nearby, and falls slowly. Hungry fish take baits sooner; scared fish hide in cover (now that homes are by
  cover, they have somewhere to go).
- **Schools:** minnows and greengill in schools: a leader runs the AI, followers hold offsets and copy it (cheap:
  one think per school). Drawn small and far as billboards.
- All staggered under the existing think budget; measured on the lab 90 bench.

## 8. Stretch: underwater atmosphere

Only if the budget allows after 1-7:
- **Bubbles:** a few small bubbles rising from weed beds near the camera (a small pool of billboards, like
  vanilla's effects).
- **Drifting specks:** a few dozen faint motes floating in the water around the camera when it's underwater.
- Together with the haze and the sway, these would sell the water most.

## 9. Wrap-up

- **Rain sound:** check it in an emulator with audio on (ares' audio is off on the server); if it's silent, use
  the storm ambience the way Song of Storms does.
- **Benches:** the full set against the test4 release and against this start of the test5 work.
- **Release lake-test5:** the patch, notes, and a test-mode variant; the tour stays as a separate feedback build.

## Order

1 (collision) and 2 (tuning, once your notes are in) first, since they change the lake everything else is tested in.
Then 5 (feel) and 7 (behaviour) together, since bites depend on mood and senses. Then 3, 4, 6. 8 last, if there's room.
