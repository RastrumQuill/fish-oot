# frames1: the game as shipped against vanilla, frame by frame, on the same route

Two ROMs that play the play bench's route at Lake Hylia by themselves (the tour's stops, eight laps: every time of day
and weather), log every frame and sample the CPU, over the SummerCart64's USB:

| patch | what |
|---|---|
| **`fish-oot-frames1-shipped.bps`** | the mod as shipped (GCC; IDO for audio and libultra): `fishmod/build.sh --script pcprof_frames --hw` |
| **`fish-oot-frames1-vanilla.bps`** | the unmodified game with only the route player, the timing and the USB: `vanilla/build.sh` |

Both from the commit that adds this file. The same in each: what's timed and how, the route, the blocks.

## Running each
1. In an **empty folder** (one per ROM), run `sc64deployer debug` and leave it running.
2. Start the ROM (blank save for the mod's; the vanilla one keeps no save). It plays by itself.
3. On screen: **`pcprof usb: N packets`** (the mod's) or **`vbench usb: N packets`** (vanilla's). At the end it says
   **DONE** and stops sending: then stop `sc64deployer`.
4. Zip each folder and send them. The mod's run takes about 8 hours (the reeling in after fishing legs adds a little);
   vanilla's a little less.

## What it gives
Per frame: its length, the CPU's update (by section: collision, actors, camera, environment...) and draw, the RSP, the
RDP and its counters, the costliest actor kinds; and the CPU's profile split by frames over 52 ms and the rest. So for
each ROM: how often it misses 20 fps on the path, where those frames' time goes, and for the mod, what it adds over
vanilla, section by section.

Read with `fishmod/tools/framelog.py FOLDER --by-leg` and `n64prof/tools/n64prof.py report FOLDER --elf ELF --frames slow`
(the ELFs: the same commit and commands rebuild them).

## Checked in ares
Both boot, play the route and (without a SummerCart64) say so on screen; the readers were tested on made-up packets.
The USB path itself runs only on the console.
