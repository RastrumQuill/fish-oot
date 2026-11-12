# frames6: Expansion Pak, the debug labels gone, F3DEX3's counters

**`fish-oot-frames6.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --gcc-all`): frames5 plus everything
below. Apply it to the same ROM as the other frames patches and run it the same way (the route, the USB frame log).

**`fish-oot-frames6-profA.bps`, `-profB.bps`, `-profC.bps`**: the same build with one of F3DEX3's profiling microcodes
(`F3DEX3_PROF=A|B|C`, built from upstream F3DEX3 with armips). Each logs a different set of the RSP's own counters in
every frame record, and `fishmod/tools/framelog.py` prints them for slow and full-speed frames:

- A: microseconds of vertex work and of triangle work, RSP stalls on a full RDP FIFO, display list commands run and fetched
- B: vertices and lit vertices, triangles culled by the occlusion plane, clipped, microcode overlay loads
- C: how long F3DEX3 ran, how often the RDP was doing work when sampled (against waiting on memory), small RDP
  commands, matrix loads, RSP stalls on DMAs

The plain build logs F3DEX3's default counters: vertices, triangles in and drawn, rectangles, FIFO stalls. Each
profiling run only needs a lap or two.

## What changed

| change | checked |
|---|---|
| Expansion Pak: the two frame buffers in different 1 MB RDRAM banks (the Z buffer in base RAM): the RDP drawing one and the VI reading the other don't share a bank | the console run (RDP) |
| Expansion Pak: each prop's matrix kept for the scene, where it depends on the prop alone | rebuilt and compared on every use: 47,945 alike; props draw 4.4 -> 3.7 ms in ares |
| No debugger labels in the display lists without debug features (two no-op commands for every actor drawn each frame) | invisible: no-ops |
| The bench's code guard (8 KB of code checksummed a frame, 1.9% of the CPU, the data cache pushed out) opt-in | bench only |
| Audio: the hot note fields in one cache line; the sample buffers' countdowns tracked in a bitmask | audio check: every sample alike (4,620 buffers) |
| guLookAtHilite's answers kept for its inputs (Link asks twice a frame) | collision log alike |
| Moving-platform slot holders filtered by a hash of their address before the search | collision log alike (1,620 frames) |

Tried and dropped: the whole game at `-Os` and with 32-byte function alignment (both hang at start in ares); audio at
`-Os` (slower in ares). The FPU's flush-to-zero was already on (libultra sets it for every thread).

ares, the same route (CPU only; ares's caches make code layout move these by up to about a millisecond):

| ms a frame | frames5 | frames6 (most of it) |
|---|---|---|
| frame | 53.2 | 52.9 |
| CPU draw | 22.9 | 22.0 |
| props draw | 4.4 | 3.7 |
