# Optimization plan after the hardware bench

## First results (hwbench-1.md, SummerCart, 7 laps)
- **The props are the cost.** The game reaches the 20 fps cap at every stop with props off; with them on it runs
  5-20 fps (12.6 fps on average). The worst stops are 10 (446 props, 5.4 fps), 17, 1 and 11.
- **It's RDP fill.** RDP busy is 85-96%. The extra RDP time per prop ranges from 0.09 ms (far, small) to 0.55 ms
  (near, under water), so the cost tracks the pixels covered, not the prop count. RSP time follows the RDP because
  it waits for the RDP to take its output.
- **What hardly matters:**
  - tex4: +0.2% (texture loads are only 2-6% of RDP time);
  - cold caches: +1.8%;
  - fish drawing: 2.5%.
- **Why fill is expensive here:** the props draw in 2-cycle mode with fog and `AA_ZB_TEX_EDGE2`. Every pixel reads
  and writes Z, and the AA blend also reads the colour buffer, all over RDRAM.

## Next: a fill bench
A/B on hardware, each with a pixel diff in the emulator:
- the current render mode against no AA (alpha compare, no colour read);
- 1-cycle mode with fog baked into the vertex colours;
- no Z update for the props;
- a per-frame budget for covered pixels on near props.

## More fill ideas: the invisible ones go first
1. A custom render mode: TEX_EDGE without `IM_RD`. The VI still anti-aliases the edges from the coverage bits, and
   the RDP no longer reads the colour buffer.
2. No Z writes for props: compare only, with props drawn back to front among themselves.
3. Tighter card outlines. Re-run the annealer with more vertices and a cost weighted for fill, now that the RDP
   is the limit. Pixels with zero alpha still cost Z and memory traffic.
4. Fewer RDP pipe syncs and state changes per prop.
5. 1-cycle mode for props.
6. The no-prop baseline (18-38 ms of RDP): skip the skybox when it's off screen, check the water layers and
   the fill of the lake floor.
7. Bank layout. Colour buffers are at the top of the Expansion Pak and Z is in base RAM, probably already in
   different banks. Check it once.
8. Visibility (item 4) and simpler far props (item 5).
Only if these aren't enough: a budget for near-prop pixels and density tuning, chosen by the pixel test.

## Reordered by these results
Fill comes first. Items 4 (visibility) and 5 help by drawing less. Items 1 and 7 (CPU and cache) and 6 and 8
(texture loads) save little, so they move down. The perceptual CI4 encoder is shelved.

None of these change how the game looks. All of them wait for the first real-hardware results (`hwbench` .sav,
read with `fishmod/tools/hwbench_report.py`). Those results set the order: how RDRAM-bound we are, how much
the 4-bit textures save, and RDP busy vs idle. Each item is measured A/B on hardware before it stays in.

The N64 is limited by RDRAM bandwidth, not by the CPU. Every item cuts bytes moved over the bus, and the CPU cost
has to stay small.

1. **Size-optimized mod code.** The CPU's instruction cache is 16 KB, and a miss reads from RDRAM.
   - Build the mod's own files (`fishmod*`, fish and rod actors) with `-Os` (or GCC `-Os`).
   - No unrolling or inlining in hot loops.
   - Group the hot functions together.
2. **Undo the memory-for-CPU trades.**
   - Per-prop matrices and the sway multiply: one shared matrix per batch, or build them on the RSP.
   - The fasttrig and look tables become short polynomials kept in registers.
   - Cheaper water-map lookups.
   - Reuse display lists instead of rebuilding them every frame.
3. **F3DEX3 microcode** (already `--f3dex3`): make it the default if hardware confirms the win. Use packed normals.
4. **Visibility.** Precompute chunk-to-chunk visibility, and drop chunks hidden by terrain before their props
   are touched.
5. **Simpler far LODs.** Simpler far meshes (meshoptimizer simplifier) and smaller far textures, kept only where the
   pixel test shows no change.
6. **Texture atlases.** Fewer texture loads and fewer pipeline syncs.
7. **Data layout and packed data.**
   - Hot fish and prop fields fit in one 16-byte cache line, read in order.
   - Fields stored as quantized 16-bit or 8-bit values and bitfields, decoded in registers (netcode-style).
   - The same packing for saved fish identity.
8. **Material-sorted draws.** Each texture loads once per frame.

Related compression work, from the same discussion:
- **Perceptual CI4 encoder** (palette and indices optimized together; GPU job on the PC) so more textures can go
  4-bit.
- **ACL-style animation compression**: per-track range and bit-width quantization.
- **Prop vertices as offsets** from the chunk origin, expanded on the RSP.
- **LZ4 / aPLib / Shrinkler** instead of Yaz0 for load times.

## Engine-wide ideas (the whole game, not just the props)
None of these is started. Each is invisible by construction or checked with the pixel test, and measured on the
console before it stays.

1. **The same edge trick for opaque surfaces everywhere.** Vanilla draws terrain and most actors with
   `AA_ZB_OPA_SURF`, which reads the colour buffer for every pixel. Libultra's "reduced aliasing" modes (`RA_ZB_OPA_SURF`)
   write coverage and let the VI smooth the silhouettes. This is the props' change applied to `Gfx_SetupDL` and the
   room meshes' render modes. The risk is internal edges (one surface ending in front of another), so it needs the
   same side-by-side check at many places, not just the lake. It's likely the biggest RDP win across the whole game.
2. **Frame and depth buffers in different RDRAM banks** (the N64 manual's advice). It helps every pixel drawn
   anywhere. Check where each sits in each memory layout (with and without the Expansion Pak).
3. **A build-time display-list pass over the vanilla scenes:** drop redundant pipe syncs and repeated state and
   texture loads, and merge identical materials. Same picture, fewer RDP stalls and commands.
4. **Finer room culling.** Vanilla culls room meshes by coarse bounding spheres. Per-part frustum culling, and
   precomputed visibility where rooms are big (Hyrule Field, Lake Hylia), would draw less.
5. **The whole game built with GCC** (`-O2`, or `-Os` for the instruction cache) instead of IDO. The Makefile
   already has a GCC path, and HackerOoT ships GCC builds. It frees CPU time for the fish AI and the rest. It needs a
   pass for undefined behaviour IDO tolerated.
6. **F3DEX3 by default** (`--f3dex3` exists): faster vertex work on the RSP, for every scene.
7. **CPU hot spots:** collision queries (BgCheck), the software matrix stack, actor update overhead. Profile first.
   The few functions that run the most can then be hand-written in MIPS assembly, sized to fit the instruction cache
   and skipping the compiler entirely (as done in SM64 optimisation work). This only makes sense once a profile names
   them, and each one needs a C version kept beside it to check the assembly against.

What the hardware bench showed decides the order: the RDP is the limit in the lake, so 1–4 come first. 5–7 matter
where the CPU is the limit (busy scenes, lots of actors, the fish AI).

### GCC status
`fishmod/build.sh --gcc O2` (or `Os`) compiles and links the whole game with mips-linux-gnu-gcc 12 (strict aliasing
off), but the ROM hangs at boot. ares reports that the CPU never sent the PIF its boot-termination command. That
happens in libultra's `osInitialize`, so the hang is in the boot code or libultra, before the game starts. Next step,
if pursued: GCC only for `src/code` and the overlays (where the CPU time goes), with the boot code, libultra, libc
and audio staying on IDO; or port HackerOoT's GCC fixes.

### The edge trick, tested exactly (edge_ab_capture.py)
Raw VI frames from ares, scene frozen, the two edge modes alternating at all 22 tour stops:
- **Every prop in the new mode:** not invisible. Far props are a few pixels across and mostly edge; without the
  blend their partly covered pixels drop out. Small lotus flowers vanished (stops 10, 22) and far dock posts thinned
  (stop 3).
- **The new mode within 500 units only** (now the default): the far differences are gone. Near plants differ in
  scattered single edge pixels (up to about 1% of the screen by more than the VI's own noise), which can't be told
  apart side by side at 6x.
- **Implication for the engine-wide version:** small or distant geometry is where it shows, so it would need the same
  near/far split, and the same capture at many places, before going in.

### Fallback if the invisible fixes aren't enough: plants as solid geometry
Not a budget and not thinning, which are both ruled out. Instead, rebuild the costly near plants as low-poly opaque
meshes, the way vanilla's Kokiri grass bushes are made, in place of cut-out cards:
- **No see-through pixels:** a card still fills the empty parts of its texture inside the outline; a mesh fills only
  its silhouette.
- **Back faces culled:** a closed mesh can use back-face culling, so its far side costs nothing; cards can't, being
  seen from both sides.
- **Cheaper per pixel:** an opaque surface needs no alpha-derived coverage, and could go to 1-cycle mode where fog
  allows.
- **The cost is triangles,** which the RSP has room for while it waits on the RDP.

Candidates are the plants that fill the most at the worst stops: weed clumps, broadleaf and bushes. Kept for later,
if the bench-driven fixes leave the worst stops short.

## Second results (hwbench-2.md, SummerCart, 1 lap + 3 stops)
- **Far props cost twice what near ones do.** Of the props' 54.7 ms of RDP time, 34.5 ms is props past 500 units and
  17 ms is props nearer. At the worst stops it's more lopsided: stop 10, far about 136 ms against near 45 ms;
  stop 11, 110 against 58; stop 1, 87 against 42. Far props are small on screen, so the cost is per triangle and
  per short span (each scanline's run starts with a memory fetch), not per pixel.
- **Edge mode for near props:** the old mode everywhere costs 10% more RDP time (91.2 against 82.8 ms). Kept.
- **1-cycle:** 1% faster. The RDP is limited by memory, not by its pipeline. Not worth building.
- **No depth writes:** 4% slower (fewer hidden pixels rejected). Dropped.
- **Skybox:** 5.4 ms on average. **Water surface:** 0.1 ms.
- **The first bench's prop counts were off** at the under-water stops (stop 2: 191 against 348). The same v1 ROM
  in ares draws 348, like v2 on the console, so per-stop comparisons with hwbench-1 aren't reliable there.

### What follows
1. **Far props, cheaper and the same to look at:** a far LOD per kind (one card, or fewer, with a short outline of
   4-6 vertices), switched at a distance the pixel test picks so the switch can't be seen. The card optimizer's
   2/3/4-card tiers are this. The tighter outlines (more vertices) add triangles to every far prop, so they should be
   for near props only.
2. **Skybox when it can't be seen** (5.4 ms): skip it when the terrain and water cover the screen. Needs a reliable
   test for "covered".
3. 1-cycle and no depth writes are dropped.

## Reading hwbench-4 (decided before the results)
Two saves: `fish-oot-hwbench4` (IDO) and `fish-oot-hwbench4-gcc`. Read each with `hwbench_report.py save.sav`, and
the pair with `hwbench_report.py ido.sav --vs gcc.sav`. The bench's run-to-run noise was about 1-2% of mean RDP time,
so a change smaller than 2% counts as none. Each mode is the old way of doing one thing, so its change against base is
what the new way saves (a positive change means the new way wins):
- **notri** (one triangle per small-plant card): keep if notri's RDP time is 2% or more above base. If it is, the lily
  pads and the lotus flowers' cards get one triangle too. Within 2%: keep it anyway (fewer vertices for the RSP and the
  CPU, no visible change). Clearly below base: put the cut outlines back for near props.
- **nostrict** (nearest props strictly nearest first): the same rule. Within 2% it goes, since the sort costs CPU.
- **trimfar** (cut outlines for far props instead of uncut cards): keep the uncut far cards unless trimfar is 2% or
  more below base.
- **altedge**: expected about +10% as in hwbench-2. A different answer means the new cards changed the edge cost.
- **nonear / nofar / noprops / nosky**: no decision; they say where the remaining props' time is, which picks the next
  optimization (far props still dominant: a far LOD per kind; near props: the solid-geometry fallback).
- **GCC against IDO**: GCC stays the default if the CPU's update and draw times drop and the GCC save shows as many
  laps without a crash. Its RDP time should be unchanged (within 2%); if it isn't, look at why before keeping it.
- **A crash** in either save (the breadcrumb shows where): that build's result is read up to the crash, and the crash
  is fixed before anything else.

## Fourth results (hwbench-4.md, SummerCart: IDO 12 laps, GCC 15 laps, no crash in either)
Mean RDP time over the 22 stops (GCC save; the IDO save agrees within 1 point everywhere):

| | RDP | against base |
|---|---|---|
| base (as shipped in 5e) | 63.1 ms | |
| far props cut to outlines (`trimfar`) | 80.0 ms | +27% |
| `notri` | 81.8 ms | +30% |
| old edge mode (`altedge`) | 71.3 ms | +13% |
| no strict nearest-first order (`nostrict`) | 63.1 ms | 0.0% |

- **A flaw in the `notri` mode:** it also gave the other kinds' far props their cut outlines, so it holds `trimfar`'s
  cost too. The single triangle's own share is `notri − trimfar`: +1.8 ms (2.8%) in both saves, up to 4-5 ms at the
  worst stops (1, 10, 11, 19).
- **Since hwbench-2:** base RDP 82.8 → 63.1 ms (−24%), the props 54.7 → 36.2 ms. The uncut far cards are most of it.
- **Where it goes now:** props 36 ms (near 17, far 16.5), skybox 5.3 ms, everything else 22 ms. Without the props
  every stop holds 20 fps.
- **The worst stops** are still props: 10 (129 ms, 106 of it props), 11 (133 / 103), 17 (109 / 82), 1 (103 / 72),
  12 (102 / 78). There both near and far are 30-60 ms each.
- **GCC:** CPU update −14%, CPU draw −16%, RDP −2% (feeding it sooner), frame −1% (the frame is the RDP's).

### Decisions (by the rules set before the results)
- Uncut far cards: kept. One triangle per small-plant card: kept (+2.8% when off).
- Strict nearest-first order: dropped (no difference; the sort cost CPU). The code is gone.
- Edge mode for near props: kept, as hwbench-2 found.
- GCC: now the default (`build.sh`; `--ido` for the old compiler).
- **Lily pads as one triangle: not done.** Vanilla's pad is a 3-triangle fan, and its normals light one corner (two
  vertices face up, three have zero normals), so its shading is piecewise. One triangle would change that shading
  by up to 35% of the sun's share somewhere on the pad: not invisible. (Its fan also cuts off a corner of the
  texture, which a triangle could keep.) The lotus flowers and centres already have one triangle.

## What Majora's Mask does (zeldaret/mm, RastrumQuill/mm)
MM's answer to a lush field is `ovl_Obj_Grass` (Termina Field's grass):
- **Bushes are opaque solid meshes,** one shared display list, not cut-out cards: our solid-geometry fallback is
  what MM shipped.
- **Groups:** the bushes come in groups. Each group is tested once (distance under 1280 and a bounding check
  against the view, `func_809A9110`) and binds the scene's lights once for all its bushes.
- **Distance:** opaque within 1080 units, then a translucent fade to nothing by 1180 (a pop-free cutoff). Beyond
  that there's no grass at all. MM keeps fields lush only near the camera; the distance is bare ground.
- **What we already do better:** batched per-area props, far LOD cards, per-prop view test, material sorting.
- **What to borrow, if needed:** (1) solid meshes for the near bushy plants (weed clumps, broadleaf), the fallback
  above; (2) the fade instead of a pop at the far cutoff (costs a translucent pass, so only for a thin band).
The meshes themselves are in MM's assets, which need an MM ROM to extract; the code is readable without one.

## Far tiers (after hwbench-4)
Every prop is drawn one of four ways, chosen per prop when the set is built (`Scatter_Tier`):

| Tier | Where | What |
|---|---|---|
| near | within 500 | the cut cards (or one triangle per card), edges by the VI |
| far | 500 on | the whole cards, edges anti-aliased in the RDP |
| half | from where a texel of the full texture is under half a pixel (`half_distance`, per kind, times the prop's scale) | the same whole cards on the texture at half size (`...DLFarH`) |
| single card | from where the plant is at most 16 pixels tall (`IMP_PX`), and only seen within 20 degrees of the side | one card with a picture of the whole plant (`fishmod/tools/impostor.py`: rendered from its mesh, texture and vertex colours, from the yaw showing most of it), one clamped triangle, turned about its up to face the camera |

- **The dissolve:** for 15% past a kind's distance its props are single cards, faded by the primitive alpha
  against the RDP's dithered alpha test: in the opaque pass, at opaque cost, no blending. It replaces the pop at the
  cutoff.
- **Seen from above:** a single card that turns only about its up is a sliver from above. The first A/B capture
  showed it (stop 10: the grass on the bed, seen from the shore, thinned out), so single cards are only used where the
  view to the plant is within 20 degrees of level.
- **Bench v5** (`hwbench.inc.c`): base, noimp, nofartex, nodissolve, zsame, nonear, nofar, noprops, nosky. `zsame`
  draws depth into a buffer in the frame buffers' 1 MB bank (debug builds reserve it below them): the depth buffer
  normally shares the 2nd MB with the RDP's command buffer and the display lists, the frame buffers have the 8th to
  themselves. It measures how much the bank layout matters at all.

## LOD tuning, first run (lodtune3: all 13,584 view-times, `lodtune-1.txt`)
3,396 views of Lake Hylia (above and under water, looking down, level and up) at dawn, noon, dusk and midnight. For
each kind: the shortest cut, single-card and half-texture distances that changed no view by more than 2 pixels (a
pixel counted when a channel moves more than 2 of 31). The table keeps the most any view needed.

- **Above water, almost nothing can go without some view seeing it.** Most kinds need their full 3000 for the cut,
  and the single card needs 3000 for nearly every kind: the lake is open, plants on the far shore are visible, and
  the single card changes a few pixels somewhere in some view however far away it starts. The shipped cutoffs
  (800-1600) are far inside what this criterion asks: that is the pop-in.
- **Under water the haze does hide things, but later than assumed:** cut distances 50-2000 (grass 950-1900, ribbon
  weed 1050-2000, kelp 1800) against the shipped 500-900. The single card and half texture can start at 350-2000
  depending on the kind; rushes, marigolds and bank roots can't be seen under water at all.
- **Time of day matters:** dusk needs the least almost everywhere (e.g. grass above water: 2200 at dusk against 3000).
- **The criterion is strict:** the most any of 13,584 views needed, at 2 pixels. One view where a far plant's few
  pixels change sets the table. Only the maximum was stored, not how many views needed how much, so this run can't say
  what a looser line (a few dozen pixels, say) would allow.
- **Used:** with the camera under water, each kind's cut, single-card and half-texture distances come from the
  table (`Scatter_TunedDistances`: of the two measured times either side of the clock, the further), at the prop's
  scale. Above water the hand-set distances stay: the tuning's views there (looking across the lake from anywhere,
  at any pitch) were harsher than anything the game's camera does, so those numbers are set by eye instead.

## Frame-rate pass: a solid 20 fps from in-game views (steps 1-4 of 5)
The goal is 20 fps, solid, from views like the game's own. `section_budget.py` estimates the console's frame time
for 240 views per lake section (the rod camera from the shore, swimming at the surface, diving), with
`shot_search.py`'s cost model: the scanlines each prop's cards cover, fitted to hwbench-4 (near 0.00326, far 0.00242
ms per card-scanline, 3.2 ms rms per stop). The model is pessimistic: it leaves out the far single cards and half
textures, and the skip for plants the camera is inside.

What it showed: the props are the whole problem (the no-prop time is 19-37 ms everywhere), and three tall kinds made
most of it: kelp (260 tall, 3 cards), tall weed (190, 4 cards) and ribbon weed (6 cards), from every camera, even
above water through the surface.

| Step | Views over 50 ms |
|---|---|
| Before | 51% |
| 1. Fewer cards: kelp 1 (turned to face the camera, `faceCam`), tall weed 2, ribbon weed 3 | (together, below) |
| 2. Shorter: kelp 260 -> 180, tall weed 190 -> 140 | (together, below) |
| 3. Tall kinds as accents: kelp and tall weed a third as many; ribbon weed keeps its beds' edges (half), the middles (a sixth kept) turn to grass and broadleaf (`gen_scatter.py accents`, by a hash of position, so the rest of the layout doesn't move) | (together, below) |
| 4. Under water: tall kinds cut at 900 (`UNDER_CUT_TALL`), the haze a little nearer (`HAZE_FOG_NEAR` 953 -> 945) | (together, below) |
| After 1-4 | 12% (p95 57.9 ms) |

Still heavy: West weed flat (60% of views), North beach shallows (39%), Dry dock (22%), East shore lilies (21%).
Mostly diving among mid-height plants.

**Not done (the next squeeze, each a small change of look; the model's estimate of views over 50 ms):** pondweed 4
-> 2 cards (10%), grass tuft patches 8 -> 4 cards (7%), bush 6 -> 4 (6%), broadleaf 3 -> 2 (5%), vine strands 2 -> 1
(4%).

**Submerged plants from above water** (not done; the next candidate): the game draws every plant under the surface
even where the surface hides it (a swimming view over the West weed flat: 75 ms estimated, almost nothing visible).
Cutting submerged plants at 600 from the camera while it's above water: fishing views over 50 ms 4% -> 0%,
swimming 11% -> 4% (the model's estimate). Needs an A/B capture to check nothing visible goes.

After 1-4 the diving views are what's left (21% over 50 ms), mostly among mid-height plants.

**Step 5, a per-frame plant budget** (the game estimates its own plant cost each frame the same way and fades the
furthest plants out when over): the only way to guarantee a solid 20 in every view. Held back until everything else
is squeezed.

**Next LOD tuning run (the compute package):** the meshes changed (kelp, tall weed, ribbon weed), so the tuned
under-water distances need re-running. This time no hard limit: store, per kind, view and candidate distance, the
percentage of the picture that changed, so the cut-off can be chosen afterwards (and differently per kind) without
running it again.
