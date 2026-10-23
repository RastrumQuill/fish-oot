# Test 5i: what a prop costs

## The patches
- **`fish-oot-hwbench-5i.bps`**: the hardware bench (needs a fresh save; send the `.sav`). It stops by itself after
  15 laps (about 8 hours) and shows "BENCH DONE": then it can be turned off.
- **`fish-oot-hwbench-5i-f3dex3.bps`**: the same with the F3DEX3 graphics microcode (a faster RSP). Run it on
  another night, also from a fresh save.

## Why
Bench 5h showed that each prop costs about 0.07-0.11 ms on top of the pixels it draws, so the 330-odd moss mats at
the slow stops cost ~40 ms by themselves. This bench measures where that cost goes, with three new modes at each
stop:
- **nosway**: no sway matrix (one matrix per prop instead of two).
- **mtxonly**: each prop's colour and matrices are sent, but not its triangles (the per-prop setup alone).
- **nomoss**: no moss mats.
The rest is as before: as shipped, near props only, far props only, no props, no props and no sky. If the F3DEX3
build is much faster where there are many props, the RSP (vertices and matrices) is the limit, not drawing.

## What changed since 5h
- **Moss:** fewer, bigger mats (8-sided, about half again as wide), in clumps of 1-3; none past 600 units (500 under
  water).
- **Thinning** redone with the model refitted on 5h, which charges each prop its fixed cost: more plants became moss
  or went (mostly broadleaf and pondweed in the dense flats). On spots it wasn't tuned on, every lake section's
  worst direction is now under 47 ms by the estimate.
