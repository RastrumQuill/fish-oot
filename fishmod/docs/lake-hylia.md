# Lake Hylia plan

Status: planning. Kokiri Forest stays the place to polish mechanics; Lake Hylia is the big-water area and the
performance benchmark. Test-mode builds warp between the two (D-pad Up: Kokiri, D-pad Down: Lake Hylia) so a
small and a big body of water can be compared. Until this plan is built, Lake Hylia has a provisional table of
the Kokiri species.

## Why Lake Hylia

- It's the biggest body of water in the game, so it's the worst case for performance: lots of water to keep
  track of, fish spread far apart, and long casts.
- It has a lot of variety for its size: shallows, deep water, an island, bridges, a dock, an inflow and the lab.
  That's room for sub-biomes that feel different to fish in.
- It already has fishing in it (the Fishing Pond), so the lake is the natural place for fishing outside the pond.

## Sub-biomes

Fish are anchored to a sub-biome, more or less: each fish gets a home inside one zone and does most of its
wandering, resting and feeding there. A hooked or spooked fish can leave its zone, and drifts back over time.
Each zone has its own species mix, preferred depth and active times.

| Zone | Where | Water | Fish (examples) | Notes |
|---|---|---|---|---|
| **Shallows** | The sandy shore where the road from Hyrule Field comes down, and the beach by the lab | Shallow, sunny, warm | Minnows, greengill, young bass | Easy fishing for beginners; fish rest in the sun during the day and spook easily |
| **Weed beds** | Along the shore past the lab and around the bridge footings | Shallow to mid, cover | Bass, pike (ambush) | Needs the weeds and reeds from the underwater habitats moonshot; until then, picked spots along the shore |
| **Dry dock** | The dock by the lab and the Fishing Pond's pier, and the shade under them | Mid, shaded, posts | Bass, catfish | Structure fishing: fish hold in the shade by the posts. Cast tight to the structure |
| **Drop-offs** | The edges where the shallows fall away to deep water, and around the island | Steep, mid to deep | Carp, bass, larger greengill | Fish patrol along the edge between feeding up shallow and resting deep |
| **Deep water** | The middle of the lake, the Water Temple island area | Deep, cold, dark | Catfish, loach (bottom), big pike, hearty bass | Long casts, heavy fish, slow fights. The rare record fish live here |
| **Inflow** | Where the water comes in from Zora's Fountain | Moving water, oxygen | River fish (from Zora's River) | Current-loving species, more active at dawn |

Special case: as adult, before the Water Temple is cleared, **the lake is drained**. Options: no fish (the lake
is mud), a few stranded pools with trapped fish, or the drained state only in a later version. Decide before
building adult Lake Hylia fishing.

## How anchoring works (implementation sketch)

- A **zone table per scene**: a list of zones, each a few circles (x, z, radius), plus depth preference
  (surface/mid/bottom), species weights and a fish budget. It lives in `fishmod/data/species.json` next to the
  species and is generated into C like the spawn tables are now.
- At spawn, a fish picks a zone (by the zones' budgets) and a home inside it. Wandering picks targets inside its
  zone, and the stuck watchdog and fleeing can take it out, after which it heads home again.
- The zone's depth preference overrides the species' default depth (a bass at the dock holds mid-water in the
  shade, a bass in the shallows sits high in the sun).

## Performance plan (benchmark)

Lake Hylia is where performance is measured. Things to build and measure, one at a time, with the debug timing
line and the test-mode FPS counter, against Kokiri:

1. **Fish near Link only.** Keep the number of live fish at a budget (about 12 to 16), spawning them in zones
   near Link and despawning far ones (keeping each zone's population, so a fish caught or released stays that
   way). The whole lake doesn't need to be alive at once.
2. **Water floor cache per zone, not per water box.** The cache is sized by water box today, and Lake Hylia's
   boxes are huge, so it has to fall back to coarse cells or raycasts. Caching only around the zones, at fine
   cells, keeps the memory small and the steering exact where the fish are.
3. **Long casts.** Check the line strip, the cast camera and the bobber over long distances (with strength
   upgrades, a cast reaches 920+ units).
4. **Views.** Measure frame time looking out over the open lake and toward the lab and bridges, where the scene
   itself is heaviest.

## Species and size of water

Pick species to fit the size of the water. Big fish need big water. Small ponds, moats and grottos get small
fish; only the big lakes and rivers get the big ones. Kokiri's moat keeps its larger fish for testing, but new
areas follow the rule.

| Water | Size class | Biggest fish |
|---|---|---|
| Grotto pools, small ponds | Tiny | Minnows, small greengill (up to about 8 in) |
| Kokiri moat, castle moat, oasis | Small | Up to about 16 in (greengill, small bass, small carp) |
| Zora's River, Lost Woods shortcut | Medium (flowing) | Up to about 24 in, river species |
| Zora's Domain, Zora's Fountain | Large | Up to about 30 in |
| Lake Hylia | Huge | Everything, including the record fish (36 in and up) |

## Biome ideas (other areas)

- **Grottos**: at least one grotto has a large body of water. Pale cave fish, fireflies as bait.
- **Lost Woods**: the small water in the woods, plus the shortcut from Zora's River (which can share Lost Woods
  fish). Mysterious, nocturnal fish.
- **Zora's River**: fast water, rapids and pools. Trout-like river fish, the pike at the deeper pools.
- **Zora's Domain**: cold, clear water. Zora-themed species, the Reekfish.
- **Zora's Fountain**: deep and icy; Jabu fry around Lord Jabu-Jabu.
- **Lake Hylia**: this plan.
- **Desert oasis** (Desert Colossus): a tiny, warm pool. Small, colourful desert fish.
- **Hyrule Castle moat**: murky water. Catfish, carp, the odd strange thing.
- **Gerudo Valley**: the river under the bridge. Hard to reach water, strong fish.
- **Water Temple**: later; fishing inside a dungeon is its own design question.

## Open questions

- Adult Lake Hylia drained: no fish, stranded pools, or skip?
- Does the Fishing Pond's owner react to the lake fishing (records, a rival)?
- How many fish at once can Lake Hylia keep on real hardware at 20 fps? (Measure with the first build of the
  near-Link spawning.)
