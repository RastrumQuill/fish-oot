# frames18

**Console, 14 November (run 20261114-1127):** hitches 24.5% of pictures, the fewest of any build (frames17 30.5%,
frames16 32.7%, frames14c 27.9%); frames over 52 ms 38% (frames17 46%); frame period 53.6 ms (frames17 54.4);
CPU update -0.8 ms and CPU draw -0.7 ms against frames17; RDP 44.0 ms, unchanged: still the limit.

**`fish-oot-frames18.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --meshpack --gcc-all`): frames17
with the game's code and data pinned in the CPU's caches. The PC profile and frame log as in every bench build.
Compare with frames17 (run 20261114-1035) and frames16 (20261114-0923).

**Why:** the CPU's caches are direct mapped (16 KB of instructions, 8 KB of data): where code and data sit decides
which of them push each other out. The mod's code sat in the middle of the code segment, so every edit to it moved the
engine's code after it (the frame's graphics task, the matrices, all of the audio), and the segment's data, read-only
data and zeroed data after all of its code, and the heap (every actor's code and memory) after the segment. The
console runs showed it: frames15's fish update +2.5 ms from nothing the fish did, frames17's CPU update +0.7 to
+2.4 ms on legs far from the dock it changed.

**The pins:**
- the mod's code files last in the code segment (spec), then fishmod_pin.c: one aligned item per section, so each
  section ends on a fixed boundary (code 16 KB, the data sections 8 KB) and the next starts at the same cache place;
- the heap from a 16 KB boundary (main.c);
- the fish overlay given a whole number of 16 KB (z_actor.c), and each fish a fixed 0x600 bytes of memory
  (z_en_fishmod.h): the actors loaded after them keep their places when the fish code or struct grows.
Padding: up to about 56 KB of RAM.

**ares, the bench's frames 1,200-2,400, PC samples:** the same code built three ways (as is; with 1 KB more mod code;
with 1 KB more fish code): the CPU's spare time 17,105 / 18,239 / 18,251 samples (before the pins, the same test:
10,110 / 16,670 / 10,864); audio, collision, matrices and the actor engine within 3% across the three. Spare time is
also more than frames16's (14,982) and frames17's (10,031) in the same frames. The fish update still moves a little
with the fish code's own layout (4.47-4.90 ms).

**Props never thinned while playing:** the props' per-frame caps (900 props, 200 see-through, 256 material changes)
are now checked at build time (gen_scatter.py): the most props within draw distance of any spot on the lake, in every
direction, must fit, or the build stops. Lake Hylia: 503 / 41 / 198. On the bench route in ares the caps never left
a prop out (debug counters, gFishmodScatterDropped: 0 / 0 / 0; at most 225 props in view).
