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

## The loss

```
L = w_sem * L_semantic + w_depth * L_depth + w_view * L_multiview + w_ref * L_reference + w_cost * C_hardware
```

| Term | Model | What it measures |
|---|---|---|
| `L_semantic` | SigLIP (pairwise sigmoid loss) | Positive and negative prompts (below). Rewards similarity to the species, and penalizes similarity to "video game" looks. A card seen edge-on, thinned to a line, loses the positive. |
| `L_depth` | Monocular depth (Depth Anything V2) | A penalty for perceived depth that differs from the plant's real depth. Not a reward for depth variance (below). |
| `L_multiview` | DINOv2 dense features | The feature distance between adjacent camera angles. A spike means an arrangement that looks right from one angle and breaks when the camera moves. |
| `L_reference` | DINOv2 / LPIPS (proposed addition) | The distance to renders of the detailed reference plant (OBJ+PNG or glTF), from the same views. Keeps the result looking like this species, not like plants in general. |
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
The goal is that the depth model perceives the plant's real shape, not a few flat planes. Maximizing depth variance
would reward spreading cards apart, which is not the goal.
- **The ground truth** is the depth of the detailed reference plant, rendered from the same view. Its depth buffer is
  exact, and the monocular model's estimate of the same render is kept as a check.
- **The penalty** is the difference between the depth the model perceives in the card plant's render and that
  ground truth, inside the plant's alpha mask. Monocular depth is only known up to scale and offset, so the loss is
  scale- and shift-invariant: least-squares align first, then measure (as in MiDaS).
- **A plane check (optional, extra).** Fit up to N planes to the perceived depth inside the mask. If a few planes
  explain it almost perfectly, the plant reads as flat cards: penalize that. No reward beyond the reference's own
  level, so it can't push towards noise.

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
- **Coverage:** a term matches the plant's silhouette area to the reference's.
- **Cost:** the hardware cost term already penalizes extra pixels.

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
- **Depth:** a penalty against the reference plant's real depth, plus an optional plane check. Not variance
  maximized.
- **SigLIP:** positive prompts (the species, including the lotus flower) and negative prompts ("video game" and
  flat looks).
- **Targets:** one per species for now; more phrasings later, since they cost nothing per step.
- **Weights:** set when it's built.
- **Textures:** which texture a card carries is out of scope. Texture offsets in regions of some plants are in scope.

## Open questions
- **Under-water plants:** they're seen through haze. Their views may need the haze in the render, or separate
  prompts.
