# pcprof3: the whole game on clang, profiled per instruction over a SummerCart64's USB

The profile to refine LLVM's optimisation of the whole game, not just the mod: the Lake Hylia bench route (walking,
running, swimming, diving and fishing, every time of day and weather), on a build where clang (LLVM) compiles the
game's C code (`src/code`, all 470 overlays), the audio driver and libultra's C (`os`, `io`, `gu`), every file with
line tables. Built from the commit that adds this file with
`fishmod/build.sh --script pcprof --hw --clang-game --clang-opt Os` (ROM MD5 `a7b66762ad323d5ce4d5112e8936bdbd`); the
same commit and command rebuild the matching ELF (the build is deterministic: check the MD5).

## The patch
- **`fish-oot-pcprof3-hw.bps`**: console (F3DEX3), **blank save**.

## Running it
1. Connect the SummerCart64's USB to the PC. In an **empty folder**, run `sc64deployer debug` (from the SummerCart64
   release's deployer) and leave it running: it saves every packet the game sends as a `binaryout-*.bin` file there.
2. Start the ROM as usual (SD card menu). The route plays by itself.
3. On screen, **`pcprof usb: N packets`** should appear and grow about every 10 seconds. If **`pcprof dump`** shows
   instead, the SummerCart64 wasn't found and it's saving to the save memory as before (send the `.sav` then).
4. Any length is usable (every packet stands alone; a lap is about an hour, and the eight laps cover every time and
   weather). Stop whenever, then zip the folder and send it. (Roughly 30 MB an hour; it compresses well.)

If the PC stops listening, the game keeps going and the samples in between are lost (the reader reports missing
packets); nothing is written to the save in USB mode.

## What's new
- **The whole game on clang** (`--clang-game`): the state fingerprints of the three regression scripts match GCC's
  exactly in ares (game code, then audio, then libultra). Boot, libc's 64-bit helpers and the assembly stay as they were.
- **Every instruction counted, nothing dropped:** over USB there's no save-size limit, so the profile has one bucket per
  instruction (pcprof2: 32 bytes, the hottest ~3,500 kept, 9% of samples lost).
- **Line tables everywhere clang builds:** pcprof2 had none for audio and libultra (44% of its samples); here only the
  assembly and the instructions clang gives no line remain (about 24% of samples in ares; 6 points of that are the
  interrupts' disable/restore, where samples taken with interrupts off land).
- **A profile from the compiler it's for:** where clang inlined and how it laid the code out are in the profile, which a
  GCC build's can't give. `-fdebug-info-for-profiling` gives each block its own discriminator (the code is unchanged).

## Open
- libultra built `-Os` hangs at boot (the VI manager is never started); `-O2` runs, so libultra stays at `-O2` whatever
  `--clang-opt` says (`tools/fishmod_clang_cc.sh`).
- The route is Lake Hylia's; a general N64 profile wants other scenes and other games too.

Read with (the folder as is):
```
n64prof/tools/n64prof.py report FOLDER --elf ELF
n64prof/tools/n64prof.py llvm FOLDER --elf ELF -o oot-console3.prof
```
