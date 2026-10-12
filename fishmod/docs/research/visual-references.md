# Visual references for Lake Hylia's habitats

Inspiration for test5's props and sub-biomes, not targets to copy: this is N64. Two parts: notes on the images the
user shared (below), then a research pass (from section 0 on).

**Decided:** no new fog for the lakes. The work is geometry and look.

## From the user's images

- **Sega Bass Fishing:** underwater dock pilings with cross-bracing and a broken crib structure; laydown logs and
  bare branches on a flat sandy floor; lone boulders fish hold beside; bass seen close and large. Clear water reads
  blue-teal, murky water brown-olive. Above water: a dock on square pilings, rain on a dark rippled surface.
- **Twilight Princess (Hena's fishing hole):** a mossy, rounded rock arch standing in the water as the pond's
  landmark; layered cliffs with moss caps; twin waterfalls; a flat grassy islet; a log edging the near bank;
  cherry-blossom trees; the cabin and dock. "FISH ON!" text when the hook sets (taken for phase 5).
- **Ponds:** lily pads in drifting clusters with a few pink or white flowers, some pads yellowing or turned up; reed
  and iris walls at the edges; grey stone banks; submerged weed visible under clear water between pad clusters.
- **Aquariums:** layered heights (carpet in front, stems behind), one hero rock or branch per group, a light gravel
  path through the plants, red accents among the greens, schools of small fish moving together in open lanes.
- **Underwater:** grassy clumps on rock between carpet plants; drowned trees standing in a murky layer; kelp-like
  ribbon columns (for the deep weed beds); a pebble bed in shallow river water (the inflow); sunlit weed tufts on a
  dark bed.
- Also suggested: Subnautica for ideas on how underwater zones read.



Date: 2026-10-12. Scope: art direction for underwater and shoreline props in an OoT (N64) fishing mod.

## 0. Access caveats (read first)

- The session's egress proxy rejected CONNECT to wikipedia, wikimedia/upload.wikimedia, imgur, zeldadungeon.net, zeldawiki.wiki and gamebits.net (HTTP 403, "organization policy"). WebFetch also failed (DNS) on those hosts. Only github.com / raw.githubusercontent.com were reachable.
- So **no reference images were downloaded**; `img/` is empty. All web-sourced facts below come from search-engine result summaries (URLs listed so a human can open them). 
- Items marked **[mem]** are from my own knowledge of the games / ecology, not verified in this session. Hex values marked **~** are eyeballed estimates, not pixel samples; replace with samples from the actual ROM textures or screenshots before committing.
- Scale reference throughout: adult Link / person = 1.7 m. Convert to OoT world units using the repo's own scale convention (not researched here).

---

## 1. Newer Zelda water and lakes

### Twilight Princess (GC/Wii, 2006)
Sources: https://zeldawiki.wiki/wiki/Hena%27s_Fishing_Hole , https://www.zeldadungeon.net/wiki/Hena%27s_Fishing_Hole , https://www.zeldadungeon.net/wiki/Hylian_Loach , https://www.almarsguides.com/retro/walkthroughs/Gamecube/games/zeldatwilightprincess/quests/heartpieces/locations/fishinghole

- **Hena's Fishing Hole layout** (search summaries + [mem]): a roughly round pond in a gorge "enclosed by tall rock formations". Hena's cabin and a short plank dock sit on the near (south) bank; canoe is rented there. A large **natural rock arch** stands in the water; **waterfalls** pour in from the cliffs; a **back cave/grotto** at the far north end holds a dense **lily-pad field** (only in summer the Hylian Loach lives there). Fan wikis: "larger fish congregate under the waterfalls, under the arch, or in the lily pads." Seasons cycle each entry (spring/summer/autumn/winter) and change both props (autumn leaf litter floating, winter snow on banks, lily pads seasonal) and fish.
- Design lesson: **three named hot spots = three distinct silhouettes** (falls = vertical white column + foam ring; arch = negative-space frame; lily field = flat green carpet in shade). Each is readable from the bank at 30+ m. Fish hold at *edges* of those features.
- **Look** [mem]: water surface is a desaturated teal-green, ~#4E7A6A to #6C8F7A near shore, darker ~#2E4A45 in the middle; strong specular/sky reflection. Under the surface (Link with Zora Armor in Lake Hylia) fog is green-teal ~#2F5A55 and closes within ~15-25 m; lake bed is muddy olive ~#5A5A3A with scattered rocks and long blade-weed clumps. Underwater plants are vertical alpha-cutout cards (crossed quads), lightly swaying.
- Reeds: dense alpha-card clumps, yellow-green with brown tips ~#8C9A4A / #6E6A3A, 1.5-2 m tall, clustered at banks and around the dock. Lily pads: flat single-quad decals ~1 m across, mid-green ~#4F7A35, some with pink/white flowers.

### Skyward Sword (Wii, 2011) - Lake Floria / Faron Woods
Sources: https://www.zeldadungeon.net/skyward_sword_lake_area_and_new_dungeon_revealed_aquatic_race_detailed_tons/ , https://zeldauniverse.net/2010/06/15/skyward-swords-visuals-impressionistic/ , https://www.nintendojo.com/?p=21687
- Art direction stated as "impressionistic", Cézanne-inspired: distant geometry dissolves into painterly blur (a depth-based "brush" filter).
- [mem] Lake Floria water: very saturated cyan-turquoise ~#3FB6C8 surface, underwater clear blue-green ~#2E8FA0 with long visibility (~40 m), bright sand floor ~#D8C48A, big rounded boulders, simple tall leaf/kelp-like ribbons and bubble columns. Faron Woods water: shallow, emerald ~#4FA07A, with giant lily pads and floating-flower platforms.
- Lesson: when the palette is saturated, keep shapes very simple and large; let colour carry the read.

### Breath of the Wild / Tears of the Kingdom
Sources: https://zeldauniverse.net/2023/07/11/zeldas-study-links-hidden-swimming-animation/ , https://zeldauniverse.net/2019/07/02/a-handy-trick-in-breath-of-the-wild-lets-you-see-clearly-underwater/ , https://www.siliconera.com/where-to-farm-hylian-rice-in-the-legend-of-zelda-tears-of-the-kingdom/ , https://zeldawiki.wiki/wiki/Lily_Pad
- Water is **deliberately near-opaque** from above (no underwater gameplay; avoids modelling lake beds). Shallow areas show the bed via a depth-based fade: clear amber/sand at <0.5 m, quickly to teal, then deep blue-green.
- [mem] Lake Hylia (BotW): surface ~#4F9AA8 shallow to ~#24566A deep; shoreline reed beds are dense grass cards in yellow-green ~#A8B85A; scattered lily pads in calm coves; Hylian rice appears as tall green grass in shallow water (TotK guide: rice drops from "grass growing in the water" at Rabella Wetlands, East Necluda). Zora's Domain: luminous blue stone ~#8FC7E0, with glowing lily-like flora; very clear, cold water.
- Lesson: **foliage is mass, not leaves** — big soft-edged grass clumps with a darker base and lighter tips; the eye reads the clump silhouette, not individual blades.

### Link's Awakening (Switch, 2019)
Sources: https://zelda.nintendo.com/links-awakening/blog/fishing-with-mikiharu-oiwa , https://www.zeldadungeon.net/links-awakening-director-breaks-down-the-fishing-mini-game-in-new-interview/ , https://www.gamepro.de/artikel/zelda-links-awakening-switch-look,3345549.html
- Diorama/toy art style with tilt-shift blur; glossy, plastic-like materials. Fishing pond is a **side-on cutaway "narrow aquarium"** where the fish are fully visible (director Oiwa).
- [mem] Pond water ~#5BC0D8 with light transparent layering; bed is clean tan sand with rounded green "lollipop" weed tufts and small pebbles; everything is chunky, rounded, oversized.
- Lesson: a **cutaway aquarium read** works best with a clean light floor, a few big dark props for contrast, and plants that are simple rounded shapes.

### Majora's Mask (N64, 2000) - the closest in tech
Sources: https://zeldauniverse.net/features/how-atmosphere-shapes-every-region-in-majoras-mask/ , https://zeldawiki.wiki/wiki/Great_Bay
- Great Bay "warm and murky" while cursed. [mem] Underwater fog in Great Bay is a dark blue-green ~#1E4650 at ~10-20 m; floor is pale sand with dark rock; seaweed is alpha-card ribbons scrolled/animated; Zora Hall surroundings use rock pillars and sand flats; Pinnacle Rock is near-black fog. Southern Swamp: poison water purple-brown ~#5A3A5A turning clean green-brown ~#4A6A40 after Woodfall; giant lily pads + stilt boardwalks + tree roots.
- **Tech used on N64 (OoT/MM)** [mem]: water surfaces are 2-layer scrolling textures (two tiles panned at different speeds, combined), semi-transparent with env/prim colour; underwater uses scene fog (colour + near/far) switched when the camera is submerged; underwater plants are billboards or crossed quads with 1-bit alpha (I4/IA8/CI4 textures), sometimes vertex-animated by the actor; caustics are a scrolling texture on the lake bed.

---

## 2. Fishing references

### OoT Fishing Pond (the in-game baseline)
Sources: https://zeldadungeon.net/wiki/Fishing_Pond , https://zeldadungeon.net/wiki/Ocarina_of_Time_Fishing_Guide , https://zeldawiki.wiki/wiki/Hylian_Loach
- Search summaries: an **arched log near the centre** of the pond that Link stands on to cast; fish flee to the **deepest part** when spooked; the Hylian Loach is found **around the lily pads** and drifts to the centre about once a minute; caught with the Sinking Lure.
- [mem] Layout: rectangular-ish pond with the hut/counter at the entrance side; a small stream/waterfall inflow at the back; reeds in shallow corners; lily pads in a patch on one side; a sloped sandy margin dropping to a central deep basin; a few boulders. Water is very clear (aquarium feel) — you can see fish silhouettes from the bank. Reed texture: thin vertical blades, yellow-green; lily pad: round notched quad, mid green with lighter veins.
- Lesson for the mod: the vanilla pond already teaches players "fish hold near lily pads, the log, and the deep hole". Reuse that vocabulary in Lake Hylia.

### Twilight Princess fishing hole: behaviour of its environment
See section 1. Summary rules: (1) shade/overhead cover (lily pads in the cave, the arch) holds big fish; (2) falls = oxygenated, current, foam = active fish; (3) seasons alter what's present; (4) canoe gives access to the middle hot spots — bank fishing reaches the edges only.

### Sega Bass Fishing (arcade 1997 / Dreamcast 1999)
Sources: https://StrategyWiki.org/wiki/Sega_Bass_Fishing , https://www.gamebits.net/segabass/ (blocked; summary only)
- Search summaries: reviews mention "debris, driftwood, rocks and lake grasses" underwater; boats tied at a dock as backdrop; arcade stages named variously (Lodge / Cape / Inlet; DC tournament across "Lake Paradise" locations), at different times of day. Sources conflict; treat stage names as unverified.
- [mem] Visual vocabulary to borrow (skip boats, sonar, modern lures): a **lodge dock with square pilings**; **stumps** standing in shallow water off a point; **laydowns** (fallen trees with branches into water); **weed beds** as clumps of tall green ribbon grass; **rocky points** with tumbled boulders running out under water; **drop-offs** shown by the bed going from light sand to dark blue fog; a **creek inlet** with current. Underwater is a clear blue-green ~#3A8A90 with ~20 m fog. Bass hold: tight to cover (stump base, under dock, laydown branches, weed edge, point tip).

---

## 3. Real freshwater ecology

Sources: https://nhlakes.org/wp-content/uploads/Aquatic-Plants-and-Algae.pdf , https://www.lakestewardsofmaine.org/wp-content/uploads/2019/06/Aquatic-Plant-Communities.pdf , https://files.dnr.state.mn.us/natural_resources/water/lakes/vegetation_reports/11030800.pdf , https://jlimnol.it/index.php/jlimnol/article/view/jlimnol.2014.932 , https://gobotany.nativeplanttrust.org/species/vallisneria/americana/ , https://warcapps.usgs.gov/PlantID/Species/Details/1235 , https://edis.ifas.ufl.edu/publication/AG437/pdf , https://extension.msstate.edu/sites/default/files/publications/P3735-33_spatterdock.pdf , https://extension.rwfm.tamu.edu/wp-content/uploads/sites/7/2020/12/EHT-149-Water-lily.pdf , https://fortress.wa.gov/ecy/gisresources/lakes/AquaticPlantGuide/descriptions/nymodo.html , https://www.chesapeakebay.net/discover/field-guide/entry/spatterdock , https://www.oneonta.edu/faculty/farberas/arth/lake/lake_plants.html , https://www.michiganseagrant.org/wp-content/uploads/2023/12/MIDWEST-AQUATIC-PLANT-MANAGEMENT-SOCIETY-PLANTREFERENCE-CHART.pdf , https://snyderslake.org/native-plants--eurasian.html , https://www.angera.it/en/articles/tifa , https://sfer-docs.sfwmd.gov/2022_sfer_final/v1/appendices/v1_app5c-1.pdf

### Zonation (shore to deep)
1. **Wet margin / sedge meadow**: above to ~0.1 m water. Sedges, rushes, grasses.
2. **Emergent zone**: 0 to ~0.6 m (cattail peak density ~0.2 m; cattail max ~0.8 m; common reed and bulrush to ~1-1.5 m [mem]).
3. **Floating-leaved zone**: ~0.6 to 3 m (NH guide "2-10 ft"). Spatterdock shallower (0.3-2 m), white water lily 0.5-2 m (WA Ecology 3-6 ft).
4. **Submerged zone**: from ~0.3 m down to the light limit. Rule of thumb (MN DNR): plants grow to ~**1.5 x Secchi depth**; light reaches ~2 x Secchi. Wave-exposed shores push the *shallow* limit deeper (bare sand/gravel band at the windward shore).
5. **Open water / profundal**: bare silt, no plants. Slope sets bed width: gentle slope = wide bands; steep drop-off = narrow band then nothing.
- Zones overlap; they are not crisp stripes (Maine/NH guides). Tallest submerged species (milfoil, pondweeds, coontail) sit in the outer/deepest band forming a "weed edge"; low carpets (Chara, small rosettes) can go deepest in clear water.
- Example Secchi values [mem]: clear oligotrophic lake 6-10 m -> plants to ~9-12 m; typical mesotrophic 2-4 m -> plants to 3-5 m; turbid/eutrophic 0.5-1 m -> plants only to ~1-1.5 m.

### Species table (real-world; colours ~ approximations)

| Group | Species | Shape / silhouette | Height / size | Depth | Colour (~hex) | Density / placement | Movement |
|---|---|---|---|---|---|---|---|
| Submerged | Eelgrass / tape grass (Vallisneria americana) | rosettes of flat ribbons, 3-15 mm wide, rounded tips; tips trail on the surface | leaves 0.3-1.1 m typical (to ~1.8 m) | shallow-mid, clear water, 0.3-~3 m | green #5E8C3A to brown-red #7A5A3A | meadows; plants spread by runners, forming lawns | long ribbons stream with current, all leaning one way |
| Submerged | Coontail / hornwort (Ceratophyllum) | bottlebrush whorls of forked spiky leaves; rootless | stems 0.5-3 m | anywhere, often dense masses just under surface, also deep channels | dark olive #3E5A2A | tangled clouds | stiff, little sway |
| Submerged | Elodea / waterweed | small leaves in whorls of 3 on long stems | 0.3-3 m | 0.5-4 m | bright translucent green #6FA040 | dense stands | gentle sway |
| Submerged | Milfoil (Myriophyllum) | feathery whorls of 4, reddish tips; canopy at surface | to 3+ m stems | 1-3.6 m (3-12 ft) | olive #5A7A35, tips #8A4A3A | forms surface canopy "mats" | limp, flows |
| Submerged | Pondweeds (Potamogeton: large-leaf, curly-leaf, floating-leaf) | upright stems with broad or wavy translucent leaves; some float oval leaves | 0.5-3 m | 0.5-5 m | olive-brown #6A6A30, reddish #8A5A30 | clumps and beds; outer weed edge | sway |
| Submerged | Chara / muskgrass (an alga) | low bushy whorled branches, gritty, carpet | 0.1-0.6 m | to deepest plant zone in clear hard water | grey-green #7A8A6A (calcium crust) | carpets on sand/marl | almost none |
| Emergent | Common reed (Phragmites) | tall stiff stems, strap leaves, feathery purple-brown plume | 2-4 m | dry to ~1.5 m | stems #8A9A50, dead #C8B07A, plume #7A5A5A | very dense monoculture walls | whole stand waves, plumes nod |
| Emergent | Cattail (Typha latifolia) | flat sword leaves + brown cigar spike | 1.5-3 m | 0-0.8 m (peak ~0.2 m) | leaves #6E8A3A, spike #5A3A22 | dense clones, stands with crisp edge | leaves bend, spike stiff |
| Emergent | Bulrush (Schoenoplectus) | round leafless dark stems, small brown flower tufts | 1-3 m | 0-1.5+ m | dark green #3E5A30 | sparser, rows out into water | stems bob |
| Emergent | Sedges (Carex) | arching grassy tussocks, triangular stems | 0.3-1.2 m | damp margin to 0.2 m | blue/yellow-green #8A9A5A | tussocks spaced along bank | arch and flutter |
| Floating-leaved | White water lily (Nymphaea odorata) | round flat pads with narrow slit; star-like white flower, yellow centre | pads 15-30 cm; flower 7-20 cm | 0.5-2 m | pad top #4A7A30, underside #7A3A3A, flower #F4F0E8, centre #E8C030 | patches; pads overlap, gaps of open water | bob gently, flowers open by day |
| Floating-leaved | Spatterdock (Nuphar) | heart pad with V-notch, often held *above* water; yellow ball flower | pads to 40 cm; flower 2-5 cm | 0.3-2 m | pad #5A8A35, flower #E8C820 | dense, leaves stand up crumpled | stiff |
| Floating-leaved | Lotus (Nelumbo) | round pad no slit, raised up to 1-2 m above water, big pink flower | pad 30-60 cm | 0.3-2 m | pad #6A9A5A (bluish waxy), flower #E890A8 | massive colonies | sway on stalks |
| Floating-leaved | Water shield (Brasenia) | small oval pads, stem in centre, slimy | pads 5-12 cm | 0.5-2 m | pad #5A7A35, underside purple #6A3A5A | scattered carpets | lie flat |
| Free-floating | Duckweed (Lemna) | 2-5 mm dots, solid sheet | — | calm surface | bright #8AC040 | sheets in calm backwaters, against reeds downwind | drift with wind |
| Free-floating | Frogbit | coin-size round leaves, rosettes | 2-5 cm | calm surface | #5A8A30 | among reeds | drift |

### Substrate
- **Sand**: wave-washed shores and points; tan #C8B48A, rippled. Plants sparse.
- **Gravel/cobble**: inflows, windward shores, rocky points; mixed grey-brown #8A7A6A with darker gaps; spawning sites.
- **Silt/mud**: sheltered bays and deep basin; dark olive-brown #4A4030, soft, smooth; where most submerged plants root.
- **Leaf litter / organic muck**: under overhanging trees and in coves; brown #5A4028 with dark patches.
- **Sunken wood**: logs black-brown #3A3020, often algae-coated green-brown #4A5030, bark gone, branches stubbed.
- **Inflow**: delta fan of sand/gravel, clearer, colder water plume, scoured channel, bubbles/foam line.

### Underwater light
- Clear lake: blue-green #3A7A80, visibility 5-10 m+, sunbeams and caustics on the bed.
- Typical lake: green #4A6A40, visibility 1-3 m.
- Turbid/eutrophic: pea-green #5A6A30, <1 m. Tannin-stained: tea-brown #5A4A28 (fishing forums say tannic water can still have a few feet of visibility; silt reduces clarity more).
- Bass and cover (fishing sources): brighter light -> fish hold tighter to cover; edges (weed edge, mud line, drop-off, point) concentrate fish.
- Sources: https://southernfishingnews.com/part-1-water-conditions-101/ , https://bassresource.com/bass-fishing-forums/topic/73336-bass-and-cover

---

## 4. Aquascaping as composition guidance

Sources: https://en.wikipedia.org/wiki/Aquascaping , https://fishyhub.com/sg/blogs/blog-detail/iwagumi-aquascape , https://gensou.sg/?p=40768 , https://www.adana.co.jp/en/aquajournal/nature-in-the-glass-aj336/

- **Nature Aquarium (Amano)**: one focal point at the golden ratio (~38%/62% across) or ~1/3 rule; asymmetry; few species in large masses; stones read as mountains, wood as fallen trees; negative space (ma) is part of the design; wabi-sabi imperfection.
- **Iwagumi**: odd numbers of stones (3, 5, 7); main stone *oyaishi* (largest), secondary *fukuishi*, accent *soeishi*, plus small *suteishi*; one rock type; all grain lines and tilt angles matching; typically one carpet plant species.
- **Dutch style** [mem]: no rocks/wood; dense plant groups arranged in "streets" (terraced diagonal rows of contrasting colour/texture, receding from front to back); about one species per 10 cm of tank width; strong colour contrast between neighbouring groups; low foreground, mid stems, tall background.
- **Translate to props**:
  - Per cluster: 1 hero + 2 secondaries + 2-4 small pieces (odd count), the hero at about 1/3 of the cluster.
  - Height layering relative to the player view: low (Chara, gravel) nearest shore; medium (pondweed, eelgrass) mid; tallest (milfoil/coontail weed edge) at the deep side. This matches real zonation.
  - Leave open "lanes" of bare sand between beds so fish and lures can be seen (OoT clarity = aquarium).
  - Keep a shared lean/grain direction for rocks in a group, and one current direction for all ribbon plants in an area.

---

## 5. Style rules for the mod

### Do
1. **Build from silhouettes**: crossed quads (2-3 planes at 60/90 deg) with 1-bit alpha for reeds, cattails, eelgrass, milfoil; a single flat quad per lily pad; low-poly hulls (6-12 tris) for rocks; 6-sided cylinders for logs and pilings.
2. **Textures**: 16x16 or 32x32 (32x64 for tall reed/eelgrass cards), CI4/I4/IA8; reuse OoT's sand/cobble/plank tiles for beds, stones and pilings rather than making new ones. Match OoT's soft, slightly blurry pixel density (no high-frequency noise).
3. **Vertex colours do the lighting**: dark at the base, light at the tip (e.g. reed base #4A5A2A -> tip #A8B060); underwater objects shaded toward the fog colour with depth; floor props darker on the underside.
4. **Zonation in placement**: shore -> sedge tussocks -> reed/cattail walls (0-0.6 m) -> lily pads (0.6-2.5 m) -> submerged beds to a weed edge at ~1.5 x the visibility you chose -> drop-off into bare silt and fog.
5. **Odd-number clusters**, one hero per cluster, gaps of open water/sand between clusters; follow one current direction per area.
6. **Animation**: gentle UV-scroll or 2-frame sway on submerged ribbons (period 2-4 s, amplitude ~5-10 deg); lily pads bob ±1-2 units; reed tops sway with Lake Hylia wind; inflow uses a scrolling foam texture + bubble particles.
7. **Scale vs 1.7 m Link**: reeds/cattails 1.5-2.5 m (taller than Link, so they hide him), sedges knee to hip (0.4-0.9 m), lily pads stylised up to 0.6-1.0 m (oversized for readability, like TP/OoT), lily flowers ~0.25 m, logs 0.3-0.6 m diameter x 4-8 m, pilings 0.25 m wide, rocks 0.3-2 m, eelgrass ribbons 0.6-1.5 m.

### Don't
- No photographic textures, no gradients baked into large textures, no per-leaf geometry.
- No saturated pure greens (#00FF00-like); OoT greens are olive/yellow-tinted.
- No uniform grid placement or perfectly even density; no straight lines of plants except along a dock or old fence.
- No modern objects (plastic buoys, boats with motors, metal ladders, signs with modern text).
- Don't plant across the drop-off: the deep basin stays bare so the fog edge reads as "deep".
- Don't over-use transparency layers stacked in view (N64 fill rate; also sorting artefacts with the water surface).

### Proposed palettes (~16 colours per family; approximations to tune against sampled OoT textures)

**A. Emergent (reeds, cattails, bulrush, sedge)**
#2E3A1E #3E4E24 #4A5A2A #5A6E30 #6E8A3A #7E9A44 #8C9A4A #A0AC58 #A8B060 #B8B870 #C8B07A #D8C890 #6E5A3A #5A3A22 #7A5A5A #3E5A30

**B. Floating-leaved (lily pads, flowers, duckweed)**
#2E4A22 #3A5E28 #4A7A30 #5A8A35 #6A9A40 #8AC040 #A0C860 #7A3A3A #6A3A5A #F4F0E8 #F0D8E0 #E890A8 #C86080 #E8C030 #E8C820 #B0A040

**C. Submerged (eelgrass, milfoil, pondweed, coontail, Chara)**
#1E3A2A #2A4A30 #3E5A2A #4A6A35 #5A7A35 #5E8C3A #6FA040 #7A8A6A #6A6A30 #7A5A3A #8A4A3A #8A5A30 #4A5030 #3A5040 #2E5A55 #5A8A80

**D. Substrate (sand, gravel, silt, leaf litter)**
#E0D0A0 #D0C090 #C8B48A #B8A070 #A08860 #8A7A6A #706458 #5A5048 #4A4030 #3A3428 #5A4028 #6E5030 #4A5A40 #3A4A3A #2A3A34 #9A9480

**E. Wood (sunken logs, stumps, dock pilings, planks)**
#1E1810 #2A2218 #3A3020 #4A3A28 #5A4830 #6E5838 #806848 #947A58 #A88C68 #BCA078 #4A5030 #5A6038 #3E4A3A #707060 #8A8A78 #2E2A24

**F. Rock (boulders, cobble, rocky points)**
#2A2C2E #3A3C3C #4A4C4A #5A5C58 #6A6C66 #7A7C74 #8A8A80 #9A9A8E #ACAA9C #BCB8A8 #5A5A48 #4A5040 #6A6A50 #5A6A5A #3A4A48 #2A3A3E

**G. Water / fog targets (per sub-biome)**
Clear sandy shallows fog #4A8A8A (far ~25 m) · Weed-bed bay #4A6A40 (far ~10 m) · Deep basin #1E3A48 (far ~18 m) · Inflow plume #6AA0A0 (far ~20 m) · Lily cove #3A5A3A (far ~8 m) · Silty mud flat #5A5A38 (far ~6 m)

### Sub-biome recipes
- **Inflow / river mouth**: gravel-cobble fan, scoured channel, foam line, few plants except eelgrass bending downstream; rocks with shared tilt; fish at the seam where current meets slack water.
- **Reed bank**: wall of reeds/cattails with ragged water edge, duckweed sheet trapped downwind, sedges on land side; 1-2 open pockets ("reed gaps") where fish hold.
- **Lily cove**: sheltered, silty, dark; pads in 3-5 overlapping patches with lanes between, 10-20% flowering; sunken log or stump under the pads.
- **Weed flat**: 1-2.5 m deep, gentle slope, alternating eelgrass meadows and pondweed clumps with sand lanes; Chara carpet near shore.
- **Weed edge / drop-off**: tallest milfoil/coontail wall at the lip, then steep bank into fog; scattered boulders on the slope.
- **Rocky point**: iwagumi-style 3/5/7 boulders running out under water, cobble, little vegetation, clear water.
- **Dock**: square/round pilings (algae band at waterline #4A5030 and dark below), planks reused from OoT textures; shade patch under the dock; a few weed clumps around piles.
- **Sunken timber**: laydown log angled from shore into deep water, branch stubs, leaf litter, sparse weeds around.
