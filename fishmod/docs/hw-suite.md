# The console test suite

A queue for the morning runner (`fishmod/pc/morning.bat`, `fishmod/pc/MORNING.md`) that answers two questions on
the real console: does each build of today's code run right, and which is fastest. The emulator's numbers on the
second question disagree with each other (below), so the console decides.

## Running it

It is the morning queue as committed (`fishmod/pc/morning_queue.txt`): switch the console on, run `morning.bat`,
press RESET if it asks. Nine runs, about an hour, nobody needed after the first minute. No Expansion Pak. The last
run (the replay) writes the recording's empty save over the cart's save memory: back it up first if it matters.

The report is `hwsuite.txt` in the morning's folder (pushed to `morning-results` with the rest), also printed at
the end. To make it again from any morning's folder: `python fishmod/pc/hwsuite.py <folder>`.

## What runs

| label | compiler | level | on GCC |
|---|---|---|---|
| `hw-base` | GCC 10 (Ubuntu's, for Linux) | `-O2` | the game, audio, libultra (`--gcc-all`): today's build |
| `hw-g10all` | GCC 10 | `-O2` | every C file (`--gcc-everything`) |
| `hw-g14o2` | GCC 14.2 for bare metal (`fishmod/tools/build_n64_gcc.sh`) | `-O2` | every C file |
| `hw-g14os` | the same | `-Os` | every C file |
| `hw-replay-g14o2` | the same as `hw-g14o2` | | a recorded opening replayed from power-on |

The four bench builds are `--script hwtest_frames --hw --texpack --colpack --meshpack`: rc2's frame-log bench (the
short lap at Lake Hylia) with **the sampler off**. With it on, its 4,000 interrupts a second slow the game by a
quarter and empty its caches (fishmod/docs/input-replay.md), which would bury the differences this suite is after.
Each frame's record still goes over USB. Each build runs twice, A B C D then A B C D, so the report can show the
spread between a build's own runs next to the difference between builds.

The replay is `--script hwtest_replay` with `releases/hwsuite/opening-5k.finp` attached: power-on, the file select,
a new save, the opening cutscene, 5,000 frames, recorded in the emulator on an older `-O2` build.

## What the report says

1. **Did each run work**: it started, finished, and its `FISHMOD-CHECK` line (frames played, how many sets of
   props were made and a running hash of all of them; for the replay, its counts) against what the same ROM gave in
   ares (`releases/hwsuite/expected.json`). The same values mean the build does on the console exactly what it does
   in the emulator. Different values aren't a failure by themselves (the console's timing isn't the emulator's, and
   a set is made when the camera has moved enough, which can fall a frame apart), but a run that doesn't finish, or
   a replay that had to force scenes, is.
2. **Do a build's own runs agree**: the same ROM twice must give the same check values. If not, the route isn't
   repeatable on the console and the timing comparison is looser than it looks.
3. **How fast**: frames a second, the frame's period, the CPU's update and draw, the RSP, the RDP, slow frames.
4. **Each build against `hw-base`**, with the spread between each build's two runs: a difference smaller than
   the spreads is noise.

The set hash is only comparable between runs of the same ROM: different compilers round floats differently, the
camera ends a hair apart, and other sets get made (in ares: 798, 763, 745 and 798 sets over the same 3,765 frames).

## The builds stop playing the same game at frame 875

Laid side by side (`fishmod/tools/settrace.py`, from `gFishmodSetTrace` and `gFishmodLinkTrace`), `hw-base` and the
bare-metal `-O2` build make the same sets of props from the same camera, to the unit, for the bench's first 871
frames (237 sets). On frame 875 the camera is somewhere else in one of them (a decision of the game's camera that
went the other way: which comparison, not found yet), Link is steered by the camera, and by frame 1056 he's in
another place too. The route puts him back at each leg, so they meet again and part again. The same ROM run twice
is identical throughout.

So the prop-set hash can only be compared between runs of one ROM, and frame times over the whole run compare
builds doing different work. The report's last table uses frames 60 to 860 only, where the work is the same (in
ares all four builds draw 164.6 props a frame there):

| build | CPU update + draw, frames 60-859, in ares | against `hw-base` |
|---|---:|---:|
| `hw-base` | 28.04 ms | |
| `hw-g10all` | 27.91 ms | -0.5% |
| `hw-g14o2` | 28.91 ms | +3.1% |
| `hw-g14os` | 29.00 ms | +3.4% |

(The game never sets its frame counter: in the all-GCC builds it started from 1,701,999,717, in the others from 0.
Scripted builds now start it at 0. It wasn't the cause of the parting: with it fixed the builds still part on
frame 875. The suite's committed patches are from before that change.)

## What the emulator said, and why the console is needed

| build | ares's exact profile, working cycles against the old `-O2` build | this suite's dry run in ares, CPU update + draw against `hw-base` |
|---|---:|---:|
| `hw-g10all` | | +0.4% |
| `hw-g14o2` | -1.6% | +4.0% |
| `hw-g14os` | +4.1% | +3.4% |

The two columns aren't the same measurement (the first is every thread's cycles, against a base built before the
copy and divide-check options; the second is the game thread's update and draw, against a base that has them), but
they point opposite ways for the bare-metal `-O2` build. Neither is the console.

## Making the patches again

```bash
F="--hw --texpack --colpack --meshpack"
fishmod/build.sh --script hwtest_frames $F --gcc-all                      # hw-base
fishmod/build.sh --script hwtest_frames $F --gcc-everything               # hw-g10all
export FISHMOD_GCC=/opt/n64gcc/bin/mips64-elf-gcc FISHMOD_GCC_AS_DIR=/opt/n64gcc/linuxas
fishmod/build.sh --script hwtest_frames $F --gcc O2 --gcc-everything      # hw-g14o2
fishmod/build.sh --script hwtest_frames $F --gcc Os --gcc-everything      # hw-g14os
fishmod/build.sh --script hwtest_replay $F --gcc O2 --gcc-everything
fishmod/tools/inputrec.py attach <that ROM> releases/hwsuite/opening-5k.finp replay.z64
fishmod/tools/bps.py --create --bps baseroms/ntsc-1.2/baserom.z64 <ROM> releases/hwsuite/<label>.bps
```

Then run each in ares with `ARES_SC64_DIR=<folder>` and put its `FISHMOD-CHECK` values in `expected.json`.
