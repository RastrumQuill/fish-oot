# Optimization plan after the hardware bench

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
