# pcprof2: the PC-sampling profile build, second run (console)

For the compiler profile (n64prof, `n64prof/README.md`): the same run as pcprof1 (the Lake Hylia bench route by itself:
walking, running, swimming, diving and fishing, every time of day and weather), on today's code. Built from the commit
that adds this file with `fishmod/build.sh --script pcprof --hw`; the same commit rebuilds the matching ELF.

## The patch
- **`fish-oot-pcprof2-hw.bps`**: console (F3DEX3). **Blank save**: it writes over the save files. It saves the profile
  every 30 seconds, so any length of run is readable; a few hours is plenty. Send the `.sav`.

## What changed since pcprof1
- **Fewer samples lost:** 32-byte buckets (half as many to keep). pcprof1 lost 12.8% of its samples for want of room in
  the save; in ares this one loses about 1%.
- **Less of the profiler in its own profile:** the overlay table is read every 8 frames (it was every frame: 0.8%), and
  the debug text isn't formatted every frame (about 2%).
- **The game's own fixes from pcprof1's findings**, to see what they bought:
  - `memset` a word at a time (it wrote a byte at a time: 2% of the CPU);
  - `Matrix_MtxFToMtx` (1.6% + 1.1% for the skinned copy, which now shares it): whole-word stores, and the
    destination's cache lines claimed without reading them from RDRAM first.
- **A correction:** pcprof1's report named `fmodf` at 1.5%. That was read with an ELF 0x20 bytes off the build that
  ran; with the right one it is `memset` (nothing calls `fmodf`).

Read with:
```
n64prof/tools/n64prof.py report SAVE --elf ELF
n64prof/tools/n64prof.py llvm SAVE --elf ELF -o oot.prof
```
