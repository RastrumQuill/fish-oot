# Test 5c: the game's code built with GCC

The game's own code (`src/code`, every actor, and the mod) is now compiled with GCC 12 at `-O2`, instead of the 1998
IDO compiler. The boot code, libultra, libc and audio stay on IDO: an all-GCC build hangs before the game starts.
Build with `fishmod/build.sh --gcc O2`.

In ares, at two tour stops and all nine bench modes, the CPU time per frame went down:

| | IDO | GCC | Change |
|---|---|---|---|
| Update (game logic) | 20.4 ms | 17.8 ms | −13% |
| Draw (building the display lists) | 15.9 ms | 13.1 ms | −17% |
| Fish update | 7.5 ms | 7.5 ms | 0% |
| Fish draw | 6.1 ms | 5.5 ms | −10% |

The gain is in the vanilla engine. The mod's own fish code is no faster yet. These are ares's timings, which are only
roughly the console's; the console bench decides.

These builds also have the props' new edge mode only within 500 units, as in the exact A/B test.

## 1. `fish-oot-hwbench2-gcc.bps`
The same overnight bench as `hwbench2`, built with GCC. Run it for a night after `hwbench2`, and send both `.sav`
files. Comparing their CPU columns gives the console's own answer.

## 2. `fish-oot-kokiri-test5c-gcc.bps`
The Kokiri / Lake Hylia test built with GCC, for play testing. Anything that behaves differently from test 5b is
worth noting: the compiler change touches every actor.
