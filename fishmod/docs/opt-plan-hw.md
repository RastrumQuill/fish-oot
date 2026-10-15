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
