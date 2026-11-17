# Recording a playthrough and playing it back

A playthrough of the real game, from power-on, recorded once and played back by any later build. It's the profilers'
path through the whole game (every system, cutscenes and all), and the input for a profile-guided build.

Code: `src/code/fishmod_debug_inputrec.inc.c`. PC side: `fishmod/tools/inputrec.py`. Report: `fishmod/tools/sysprof.py`.

## Recording (you play)

1. Build the recording ROM (the game as shipped, plus the recorder, the PC-sampling profiler and the frame log):

   ```bash
   fishmod/build.sh --script inputrec --hw --colpack --meshpack --gcc-all
   ```

2. On the PC, start the capture **before** switching the console on (the save memory is sent in the first second):

   ```bash
   mkdir rec1 && cd rec1 && sc64deployer debug
   ```

3. Upload the ROM and play. Any save works: the cart's save memory is recorded as the game found it, so a session can
   start from a new file or from one already hours in. Every half minute the terminal prints a line like
   `FISHMOD-REC 12:30, 41 game states, 310 packets, 0 words dropped`. If the dropped count is ever not 0, the PC wasn't
   keeping up and that recording has a hole: stop and start another.

4. Stop whenever: switch the console off, then stop `sc64deployer`. A recording cut off anywhere is good up to its
   last second. Sessions are separate recordings (one per power-on); play as many as you like, each from the save the
   last one left.

5. Join the packets into one file:

   ```bash
   fishmod/tools/inputrec.py join rec1 -o session1.finp
   fishmod/tools/inputrec.py info session1.finp        # length, scenes visited, scenes never visited
   fishmod/tools/inputrec.py info -v session1.finp     # every game state and scene, in order
   ```

The same folder also holds that session's console profile and frame log (`n64prof.py`, `framelog.py`, `sysprof.py`
read it as it is): the recording session is itself a real-hardware profile of the playthrough.

What to play, for coverage: `info` lists the scenes a recording never loaded. Across sessions, the aim is every scene,
both ages, day and night, each dungeon and boss, the minigames, the pause menu's pages, text and shops, and the
cutscenes.

Nothing is drawn over the picture while recording or replaying (debug text is off unless a script defines
`FISHMOD_SCREEN_TEXT`); what a run has to say comes over USB to the terminal.

## Playing it back

```bash
fishmod/build.sh --script inputplay --hw --colpack --meshpack --gcc-all
fishmod/tools/inputrec.py attach fishmod/out/fish-oot-debug-inputplay-....z64 session1.finp replay1.z64
```

`replay1.z64` (64 MB: the recording sits at 0x03000000) plays the session by itself, in ares or on the console, with
the profiler and frame log running. Over USB it prints `FISHMOD-REPLAY` lines and, at the end, a last one and
`FISHMOD-DONE`.

### How it stays in step

A different build doesn't run the same frames: an object's file arrives a frame later, a sound that a text box waits
for ends a frame sooner. Pure input replay goes wrong at the first such frame and never recovers. So the replay
repairs itself:

- Every game state (the logo, the file select, every scene loaded) starts from the recording's save context, whatever
  happened before it. A scene always starts as it did when recorded.
- Once a second the random numbers are set to the recording's, and Link is put back where the recording had him if
  he's more than 40 units away.
- A scene the replay leaves before the recording did: the rest of its inputs are skipped.
- A scene the replay is still in when the recording left: 5 seconds' grace, then it's left by force for the next one.

The report counts each of those (`gFishmodReplay`, and the `FISHMOD-REPLAY` lines):

| count | meaning |
|---|---|
| `early` (and frames skipped) | scenes left before the recording did |
| `late` / `forced` | scenes still going when the recording left / of those, ended by force |
| `snaps` | times Link was moved back to the recorded place |
| `rng` | times the random numbers differed at the once-a-second check |

All zeros means the replay ran exactly as recorded. Small counts mean it wandered and was put back: still the same
path through the game for a profile, but the frames around a repair aren't what was played.

Variants for testing: `inputplay_loose` (counts drift, repairs nothing: what a build drifts by on its own),
`inputplay_perturb` (knocks the random numbers out of step every 7 seconds: tests the repairs).

## Profiling a replay

Exactly, in ares (the build from `fishmod/tools/build_ares.sh`, with `ares-prof.patch`):

```bash
OVL=$(mips-linux-gnu-nm build/ntsc-1.2/oot-ntsc-1.2.elf | awk '/ gFishmodOvlMap$/ {print $1}')
ARES_CPUPROF=cpu.bin ARES_CPUPROF_OVL=$OVL ARES_FAST=1 fishmod/tools/ares_run.sh 0 replay1.z64 SECONDS usb1
fishmod/tools/sysprof.py report cpu.bin --elf build/ntsc-1.2/oot-ntsc-1.2.elf -o report.md --json report.json
```

`ARES_CPUPROF_OVL` is new: the game keeps a map of where each overlay (actors, Link, the pause menu, the file select)
is loaded, and ares counts their instructions at the addresses the ELF has for them. Without it a whole-game profile
can't tell one actor's code from another's that was later loaded at the same place.

On the console: run `replay1.z64` under `sc64deployer debug`; the folder of packets is the profile
(`sysprof.py report FOLDER --elf ...`).

`sysprof.py report` gives every system's share of the CPU's working time, then for each system its files, its
costliest functions (calls, cycles and instructions per call, cache fills) and their costliest lines.
`sysprof.py compare before.json after.json` puts two runs side by side by system and by function: the check after an
optimization.

For a profile-guided build: `n64prof.py llvm` turns a console profile into the sample profile `fishmod/build.sh`'s
clang option takes (`PGO_PROFILE=...`).

## Testing (no controller needed)

`--script inputrec_test` is a recording made by a script instead of a player: a new file at the file select, the
opening cutscene and its text, then out of Link's house. Run it in ares with `ARES_SC64_DIR=folder`, join, attach to
an `inputplay` build, run that, and read the counts.

## The recording's format

`'FINP'`, version 1, the save memory's size, the stream's length in words, the save context's size in words, three
spare words; the save memory; the stream. The stream's words are described at the top of the `.inc.c`. A recording only
fits builds whose save context is the same size (the replay says so and plays nothing otherwise).
