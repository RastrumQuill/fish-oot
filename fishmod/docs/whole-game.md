# The whole game smoother: what applies where

A running list of every optimization and what it would take to apply it to the rest of the game (every scene, not
just Lake Hylia). Each is tested before it goes in (the same picture, the same collision answers, the same sound,
or, where it can't be the same, said so). Kept up to date as work lands.

Status: **all scenes** = already applied game-wide; **lake only** = done for Lake Hylia, the rest to do; **to do** =
not started; **on hold** = waiting on a decision.

## Engine-wide (every scene already, by being in the code)

| What | Status | Proof |
|---|---|---|
| Actor framework: list links and routines in the first cache lines, walks without extra misses | all scenes | collision logs equal |
| Collision: static lookup lists made contiguous at load | all scenes | collision logs equal |
| Collision: floor/wall answer caches, raycast caching | all scenes | collision logs equal |
| Matrix work: affine skip, fast float-to-fixed | all scenes | host tests, graphics logs |
| Audio driver: per-note/channel fields packed, slots claimed without a read, sample store in RAM | all scenes | audio check (every sample) |
| Sky: one-cycle drawing, no colour fill under it, Z clear by the RSP | all scenes | screenshots (frames11) |
| Run-ahead: a third frame buffer, the display-list pool | all scenes | console runs |
| Display lists: the cache lines claimed instead of read | all scenes | graphics logs |

## Build passes over the assets (they run over every scene; what each does)

| What | Status | Notes |
|---|---|---|
| `--colpack`: collision triangles in the same plane merged (fewer triangles, same surfaces) | all scenes | the pass covers every scene's collision |
| `--texpack`: 16-bit textures of 256 colours or fewer to 8-bit indexed, pipe syncs with nothing between dropped | all scenes (when built with it) | not in the bench builds yet: test on the console |
| `--meshpack`: wide mesh pieces cut into areas the RSP culls on its own | lake only | the tool takes a list of pieces: for another scene, pick its widest pieces (the dump analyzer shows them) |
| `--meshpack`: lit scene geometry lit by the CPU from the frame's sky lights | lake only | needs the scene's room to have only sky lights (true for rooms: point lights are never bound to them); the CPU side keys on scene and room |

## Measured per scene (to do for each)

- **Where the frame goes**: the frame log bench exists only for the lake route. A short scripted route through each
  other scene (or the busiest ones first: Hyrule Field, Kakariko, Castle Town market, Death Mountain, the dungeons
  with many actors) to find what's slow there.
- **Display-list waste**: `fishmod/tools/dlstat.py` on a RAM dump of each scene (state set to what it already is, pipe
  syncs with nothing drawn, texture loads repeated). For the game's own object and scene lists the waste can't be
  removed in place safely (lists jump into each other): a build pass over the extracted lists, like texpack's, can.
- **Vertices per triangle**: the lake loads ~1.8 vertices per triangle drawn; the rooms with the worst ratio first.

## Places to measure, by the engine system they load

Order (13 Nov): the animation harness first, then the spots bench. The spots bench runs first on a **control**: a build
as close to the original ROM as can be (the game's own code and assets, only the frame log and the spots script
added: none of fish-oot's changes), to have the original game's numbers at each place; then every engine change is
validated against that control, once Lake Hylia is done.

Not a whole-game profile: a "spots" bench (one ROM, one power cycle) that warps to each place, walks a short fixed
path for ~30 seconds and tags the frame log and profile by place, to see which engine system costs most where, and
then whether an engine-wide improvement (a faster particle system, actor update, skeleton draw...) makes each run
smoother. Documented reports (13 Nov search: scattered, none measured on hardware in these terms) are marked *(rep.)*.

| System | Places |
|---|---|
| Particles and effects (EffectSs, Effect) | Death Mountain Crater (fire, smoke, embers); Fire Temple; the rain at Lake Hylia and Kakariko (Song of Storms); Ganon's Tower collapse *(rep.)* (debris, fire); Volvagia; bomb and bombchu explosions in a crowd (Bombchu Bowling); Dodongo's Cavern (fire, bomb flowers) |
| Many actors (update, draw, collision) | Castle Town market *(rep.)* (people, dogs); Kokiri Forest *(rep.)* (Kokiri, grass, bushes); Kakariko Village (chickens, workers, people); Lon Lon Ranch (horses); Hyrule Field at night (Stalchildren) *(rep.: field)*; Gerudo Fortress (guards); grass fields cut with a spin attack |
| Skeletons and animation | the market (many animated people); Lon Lon Ranch (horses); the Ganon fight *(rep.)*; Twinrova; Gohma's larvae; Spirit Temple (Iron Knuckles) |
| Big rooms and geometry | Hyrule Field *(rep.)*; Gerudo Valley; Zora's Domain and Zora's River (waterfalls); Death Mountain Trail; Lake Hylia (ours) |
| Translucency and fill (RDP) | Zora's Domain waterfall; Jabu-Jabu's Belly (animated walls); the Forest Temple's twisted corridor; Shadow Temple (fog walls); the water at Lake Hylia (ours); Ganon's Castle barrier |
| Audio | the market (many sound sources); Kakariko (wind, people, music); boss fights |

## Ideas not started (for all scenes)

- A display-list pass (build time) removing state commands that change nothing and idle pipe syncs from every
  scene's and object's lists, proven like the fish rig's (`fishmod/tools/rigcheck.py`'s method: same state at every
  draw).
- Identical-vertex merging inside each vertex load, for every scene list (exact).
- Room-level culling finer than the game's own for the big outdoor rooms (as the lake's areas).

## Skeletons, textures and models (every object, NPC, enemy and Link)

Every skeleton change is proven with the animation harness first (`fishmod/docs/anim-verify.md`): the pose, the draw
stream and the picture, every frame of every animation, before and after. Link first.

| What | Status | Notes |
|---|---|---|
| Limb display lists cleaned at build time: state set to what it already is, pipe syncs with nothing drawn, repeated texture loads (exact) | to do | as the fish rig's (19-27% fewer commands there); proven per list by the rigcheck method; the extracted object lists are known by name, so a build pass can rewrite them safely (unlike patching them in RAM) |
| Identical vertices merged inside each vertex load of every model (exact) | to do | fewer vertices transformed and lit by the RSP |
| Skeleton matrices: limbs that never move relative to their parent drawn under the parent's matrix | surveyed (14 Nov) | see "What the skeletons waste" below: 104 limbs can merge exactly (a matrix saved each), mostly horses, Twinrova and Ganon |
| Skeleton animation on the CPU: frame data decoded once per frame per skeleton, the matrix stack work trimmed | to do | measure first: the profile's costliest skeleton functions |
| Textures: CI8 for 16-bit textures of 256 colours or fewer | all assets (texpack) | in builds with `--texpack`; to put in the bench builds and test on the console |
| Textures: CI4 for those of 16 colours or fewer, I4/IA8 for grey ones | to do | half again the bytes loaded, the same texels |
| Models: lower-detail versions for distance (Link and the common NPCs/enemies) | to do | changes the picture far away: tested by eye |

## What the skeletons waste (14 Nov survey: `fishmod/tools/skel_survey.py`)

152 animated skeletons in the game (animations from the same object), 2,947 limbs: 833 without geometry (joints
only), 1,070 whose rotation never changes in any of their animations. But OoT's rigs are not wasteful the way the
count suggests: most of those are a pattern, a moving joint with no geometry and under it a "holder" limb with the
geometry, at no offset and no rotation (the Tektite: 24 limbs, 9 parts that really move independently, 9 matrices
needed; its 15 fixed limbs are holders and joints). By what merging the never-turning limbs with geometry would save:

| the limb | count | a merge saves |
|---|---|---|
| no offset, no turn (identity), its parent without geometry (the holder pattern) | 452 | no matrix (its matrix is needed for its geometry either way); a push, a transform and a pop on the CPU (~1-2 us) |
| identity, its parent with geometry | 104 | a matrix (CPU conversion, RSP load) and a call, exactly (the same matrix) |
| fixed but not identity, its parent with geometry | 100 | a matrix, approximately (the vertices moved into the parent's frame once: rounding) |
| fixed, not identity, parent without geometry | 29 | nothing |

The 104 exact merges by skeleton: the phantom horse and Ganon's horse 15 each, Twinrova 12, Child Epona 9, Epona,
Ingo's horses, the ranch horses, Zelda's horse and Ganon 8 each, the loach 3, a few others 1-2. Horses draw through
the skin system (Skin_Draw): the harness needs that path before they're touched. Link: 21 limbs, 18 move in his
18,495 animation frames: nothing to merge.

So: the skeletons' matrix count is close to what the motion needs; the gains are per actor (the horses for Lon Lon
Ranch and Hyrule Field), and the CPU's per-limb bookkeeping for holder limbs (small). Each needs the harness for that
actor (posing any actor: the next harness step).

## The fish (14 Nov, ares cycle timers on the bench route)

A rigged fish cost 356 us of CPU a frame; now ~200-230 (its tint table made once, the root's constant part made with
the rig, the swim wave's pivot worked out once a fish, sine and cosine together, bone matrices converted straight from
the rig's 3x4). A far fish (billboard) 96 -> 58 us (corner colours made once, world coordinates under one identity
matrix for all of them). The pictures: the same (studio frames identical; the route's far fish identical). The
fish manager's draw is mostly the props: 4.4 of its 7.0 ms a frame.

## On hold

- Other scenes' optimizations: on hold for now (13 Nov), the list kept here meanwhile.
