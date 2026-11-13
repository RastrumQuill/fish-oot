# frames11b: which of frames11's changes made the CPU slower

frames11 (see [its notes](notes-frames11.md)) took 2.4 ms off the RDP but made the CPU 4-5 ms slower, spread over
nearly every function: not where the changes are. Six builds of the same code (all with run-ahead, as frames10), each
leaving one change out (`fishmod/build.sh --script pcprof_frames --hw --colpack --gcc-all "GCC_EXTRA_OPT=..."`,
touching the changed files first so make recompiles them):

| patch | left out | switch |
|---|---|---|
| `fish-oot-frames11b.bps` | nothing (frames11 as it was) | |
| `fish-oot-frames11b-oldsky.bps` | the sky and clears (fill kept, Z by the RDP, sky 2-cycle) | `-DFISHMOD_OLD_SKY` |
| `fish-oot-frames11b-noclaim.bps` | the display lists' cache-line claims | `-DFISHMOD_NO_DLCLAIM` |
| `fish-oot-frames11b-nowall.bps` | the wall tests' clearance bound | `-DFISHMOD_NO_WALLCLEAR` |
| `fish-oot-frames11b-noaffine.bps` | the limb matrices' bottom-row skip | `-DFISHMOD_NO_AFFINE` |
| `fish-oot-frames11b-oldfish.bps` | the fish's grouped matrix loads and edge mode | `-DFISHMOD_OLD_RIGLOADS -DFISHMOD_NO_FISH_RA` |

All of them also **halt a second after DONE** (`FISHMOD-HALTED`): the picture stops there on purpose, so the next
upload can't be read by a game still running.

What to compare: the CPU's update and draw, and the RDP, of each against frames11b (and frames10). A switch whose build
gets the CPU back is the culprit; if none does, it's the code's layout, not any one change.

## Results (12 November, the console)

| | visible hitches | CPU update (full-speed frames) | CPU draw | RDP |
|---|---|---|---|---|
| frames11 (4 runs) | 28-31% | 23.2-23.5 ms | 22.2-22.3 ms | 39.3-39.7 ms |
| noclaim | 28% | 23.1 | 22.0 | 39.7 |
| **nowall** | **19%** | **22.0** | **21.7** | 39.6 |
| oldsky | 32% | 23.2 | 22.0 | 40.9 |

The wall clearance bound was the slowdown: off from frames13.
