# F3DEX3 microcode

[F3DEX3](https://github.com/HackerN64/F3DEX3) by Sauraen is a faster replacement for the game's F3DZEX2 graphics
microcode. Build with `F3DEX3=1` (`fishmod/build.sh --f3dex3`) to use it.

The microcode isn't stored in the repository directly: the BPS patches in `fishmod/f3dex3/` (from [HackerOoT](https://github.com/HackerN64/HackerOoT))
turn the F3DZEX2 microcode extracted from your own ROM into F3DEX3 at build time.

| Patch | Variant |
|---|---|
| `F3DEX3_BrW_NOC.*.bps` | `BrW`: the Z-buffer and tri setup used by OoT. `NOC`: without occlusion planes (unused here) |

F3DEX3 runs on real hardware and on LLE emulators (ares, parallel-n64 / ParaLLEl). HLE emulators (Project64,
mupen64plus with rsp-hle) don't know it and will show a black screen.

## Keeping it up to date

The patches give the latest F3DEX3 (version B, `F3DEX3_BrW_NOC_B`, upstream as of October 2025): checked by building
it from source and comparing byte for byte. `include/ultra64/gbi.f3dex3.h` is upstream's `gbi.h` (only the include
guard renamed). To check or update after an upstream change:

    git clone https://github.com/HackerN64/F3DEX3 && git clone --recursive https://github.com/Kingcom/armips
    (cd armips && mkdir build && cd build && cmake .. && make armips)
    (cd F3DEX3 && make ARMIPS=../armips/build/armips PARENT_OUTPUT_DIR=../f3dex3build F3DEX3_BrW_NOC)
    # compare f3dex3build/F3DEX3_BrW_NOC/F3DEX3_BrW_NOC.code/.data with extracted/<version>/incbin/F3DEX3_BrW_NOC_*;
    # if they differ, make new patches: flips --create --bps <F3DZEX2 text/data> <new code/data> fishmod/f3dex3/*.bps
(The final `.o` step of upstream's Makefile may fail; only the `.code` and `.data` files are used.)

## Testing in ares

ares runs F3DEX3, but (as of October 2026, JIT or interpreter) it computes a different boot checksum (the
cartridge's CIC-6105 boot code) for our release ROMs than the console does, so they never start there: the boot code
loops at `0x80000248` and ares logs a "boot timeout". Debug builds start. To check what release builds go through
(the logo, title and file select screens), use a debug build that boots normally: `--script newfile`
(`FISHMOD_SCRIPT_NORMAL_BOOT`).
