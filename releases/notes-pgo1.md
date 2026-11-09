# pgo1: GCC, clang and clang with the console's profile, on the fish stress bench

The first test of LLVM profile-guided optimisation on the N64. Three builds of the fish stress bench (stress1's lab
tank: Lake Hylia's fish let in 6 more a step until 20 fps can't hold, three passes), the same game except for the
compiler of the mod's own code (the fish actor, src/code/fishmod*.c; the rest of the game is on GCC in all three):

- **`fish-oot-stress-fish-pgo1-gcc-hw.bps`**: GCC, as every release so far (`fishmod/build.sh --script stress_fish --hw`).
- **`fish-oot-stress-fish-pgo1-clang-hw.bps`**: clang (LLVM 18) without a profile (`--clang`).
- **`fish-oot-stress-fish-pgo1-pgo-hw.bps`**: clang with `n64prof/profiles/oot-console2.prof`, the pcprof2 run's
  samples (`--pgo`).

Console, **blank save** (each writes over the save files). About 10 minutes each, **STRESS DONE** at the end. Send
the three `.sav` files; `fishmod/tools/stress_report.py SAVE` reads them. The columns to compare are **fish update**
and **CPU update** (the mod's code runs there); RSP and RDP should come out the same in all three.

## In ares (its timings aren't the console's)

All three play the same: the regression scripts' state fingerprints match GCC's exactly. Fish update in ms per frame
(ares's emulated clock):

| fish | GCC | clang | clang + profile |
|---|---|---|---|
| 12 | 3.8 | 4.6 | 4.2 |
| 36 | 10.2 | 11.9 | 12.7 |
| 60 | 16.6 | 17.8 | 19.5 |
| 84 | 22.6 | 24.7 | 25.3 |

So far GCC is ahead in ares. The console decides: ares's CPU timing is an approximation, and inlining and code
layout, what the profile changes most, are the kind of difference it may not time like the console.

## What it took
- The overlay loader pairs relocations the linker's way for clang's overlay (`RELOC_ELF_PAIRING`).
- `-mno-odd-spreg`: single-precision arithmetic in odd FPU registers isn't allowed in the game's FPU mode (it crashed).
- The profile's head samples: with none, LLVM took every profiled function as never called and stopped inlining in
  them (that build was slower than plain clang). They're now estimated from each function's first line.
- The profile is from a GCC build (pcprof2, commit `7d0542b`), so where GCC inlined differently or the code has
  changed since, it fits less well. A profile taken on a clang build is the next step.
