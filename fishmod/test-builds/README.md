# Performance test builds

Not releases: two builds with different sets of the CPU optimizations, to compare. Both also include the latest
fixes: fish hold steady at the bobber, put up a real fight, keep off the land (also after being released), and
the records plaque in Link's house continues into a fishing journal. Apply either to the
same **Ocarina of Time NTSC 1.2 (US)** `.z64` ROM as the releases (MD5 `57a9719ad547c516342e1a15d5c28c3d`).

| Patch | Build flag | Optimizations |
|---|---|---|
| `fish-oot-perf-A.bps` | `fishmod/build.sh --opt 1` | 1. the fishing line is one strip under one matrix (was 16)<br>2. limb shortcuts: limbs skip the part-scale math they don't need, and tints are only resent when the body part changes<br>7. the collision line-test table is cleared in large chunks instead of byte by byte (same result; affects the whole game) |
| `fish-oot-perf-B.bps` | `fishmod/build.sh --opt 2` | All of A, plus:<br>3. distant fish that are just swimming around redraw last frame's pose every other frame<br>4. fish far from Link and the bobber think every other frame and glide in between<br>5. at rest, the rod's 22 segments are drawn as 11 pairs<br>6. fish textures are palette (CI8) textures with a palette per species: half the size per draw, same look |

(Optimization 8, no debug work in release builds, was already the case.)

## Measured in ares (debug builds, the bot fishing at the Kokiri shore)

| | CPU update | CPU draw | CPU total | Fish draw | Rod draw | Frame |
|---|---|---|---|---|---|---|
| v0.1.1 | 19.1 ms | 30.9 ms | 49.9 ms | 6.2 ms | 2.4 ms | 52.5 ms |
| A | 18.7 ms | 28.5 ms | 47.2 ms | 4.2 ms | 1.2 ms | 50.6 ms |
| B | 18.7 ms | 27.1 ms | 45.9 ms | 2.4 ms | 1.2 ms | 50.2 ms |

20 fps is a 50 ms frame, and the frame can't go below that, so once the CPU fits, the gain shows up as headroom
(CPU total) rather than a lower frame time. Each sample is a 60-frame average; expect about ±1 ms of noise
between samples.

## What to look for

- A should look exactly like v0.1.1.
- B: far-away fish animate at half rate (it should be hard to notice), and the rod at rest is drawn in pairs
  (its bend is applied per pair instead of per segment). Fish colors should match v0.1.1.
