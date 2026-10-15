# Plant card optimizer (design, not built yet)

A headless pipeline, run on the PC (RTX 3060, the `fishoot` conda env), that arranges each plant's billboard cards so
the plant still reads as a solid, three-dimensional plant from every angle the game shows. It keeps the card count and
the hardware cost low. It replaces hand-placed crossed cards. This is the "AI card fitting" job (E), pivoted to the
loss below.

Status: design only. It waits until the fill-rate work and the second hardware bench are done, because the cost term
needs that bench's numbers.

## What is optimized
- **Per card (at most 4):** translation (3), rotation (3, or yaw, pitch and roll), scale (2: width and height),
  and which texture it carries, chosen from the plant's own set.
- **Fixed:** the textures themselves and the game's render settings.
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
| `L_semantic` | SigLIP (pairwise sigmoid loss) | 1 minus the similarity between each rendered view and a target text, such as "a volumetric aquatic plant". A card seen edge-on that thins to a line loses similarity, so that arrangement is penalized. |
| `L_depth` | Monocular depth (Depth Anything V2) | The negative variance of the predicted depth inside the plant's alpha mask. Rewards intersecting cards and overlapping depth layers; penalizes flat, coplanar stacks. |
| `L_multiview` | DINOv2 dense features | The feature distance between adjacent camera angles. A spike means an arrangement that looks right from one angle and breaks when the camera moves. |
| `L_reference` | DINOv2 / LPIPS (proposed addition) | The distance to renders of the detailed reference plant (OBJ+PNG or glTF), from the same views. Keeps the result looking like this species, not like plants in general. |
| `C_hardware` | Fitted cost model (below) | The plant's expected RDP time on the console, over the distances and angles the game actually shows it. |

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
The depth-variance term on its own can be maximized by spreading cards far apart or scaling them up. To prevent that:
- **Size:** cards are confined to the plant's bounding volume, and their scale is bounded.
- **Coverage:** a term matches the plant's silhouette area to the reference's.
- **Cost:** the hardware cost term already penalizes extra pixels.

Clip the depth term, or use it as a hinge (no reward beyond a target spread), so it can't dominate.

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
- which texture each card carries;
- the card count, if not fixed per run;
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

## Open questions
- **Target texts:** one per species ("a cattail", "a water lily") or one shared ("a volumetric aquatic plant"), or
  both.
- **Weights:** start with each term scaled to unit range on random arrangements, then hand-tune on two or three
  species.
- **Do texture choices per card stay in scope,** or only transforms (as specified)?
- **Under-water plants:** they're seen through haze. Their views may need the haze or a separate SigLIP prompt.
