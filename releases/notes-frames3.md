# frames3: the mod with the game-wide optimizations, on frames2's route

Two ROMs, each the frames2 mod run (the play bench's route at Lake Hylia, each leg once, eight laps, every frame
logged over the SummerCart64's USB) with everything since frames2. The second also has the texture packing, so the
textures are measured on their own (frames3-texpack against frames3), and the rest against frames2's mod run.

| patch | what |
|---|---|
| **`fish-oot-frames3.bps`** | frames2's mod plus: polynomial trig everywhere (sin, cos, atan, atan2); distant idle Tektites at a quarter rate; the collision work (line tests without clearing their table, vertices converted once, each poly's box checked first in floor, wall and ceiling searches, the scene's floor answers cached); the collision merge (`--colpack`). `fishmod/build.sh --script pcprof_frames --hw --colpack` |
| **`fish-oot-frames3-texpack.bps`** | the same plus the texture packing: 1,438 textures to 8-bit palettes where it's smaller, 3,413 empty pipe syncs gone (`--texpack`) |

## Checked first
- **Collision:** every exact change gave the same collision answers as the build before it on every frame compared in
  ares (the collision log, `fishmod/tools/collog.sh`): 3,200-3,400 frames each. The merge: the same answers (by
  surface and plane) on 2,219 frames.
- **Trig:** against double precision on the PC (`fishmod/tools/trig_check.c`): sin and cos 1e-7 (the table: 1.6e-3),
  atan 1.2e-7 (the continued fraction it replaces: 1.8e-7, with 24 divisions a call), atan2 within half a binang
  (the table: 5.6).
- **ares, frames2's mod against frames3 (CPU only; ares doesn't time the RDP):** 12.5 -> 14.7 fps, frames over 52 ms
  99% -> 90%, CPU update 45.0 -> 34.2 ms (enemies -5.9, camera -1.8, background actors -1.0, Link -0.9).
- Both boot and play the route in ares; the packets read.

## Running each
As frames2: an empty folder per ROM, `sc64deployer debug` running in it, start the ROM on a blank save. It shows
**`test: frame N, M packets`** at the bottom, and DONE at the end (about an hour). Zip each folder and send it.
