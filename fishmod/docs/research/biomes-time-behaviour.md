# Research: biomes, time and behaviour (for test5)

New material only; [fish-ai.md](fish-ai.md), [fishing-feel.md](fishing-feel.md) and
[fishing-games.md](fishing-games.md) already cover the vanilla bite multipliers, TP's learning bits, the sight cone /
lateral line / scent idea, Reynolds basics and game feel. Web claims come from search summaries of the cited pages
(the sandbox couldn't open most sites); decomp claims were read from the code.

## 1. Daily rhythm, depth and weather

| Species (ours) | Evidence | Rule |
|---|---|---|
| Largemouth bass (Hyrule Bass, Hearty Bass) | Reynolds & Casterlin: dawn and dusk peaks plus a midday peak, little at night. Illinois pond EMG: 16–19% more active by day. ([SEAFWA](https://seafwa.org/sites/default/files/journal-articles/WOODWARD-113-122.pdf), [Hanson 2008](https://www.fecpl.ca/wp-content/uploads/2008/08/CJZ-Hanson-etal-2008.pdf)) | Day fish, dawn/dusk best, small midday bump, low at night |
| Northern pike (Hylian Pike) | Kobler et al. 2008: summer twilight peaks, flat in winter; more active by day ([HU Berlin](https://www.agrar.hu-berlin.de/de/institut/departments/dntw/jp_bfm/publ_html/kob_html)). Ambush hunter | Twilight peaks, sits at cover |
| Channel catfish (Ordon Catfish) | Wabash River: moves most at night in all seasons, holds at banks and riprap by day ([EIU](https://scholars.eiu.edu/en/studentTheses/diel-and-seasonal-patterns-of-channel-catfish-movement-and-habita/)) | Night roamer, bank cover by day |
| Common carp (Sanke Carp) | Mixed: night feeding in one lake, deep and inactive at night in another; individuals differ ([UdG](https://dugi-doc.udg.edu/handle/10256/12998), [bioRxiv 2025](https://www.biorxiv.org/content/10.1101/2025.01.27.635164.full.pdf)) | Per-fish "chronotype" offset |
| Stone loach (Hylian Loach) | Fischer 2004: forages more at night even when starving; stops when a predator is near ([AWI](https://epic.awi.de/id/eprint/16968/)) | Night only; freezes near predators and Link |
| Minnows, shiners (Greengill, Minnow) | Mehner 2012 review: vertical migration follows light, up at dusk, down at dawn ([PDF](https://gambusia.bsp.ncsu.edu/readings/Mehner%202012.pdf)) | School depth follows light |

- **Light is the lever with evidence.** Escanaba Lake creel data (PLoS ONE 2021): lower light raised walleye
  catchability; wind mattered for muskellunge; the angler mattered most
  ([DOAJ](https://doaj.org/article/fed3b791cae846e0b6f5b3d4532e11de)). Barometric pressure: no effect on perch
  feeding in the lab ([Bemidji 2014](https://faculty.bemidjistate.edu/ahafs/wp-content/uploads/sites/2/2024/10/2014-VanderWeyst-D.-The-effect-of-barometric-pressure-on-feeding-activity-of-yellow-perch.pdf));
  "pressure drops = feeding spree" is angler lore.
- **Summer squeeze:** below the thermocline oxygen drops under ~2 mg/L, so bass, sunfish and catfish stay above it
  ([SC DNR](https://www.dnr.sc.gov/news/2016/aug/aug22_lakemurraystripers.html)). In winter bass go deepest and
  slowest (Hanson 2008).

## 2. Lures, bait and learning

- **Senses by range:** vision notices from far, the lateral line handles the last metres, sound (rattles, splashes)
  alerts without a direction. Blinded bass strike from shorter range (Gardiner & Motta); blinded muskies find prey
  only within inches, lateral-line-blocked ones do almost normally
  ([In-Fisherman](https://www.in-fisherman.com/editorial/sensory-muskies/154167)).
- **Lure size gates fish size:** large baits select against small pike (Arlinghaus et al. 2008,
  [HU Berlin](https://www.agrar.hu-berlin.de/de/institut/departments/dntw/jp_bfm/publ_html/NAMFA_html)).
- **Learning:**
  - Beukema 1970: spinner catches of pike fell daily and became almost useless on fish caught before; live bait
    didn't lose effect. Carp learn in one trial ([Konstanz](https://kops.uni-konstanz.de/server/api/core/bitstreams/977243bf-639e-4a4a-b352-3d5b8e29d36e/content)).
  - Wegener et al. 2018 (bass, catch and release): catch rate falls through the season, recovers after a 2 month
    closure, and it's individual learning: uncaught fish stay catchable ([USGS](https://www.usgs.gov/publications/effect-fishing-effort-catch-rate-and-catchability-largemouth-bass-small-impoundments)).
  - Lovén Wallerius et al. 2020 (carp): being hooked, and seeing another fish caught, both raise hook avoidance;
    only the first lasts ([PDF](https://www.fecpl.ca/wp-content/uploads/2020/05/Lov%C3%A9n-Wallerius_2020_Hook-avoidance-induced-by.pdf)).
    Czapla et al. 2023: no avoidance after 7 months.
  - Philipp et al. 2009: vulnerability to angling is heritable (h² 0.15)
    ([ScienceDaily](https://www.sciencedaily.com/releases/2009/04/090414153532.htm)), so a per-fish boldness value.

## 3. Approach and strike

- Pike strike from 0.2–1.2 body lengths; shorter when the prey looks big (Harper & Blake 1991,
  [JEB](https://cob.silverchair.com/jeb/article-pdf/155/1/175/2578620/jexbio_155_1_175.pdf)).
- Carp hold a pellet up to 1 s in ~60% of tries, handle food up to 53 s and spit and re-take in ~10%
  ([vestnik](https://vestnik-pp.samgtu.ru/0042-8752/article/view/688143)): the basis for nibbles.
- **Vanilla pond fish** (`src/overlays/actors/ovl_Fishing/z_fishing.c`):
  - Notice (2794–2835): lure must be in front unless the fish is small; within 120 only if A is held or the lure
    wiggles, within 70 passively; interest lasts 100–199 frames.
  - Inspect (3336+): holds 10 to the side and 15 below the lure; inside `length*0.5+20` it backs off, then comes
    back (the hover-and-retreat look). Interest grows with A, B and wiggles.
  - Gives up with a 50–79 frame refractory; flees from Link.
  - Wind-up (3471): swings to a point 30 to the side and 30 ahead, then charges at 4.5–6 (3–4.5 on the sinking lure).
  - Hook-set window 10–30 frames, wider for small fish and the sinking lure.
  - Sinking lure: falls at up to 1 unit/frame and rests 5 above the floor; **while falling every inspecting fish
    gets interest each frame** (2584), which is "strikes on the fall".
  - The loach parks deep and now and then rises to the surface with a double ripple and a sound (3263): a cheap
    "something big lives here" tell.

## 4. Schools

- Couzin et al. 2002: repulsion first, then orientation and attraction; two radii switch a school between milling
  and travelling ([PDF](https://www.cs.princeton.edu/courses/archive/fall24/cos597C/bib/couzinKJRF.pdf)).
- Berdahl et al. 2013: golden shiners find shade without sensing gradients, just by swimming faster in bright water
  ([PMC](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5954127/)). O(1) per fish.
- Escape waves spread by copying neighbours (Herbert-Read 2015, Rosenthal 2015); fleeing ~30° off the predator's
  heading gives the "fountain" split ([PMC](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC11603345/)). Cheap version:
  each member reacts after distance / wave speed.
- Leader and followers at fixed offsets (Minecraft's schooling fish, robot formations) need no neighbour queries.

## 5. Games

- **Sega Bass Fishing:** "hot out? drop to cooler water"; topwater in cool mornings, heavy lures deep at midday;
  lures sorted by depth class ([manual](https://www.segaretro.org/images/f/f9/SegaBassFishing_Steam_manual.pdf)).
- **Dredge:** time only moves while sailing, fishing or resting; night-only fish
  ([wiki](https://dredge.wiki.gg/wiki/Time)).
- **Animal Crossing:** time and weather windows; shadow size tells size class.
- **Wurm Online:** 4 day periods, a 1–4 feeding rating per period per fish, min/max depth: a compact data layout.
- **Fishing Planet:** players prefer species with their own peak times over one global bite clock.

## 6. Day length

- OoT field (speed 10, night ×2): ~4 min a day. **The Fishing Pond sets speed 1 (`z_fishing.c:913`): ~41 min**, so
  Nintendo already slowed time tenfold for fishing.
- Majora's Mask 18 min, TP ~10 min, Stardew ~14 min (lengthened after players found 10 min too short), Minecraft 20
  min. Comfortable for hanging out: about 15–45 min, with a way to skip (Sun's Song).

## 7. Patterns per fish

- Reaction-diffusion stripes add stripes as the fish grows (Kondo & Asai 1995): bake a few tiles offline, pick the
  scale by size.
- Spinda (Pokémon): a 32-bit value places 4 spots, so almost every one is unique. Maps to koi: a seed stamps 4
  blotches.
- Palette or prim colour per fish: 32 bytes (CI4 TLUT) or one colour command.

## 8. Vanilla assets we can reuse (survey of `extracted/ntsc-1.2/assets/`)

**Already loaded wherever fish are** (`object_fish`, which En_Fishmod uses):
- Reed (3 verts), lily pad (5), rock (28), wood post (15, for docks), an unused boulder (58), gravel bottom texture.
  `Fishing_DrawPondProps` (z_fishing.c:4463) shows the batched draw.
- Lures: the pond lure (14 verts), an **unused old lure** (8 verts), the sinking lure (a 4-vert billboard drawn 20
  times), the hook.
- Schools: `gFishingGroupFishModelDL`, a 4-vert quad; vanilla draws 60 of them in 3 schools.
- Effects: ripple, bubble, dust and the inflow splash (`gFishingStreamSplashDL`); the pond's own batched effect pool
  and rain ripples on the water.

**`gameplay_keep`** (always): `gGrassBladesDL` (12 verts, a weed clump), bug skeleton (worm or bug bait),
bombchu, ripple and shockwave textures. **`gameplay_field_keep`** (loaded in Lake Hylia): floating leaves, bushes,
and an **unused fish skeleton** (`gFieldUnusedFishSkel`, ~30 verts, orange).

**Effects:** EffectSs has 85 slots for the whole game; `EffectSsBubble` makes a ripple when it surfaces (a "fish
rising" tell). For many ripples a batched pool of our own is cheaper.

**Weather:** `envCtx.stormRequest` fades sky and light like the pond's storms; Lake Hylia's adult layer already has
an `EN_WEATHER_TAG` for rain. Lake Hylia only has light configs 0–2, so heavy rain (config 4) would read past its
list. Rain rings on the water are the pond's own code.

**Lake Hylia** (`spot06`): one room; water boxes: Gerudo Valley river upper (y −873) and lower (−1113), main lake
(−1313) in two boxes. Deepest point 1088 units at the Water Temple gate; by area, about 9% is 0–50 deep, 19%
50–150, 25% 150–300, 34% 300–600, 14% deeper. The inflow is the **Gerudo Valley river**, not Zora's Fountain. The
child layer already has two vanilla fish schools (`Obj_Mure`).
