# The vanilla bench

The unmodified game (the decomp as this repository imported it, commit `2a34508`, built for the US NTSC 1.2 ROM),
with only what it takes to play fish-oot's play-bench route at Lake Hylia by itself and time it the way the mod's
frame-log build does: so the two can be compared frame for frame on the same path.

- `vbench.c`, `vbench.h`: the boot (a debug save, adult Link in the Iron Boots and Zora Tunic, the lake full,
  straight into Lake Hylia), the route (fishmod/scripts/tour.inc.c's stops, the mod's blocks and conditions: eight
  laps, each at one time of day and weather), the timing (the frame period, Play_Update and Play_Draw, Play_Update's
  sections and the actors' updates, the RSP and RDP), the CPU sampler (n64prof, one bucket per instruction, samples
  tagged by their frame) and the SummerCart64's USB. The frame records and packets are the mod's
  (src/code/fishmod_debug_pcprof.inc.c): fishmod/tools/framelog.py and n64prof/tools/n64prof.py read both.
- `apply.py`: puts them into a decomp tree, with the hooks in the game's own files (each edit checked).
- `build.sh`: a worktree of the import, `make setup`, `apply.py`, the build: `fishmod/out/vanilla-bench-ntsc-1.2.z64`
  and its ELF.

What differs from the mod's run, besides the mod:
- Where the mod fishes (every third leg, on land), Link turns to the water and stands.
- The bench's own buffers are in the Expansion Pak's RAM above what vanilla uses; the game itself has only its own
  4 MB layout.
- The frame records' fish and props fields are 0, and the first section (the mod's own per-frame work) too.
