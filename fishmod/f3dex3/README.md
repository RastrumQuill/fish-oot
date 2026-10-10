# F3DEX3 microcode

[F3DEX3](https://github.com/HackerN64/F3DEX3) by Sauraen is a faster replacement for the game's F3DZEX2 graphics
microcode. Build with `F3DEX3=1` (`fishmod/build.sh --f3dex3`) to use it.

The microcode isn't stored here directly: these BPS patches (from [HackerOoT](https://github.com/HackerN64/HackerOoT))
turn the F3DZEX2 microcode extracted from your own ROM into F3DEX3 at build time.

| Patch | Variant |
|---|---|
| `F3DEX3_BrW_NOC.*.bps` | `BrW`: the Z-buffer and tri setup used by OoT. `NOC`: without occlusion planes (unused here) |

F3DEX3 runs on real hardware and on LLE emulators (ares, parallel-n64 / ParaLLEl). HLE emulators (Project64,
mupen64plus with rsp-hle) don't know it and will show a black screen.
