# frames20-colbase and frames20-colnew: the collision work on the console

Two builds of the same tree with the same toolchain (`fishmod/build.sh --script pcprof_frames --hw --colpack
--meshpack --gcc-all`, GCC 10.5 here), to compare on the console **with the Expansion Pak in**:

- **`fish-oot-frames20-colbase.bps`**: frames20's collision (`src/code/z_bgcheck.c` as at a8db7be). Not
  `fish-oot-frames20.bps` itself, so that the compiler isn't a difference between the two.
- **`fish-oot-frames20-colnew.bps`**: the collision of this branch (`claude/collision-opt`): the wall, ceiling, line
  and sphere searches from the polys' records, the line test's reach test, the platforms' reach, and, with the
  Expansion Pak, the floor search's index and short way (tools/bgcheck_check/README.md).

**The queue** (`fishmod/pc/morning_queue.txt`): colbase, colnew, colbase, colnew, 12 minutes each, so each build is
measured twice and the run-to-run spread shows. `fishmod\pc\morning.bat`, as for any morning (MORNING.md).

**What to compare:** the CPU's update time and the hitches (the frame log), and the PC samples in `z_bgcheck.c`'s
functions (n64prof). A profile is read with the ELF of the build it was taken from, and the ELFs aren't in the
repository (190 MB each): rebuild with the command above (colnew: this commit; colbase: the same with
`git show a8db7be:src/code/z_bgcheck.c > src/code/z_bgcheck.c`), or compare by the frame log alone.

**In ares** (the collog route, 8,050 frames, collision's own time counted in the game by the same code in both
builds): 3.53 -> 2.81 ms a frame (-21%), every logged frame's questions and answers the same (2,000 frames); the
scene's load no longer. The same answers: tools/bgcheck_check against a8db7be (5.5
million questions, with and without an Expansion Pak), and in the ROM the short way checked against the search as
it was on the side (67,153 answers, 97,693 platform skips, none different).

Without an Expansion Pak, colnew runs without the index and the short way (their memory comes from the 2 MB the mod
gives Play with a Pak): the other changes still apply.

## On the console (run 20261115-1503, Expansion Pak in; colbase, colnew, colbase, colnew)

| | colbase-1 | colbase-2 | colnew-1 | colnew-2 |
|---|---|---|---|---|
| visible hitches (pictures up 4 refreshes or more) | 28% | 29% | 25% | 26% |
| frames over 52 ms | 43% | 44% | 39% | 40% |
| fps | 18.4 | 18.3 | 18.5 | 18.5 |
| CPU update, slow / full-speed frames (ms) | 21.62 / 21.26 | 21.59 / 21.07 | 19.90 / 19.68 | 20.23 / 19.89 |
| update: actors (ms) | 14.34 / 14.38 | 14.31 / 14.35 | 12.95 / 13.33 | 13.25 / 13.62 |
| update: camera (ms) | 3.00 / 3.25 | 3.09 / 3.19 | 2.86 / 2.76 | 2.94 / 2.77 |
| the actors' collision checks (FISHMOD-BGCHECK, ms a frame) | 2.246 | 2.243 | 1.892 | 1.889 |
| ... a call: Link / Tektite / Guay (us) | 747 / 264 / 618 | 723 / 269 / 617 | 571 / 234 / 536 | 572 / 234 / 530 |

- **CPU update: about 1.4 ms less a frame** (21.2-21.6 -> 19.8-20.1 ms, -7%), the two runs of each build within 0.3 ms of
  each other. The actors' own collision checks account for 0.35 ms of it (-16%: Link's -22%, the Tektites' -12%, the
  Guays' -14%); the rest is in the actors' updates and the camera outside those checks (their other collision
  questions, and less of their code and data pushed out of the caches).
- **Hitches 28-29% -> 25-26%** of pictures, frames over 52 ms 43-44% -> 39-40%. The RDP, RSP and CPU draw are as they
  were (the RDP still decides most slow frames).
- Two runs a build: enough to see the difference is well past the spread between runs, not to put a third digit on it.
