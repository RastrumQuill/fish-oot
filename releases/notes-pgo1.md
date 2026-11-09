# pgo1: GCC against clang, by speed or by size, with and without the console's profile

The first test of LLVM profile-guided optimisation on the N64. Four builds of the fish stress bench (stress1's lab
tank: Lake Hylia's fish let in 6 more a step until 20 fps can't hold, three passes), the same game except for the
compiler of the mod's own code (the fish actor, src/code/fishmod*.c; the rest of the game is on GCC in all four):

| patch | the mod's code | build |
|---|---|---|
| **`fish-oot-stress-fish-pgo1-gcc-hw.bps`** | GCC -O2, as every release so far | `--script stress_fish --hw` |
| **`fish-oot-stress-fish-pgo1-clang-o2-pgo-hw.bps`** | clang -O2 with the profile | `... --pgo` |
| **`fish-oot-stress-fish-pgo1-clang-os-hw.bps`** | clang -Os | `... --clang-opt Os` |
| **`fish-oot-stress-fish-pgo1-clang-os-pgo-hw.bps`** | clang -Os with the profile | `... --clang-opt Os --pgo` |

The profile is `n64prof/profiles/oot-console2.prof` (the pcprof2 run's samples). The pairs answer: speed or size for
clang (o2-pgo against os-pgo), whether the profile helps a size build (os against os-pgo), and each against GCC.

Console, **blank save** (each writes over the save files). About 10 minutes each, **STRESS DONE** at the end. Send
the four `.sav` files; `fishmod/tools/stress_report.py SAVE` reads them. The columns to compare are **fish update**
and **CPU update** (the mod's code runs there); RSP and RDP should come out the same in all four.

## Size: the hot code against the 16 KB instruction cache

The 14 functions holding 90% of the profile's samples in the mod's code (plant drawing, fish update and AI, the
water floor, particles...), their size in each build (a function inlined away counts in its caller):

| build | the mod's code | the hot 90% |
|---|---|---|
| GCC -O2 | 260 KB | 43 KB |
| clang -O2 | 287 KB | 51 KB |
| clang -O2 + profile | 284 KB | 41 KB |
| clang -Os | 264 KB | 44 KB |
| clang -Os + profile | 262 KB | 34 KB |

Not all of it runs in one frame, so it's a measure of pressure on the cache, not a count of misses.

## In ares (its timings aren't the console's)

They play the same as GCC (the regression scripts' state fingerprints match for clang -O2). Fish update in ms per frame
(ares's emulated clock):

| fish | GCC -O2 | clang -O2 | clang -O2 + profile | clang -Os | clang -Os + profile |
|---|---|---|---|---|---|
| 12 | 3.8 | 4.6 | 4.2 | 3.4 | 4.2 |
| 36 | 10.2 | 11.9 | 12.7 | | |
| 60 | 16.6 | 17.8 | 19.5 | | |
| 84 | 22.6 | 24.7 | 25.3 | | |

(clang -Os was checked for its first steps only.) ares's CPU timing is an approximation; the console decides. For GCC
the console chose -O2 over -Os (hwbench-4: CPU time 14-16% lower), so size isn't assumed to win here either.

## What it took
- The overlay loader pairs relocations the linker's way for clang's overlay (`RELOC_ELF_PAIRING`).
- `-mno-odd-spreg`: single-precision arithmetic in odd FPU registers isn't allowed in the game's FPU mode (it crashed).
- The profile's head samples: with none, LLVM took every profiled function as never called and stopped inlining in
  them (that build was slower than plain clang). They're now estimated from each function's first line.
- The profile is from a GCC build (pcprof2, commit `7d0542b`), so where GCC inlined differently or the code has
  changed since, it fits less well. A profile taken on a clang build is the next step.
