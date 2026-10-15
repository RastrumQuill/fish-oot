# Plant card optimizer (design, not built yet)

A headless pipeline, run on the PC (RTX 3060, the `fishoot` conda env), that arranges each plant's billboard cards so
the plant still reads as a solid, three-dimensional plant from every angle the game shows. It keeps the card count and
the hardware cost low. It replaces hand-placed crossed cards. This is the "AI card fitting" job (E), pivoted to the
loss below.

Status: design only. It waits until the fill-rate work and the second hardware bench are done, because the cost term
needs that bench's numbers.

## What is optimized
- **Per card (at most 4):** translation (3), rotation (3, or yaw, pitch and roll) and scale (2: width and height).
- **Texture offsets in certain regions, for some plants:** a region of a card can shift its texture coordinates, so
  that part shows a different stretch of the plant's texture. That breaks the repetition between cards that carry
  the same texture.
  - Each region is a separate polygon with its own UVs, so it costs triangles, which the cost term counts.
  - The offsets are bounded so the region stays inside the texture, unless that texture is set to wrap.
  - Which plants and which regions get offsets is set per species.
- **Not in scope:** which texture a card carries (each card keeps its own), the textures themselves, and the game's
  render settings.
- **Afterwards:** each card's outline is cut by `card_outline.py`, as now.

Three runs go in parallel, with 2, 3 and 4 cards. The converged losses pick the base asset, and the variants with
fewer cards become its LOD steps for middle distances. The game's LOD switch distances then come from the existing
pixel calibration (`lod_calibrate.py`), so a step change is only made where it can't be seen.

## The idea: a conceptual ground truth
There is no ground truth: no detailed model of each plant to match. Instead, the cards have to convince several
independent models, each from a different domain:
- **meaning:** SigLIP sees the species, not a game asset;
- **geometry:** the depth model perceives a volume, not flat planes;
- **structure:** DINOv2's features stay steady as the camera moves.

Each model can be fooled on its own. Fooling all three at once, from many views, comes close to being the real
thing. Their agreement stands in for the ground truth.

## The loss

```
L = w_sem * L_semantic + w_depth * L_depth + w_view * L_multiview + w_cost * C_hardware
```

| Term | Model | What it measures |
|---|---|---|
| `L_semantic` | SigLIP (pairwise sigmoid loss) | Positive and negative prompts (below). Rewards similarity to the species, and penalizes similarity to "video game" looks. A card seen edge-on, thinned to a line, loses the positive. |
| `L_depth` | Monocular depth (Depth Anything V2) | A penalty when the perceived depth is just flat planes. Not a reward for depth variance (below). |
| `L_multiview` | DINOv2 dense features | The feature distance between adjacent camera angles. A spike means an arrangement that looks right from one angle and breaks when the camera moves. |
| `C_hardware` | Fitted cost model (below) | The plant's expected RDP time on the console, over the distances and angles the game actually shows it. |

The weights get set when it's built: each term is scaled to unit range on random arrangements, then tuned on two or
three species.

### The semantic term: positive and negative prompts
`L_semantic = mean over views of [ -sim(view, positive) + sim(view, negative) ]`, using SigLIP's sigmoid scores (each
prompt is scored on its own, so positives and negatives don't compete in a softmax).
- **Positive: the species.** "a cattail", "a water lily", "a lotus flower", "a clump of rushes", and so on. Every
  plant in the mod has one, the lotus flower included.
- **Negative: video-game and flat looks.** For example "a video game plant", "a low-poly game asset", "a flat
  cardboard cutout", "a 2D sprite", "a billboard texture". The renders are low-resolution and fogged, so the
  negatives have to name flatness and game-ness specifically, not just "bad".
- **One target per species for now.** Later it can use several phrasings per species ("a cattail", "bulrushes by a
  lake", "reeds in shallow water"), with one chosen at random each step, or the scores averaged. The text embeddings
  are computed once at the start, so extra phrasings cost no GPU time per step, only a little memory.

### The depth term: real depth, not flat planes
The goal is that the depth model perceives a volume, not a few flat planes. It isn't asked to maximize depth
variance, which would reward spreading cards apart. There's no reference depth, so the term penalizes flatness:
- **Plane check.** Fit up to N planes to the perceived depth inside the plant's alpha mask, N being the card count.
  If they explain it almost perfectly, the model sees what is really there, flat cards, and that is penalized: the
  penalty falls as the fit's leftover error grows.
- **Capped.** Once the leftover error reaches the level of a real plant photo or a detailed render, the term gives
  nothing more. That level is measured once, on a handful of such images. So the term can't push towards noise or
  scattered cards.
- **Steady between views.** The perceived depth of adjacent views should agree once the camera's move is undone.
  Flat cards seen from two angles give two different flat layouts, a solid shape gives one. This is optional, and
  costs a reprojection per view pair.

### Views
- **Angles:** off-axis cameras at several heights and distances, not one level orbit. Straight-down views are left
  out of the loss, so the cards' vertical edges aren't penalized unfairly.
- **Weighting:** views are weighted by how the game shows plants. Most are seen from the shore (above, looking down
  at a slant) and from under water at about plant height. The tour stops and the scatter's draw distances give the
  distribution.

### Render like the console
The discriminators should judge what the player sees, so the differentiable renderer copies the game's settings:
- 320x240 output;
- the N64's 3-point texture filtering;
- the cut-out threshold (alpha at least half, `TEX_EDGE`), with a soft threshold during training;
- fog and under-water haze;
- the prop's vertex colours and tint.

A plant judged only on clean, high-resolution renders can come out tuned for detail the console never shows.

### Guards against degenerate solutions
Any of the terms could be gamed by spreading cards far apart or scaling them up. To prevent that:
- **Size:** cards are confined to the plant's bounding volume, and their scale is bounded.
- **Coverage:** the plant's silhouette area stays near what the species is given in the scatter (its height and
  spread).
- **Cost:** the hardware cost term already penalizes extra pixels.

### Guards against fooling the models
The optimizer can read the models' gradients, so it could find arrangements that fool them without looking any
better to a person (adversarial examples). The design is already hard to fool this way, because:
- **Few parameters.** About 8 per card plus a few texture offsets: no free pixels to fill with noise patterns.
- **Many views,** each jittered at random every step (angle, distance, fog, small shifts), so an arrangement has to
  hold up across all of them, not exploit one image.

Two checks on top of that:
- **A held-out judge.** A model that isn't used in training (CLIP next to SigLIP, a second depth model such as
  Marigold or ZoeDepth) scores the result at the end. If the trained models are happy and the held-out one isn't,
  they were fooled.
- **Ensembles** (two SigLIP sizes, for example) if a single model turns out to be easy to fool.

### More judges (candidates, held out or used in training)
Each comes from a different domain or training set from the three trained models, so fooling it too is independent
evidence.

| Judge | Domain | What it asks | Cost |
|---|---|---|---|
| BioCLIP | species, trained on iNaturalist-style biology images | Does it name the right species (e.g. *Typha*, *Nelumbo*)? | cheap |
| A VLM (Qwen2-VL, Florence-2, PaliGemma), yes/no | language | "Is this a real 3D plant or flat cutouts?"; read the probability of yes | slow: judge only |
| Surface normals (DSINE, Marigold-normals, Omnidata) | geometry, another family | Flat cards give one uniform normal per card; a volume gives varied normals | medium |
| Multi-view reconstruction (DUSt3R / MASt3R, VGGT) | 3D | Reconstructed from the views together: a volume, or a few planes? | slow: judge only |
| Optical flow (RAFT) over small camera moves | motion | Flat cards move as a few homographies; a volume shows varied parallax. In-game the camera always moves | medium |
| Orbit flicker | time | LPIPS between frames of a smooth orbit; spikes where a card goes edge-on or pops | cheap |
| Photo vs. game discriminator | style | A small classifier trained on real plant photos against game renders (the "plant / not plant" idea); or KID against a photo set of the species | cheap once trained |
| Console render | hardware | The finished asset rendered in ares (the real RDP and VI), scored by the judges above: is the differentiable renderer's approximation honest? | slow: final check |
| You | people | Blind A/B pairs on a page; enough votes can train a small reward model (RLHF-style) for later species | your time |

Proposed held-out set: BioCLIP, the VLM question, normals and the console render. Optical flow and the orbit
flicker are cheap enough to join the training loss if the multi-view term misses popping.


## The hardware cost analog
"How much it costs to run on real hardware" should be a number the optimizer can use, fitted to the console's own
measurements rather than guessed.

**Model.** Per plant, averaged over the views it's seen from:

```
C = k_px * covered_pixels + k_tri * triangles + k_state * material_changes
```

- `covered_pixels` counts every pixel inside every card polygon, including see-through texels inside the outline,
  because the RDP still pays for those.
- Overlapping cards count each layer.
- Pixels past the first layer cost less when they fail the depth test (a depth read, no writes). That calls for a
  separate `k_px_hidden` once the bench separates the two.

**Fitting the constants.** These come from the hardware bench:
1. For each tour stop, `overdraw.py --props` gives the covered pixels and triangle count for exactly what the game
   draws there. It needs a small extension to report triangles and layers.
2. The bench gives each stop's prop cost on the console: RDP time (base) minus RDP time (noprops). The second bench
   splits it further with its nonear and nofar modes, and its 1-cycle, no-depth-write and edge-mode variants.
3. A least-squares fit of the stops' RDP milliseconds against pixels, triangles and material changes gives `k_px`,
   `k_tri` and `k_state`. That's 22 stops times 3 splits, about 66 points for 3 to 4 unknowns. The leftover error
   says how far to trust it.

What hwbench-1 already shows: per prop, 0.09 ms (far, small) to 0.55 ms (near, under water). The cost follows the
pixels, so `k_px` will dominate.

**During training,** the cost is computed in the renderer itself, from the projected card areas at the game's view
distances, so it's differentiable and free. **After training,** each chosen asset goes into the game, and the next
overnight bench run checks the real cost at the tour stops. That closes the loop and refits the constants.

## RL or gradients
The card transforms are continuous, and with a differentiable renderer the whole loss can be differentiated with
respect to them. Plain gradient descent (Adam, several random starts) is the fastest and steadiest way to fit those.
Use RL or an evolution strategy (PPO, or CMA-ES, which is simpler) where gradients don't exist:
- the card count, if not fixed per run;
- which regions of a card get texture offsets (the offsets themselves are continuous, so they go to the gradient
  loop);
- the outline vertex budget;
- the real hardware measurement, which can't be differentiated.

Proposed: CMA-ES or a small PPO policy on the outside makes the discrete choices and restarts; Adam on the inside
fits the transforms for each choice. The reward is the final loss.

## Running it on the PC
- **Renderer:** nvdiffrast works on Windows with CUDA. PyTorch3D is awkward on Windows. Alternatively, a small custom
  PyTorch rasterizer: the cards are textured quads, so a soft rasterizer for quads alone is short.
- **VRAM, fp16:** SigLIP base (about 400 M parameters), DINOv2-B and Depth Anything V2-Small fit in 12 GB together,
  with views batched at 224 px. The 320x240 console render is upscaled for the models, so they see its blockiness.
- **Workload:** one plant species is a few thousand steps across three runs, 2, 3 and 4 cards. It runs as a job pack
  through `fishmod/pc/run_jobs.py`'s runner, or as a separate script in the same environment.
- **Output:** per species and card count, the card transforms as JSON. `gen_habitat.py` turns them into the mesh, and
  `card_outline.py` and the pixel LOD calibration run as now.

## Decided
- **No ground truth.** Fooling several independent models (meaning, geometry, structure) stands in for one.
- **Depth:** a penalty for flat planes, capped at the level of real plants. Not variance maximized, and no
  reference depth.
- **SigLIP:** positive prompts (the species, including the lotus flower) and negative prompts ("video game" and
  flat looks).
- **Targets:** one per species for now; more phrasings later, since they cost nothing per step.
- **Weights:** set when it's built.
- **Textures:** which texture a card carries is out of scope. Texture offsets in regions of some plants are in scope.

## Open questions
- **Under-water plants:** they're seen through haze. Their views may need the haze in the render, or separate
  prompts.
