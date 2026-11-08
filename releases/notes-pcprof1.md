# pcprof1: the PC-sampling profile build (console)

For the compiler profile (n64prof, `n64prof/README.md`). Built from commit `cf2f8a5` with
`fishmod/build.sh --script pcprof --hw`; the same commit rebuilds the matching ELF for reading the save.

## The patch
- **`fish-oot-pcprof1-hw.bps`**: console (F3DEX3). **Blank save**: it writes over the save files.

## Running it
It plays the Lake Hylia bench route by itself (walking, running, swimming, diving and fishing, every time of day and
weather) with the game as shipped, and samples where the CPU is about 4000 times a second. The profile is saved every
30 seconds, so stop it whenever you like (an hour or more gives a fuller profile) and send the `.sav`.

The debug text at the bottom shows the profile's progress: `pcprof dump N: samples, buckets`.

## Reading it
```
n64prof/tools/n64prof.py report SAVE --elf ELF          # where the time goes
n64prof/tools/n64prof.py llvm SAVE --elf ELF -o oot.prof
n64prof/tools/n64prof.py compare CONSOLE_SAVE ARES_SAVE --elf ELF
```
A 10-minute run in ares: 0.97 M samples, 98% kept; llvm-profdata reads the profile (698 functions). Audio,
libultra and boot are built with IDO and have no line tables: they count by function only.
