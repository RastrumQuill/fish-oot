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

## Ideas not started (for all scenes)

- A display-list pass (build time) removing state commands that change nothing and idle pipe syncs from every
  scene's and object's lists, proven like the fish rig's (`fishmod/tools/rigcheck.py`'s method: same state at every
  draw).
- Identical-vertex merging inside each vertex load, for every scene list (exact).
- Room-level culling finer than the game's own for the big outdoor rooms (as the lake's areas).

## Skeletons, textures and models (every object, NPC, enemy and Link)

| What | Status | Notes |
|---|---|---|
| Limb display lists cleaned at build time: state set to what it already is, pipe syncs with nothing drawn, repeated texture loads (exact) | to do | as the fish rig's (19-27% fewer commands there); proven per list by the rigcheck method; the extracted object lists are known by name, so a build pass can rewrite them safely (unlike patching them in RAM) |
| Identical vertices merged inside each vertex load of every model (exact) | to do | fewer vertices transformed and lit by the RSP |
| Skeleton matrices: limbs that never move relative to their parent drawn under the parent's matrix | to do | fewer matrix loads and multiplies (CPU and RSP); exact when the limb's own transform is the identity in every animation it plays |
| Skeleton animation on the CPU: frame data decoded once per frame per skeleton, the matrix stack work trimmed | to do | measure first: the profile's costliest skeleton functions |
| Textures: CI8 for 16-bit textures of 256 colours or fewer | all assets (texpack) | in builds with `--texpack`; to put in the bench builds and test on the console |
| Textures: CI4 for those of 16 colours or fewer, I4/IA8 for grey ones | to do | half again the bytes loaded, the same texels |
| Models: lower-detail versions for distance (Link and the common NPCs/enemies) | to do | changes the picture far away: tested by eye |

## On hold

- Other scenes' optimizations: on hold for now (13 Nov), the list kept here meanwhile.
