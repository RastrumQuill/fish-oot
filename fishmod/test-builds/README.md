# Test builds

Not releases. Apply to the **Ocarina of Time NTSC 1.2 (US)** `.z64` ROM (MD5 `57a9719ad547c516342e1a15d5c28c3d`).

## Test 2 (latest)

| Patch | Build command |
|---|---|
| `fish-oot-test2.bps` | `fishmod/build.sh --opt 1` (build A's optimizations) |
| `fish-oot-test2-testmode.bps` | `fishmod/build.sh --opt 1 --testmode` |

The test-mode patch is the same game, but a **new file** skips the intro and starts with three empty bottles and
one with bugs, the slingshot (30 seeds), the Goron's Bracelet, Kokiri Sword and Deku Shield, sticks, nuts and
200 rupees. Existing files are not changed.

New since build A:
- **Fish-on music:** a fast, repeating bass line with a tense (not combat) feel replaces the area music while a
  fish is hooked; the area music comes back when it's landed or lost.
- **Fights:** the rod bends toward the fish and harder the more the pull is across it, and shakes during runs;
  Link's arms lean toward the pull. Fish mix runs away, sweeps across the line, and charges back toward you that
  throw slack into the line, and they go deep between runs.
- **Landing a fish:** item fanfare, the camera swings round to the front of Link holding the fish up, and a
  textbox names it with its size (and any new record) and asks whether to keep it in a bottle.
- **Casting:** casts go farther, and each strength upgrade adds 20% to the longest cast.
- **B button:** going through doors with the rod out no longer turns B into a Deku Stick when you have no sword.
- Not yet fixed: the save-load crash (being looked at next with your save).

## Performance test builds (earlier)

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
