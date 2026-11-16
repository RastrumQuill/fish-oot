# audio-sched-ab

**`fish-oot-audio-sched-ab.bps`** (`fishmod/build.sh --script audio_sched_ab --hw --texpack --colpack --meshpack
--gcc-all --expak`): when the audio's updates are built and when its RSP tasks run, four ways, lap by lap on one
power-on. **Needs the Expansion Pak** (the first build with `--expak`: without the pak it says so and stops).

Queue line (about 28 minutes on the console; 34 allowed):

    releases/fish-oot-audio-sched-ab.bps 34 audio-sched-ab

## What it tests

audio-laps-every2 (releases/notes-audio-laps.md) showed that waking the driver half as often saves its time (14%) and
loses on screen (57% of frames over 52 ms against 42%): the updates and their RSP tasks came twice the size, one or
two a frame. Two changes that keep the game's own updates and tasks, the same size and the same number, and move
only when they're made and run:

- **Built ahead** (`gFishmodAudioAhead`, audio/internal/thread.c): the update that takes the game's commands (one a
  game frame) also builds the next two refreshes' command lists and tasks, in the same wake, while the driver's code
  and data are in the caches. A sound the game starts is heard as soon as it ever was.
- **Sent early** (`gFishmodAudioLazy`, thread.c and code/sched.c): each task is handed to the scheduler as soon as its
  update is built instead of at the next refresh, to run when the RSP is next free; only if it still hasn't run by
  the refresh the game would have sent it on does it take the RSP from the graphics task, as it always did.

Eight laps of the short bench, every lap clear dawn:

| laps | built ahead | sent early |
|---|---|---|
| 1, 5 | no | no (as always) |
| 2, 6 | yes | no |
| 3, 7 | no | yes |
| 4, 8 | yes | yes |

## The same sound

The audio check, the build with `--expak` (built ahead) against the build without: **the lake's route, 2,682 buffers,
and the tour of the game (audlog_world: 17 scenes, 18 scene loads), 12,516 buffers, every sample alike.** The tour
found three things the lake's route hadn't, each fixed: an update built ahead drew the sequences' random numbers with
another update's count (Hyrule Field's music); while the audio heap resets the game replays an old output buffer,
and six buffers held another than three; and the RSP reads a note's filter coefficients when its task runs, by when
later updates had rewritten them (the Shadow Temple): a task built ahead now filters with its own copy. Building
ahead also stands down for a reverb that downsamples and while anything is being loaded.

Sent early changes only when a task runs, never what it is or their order: the check (lockstep) doesn't exercise it.

## Reading it

- `debug.log`, a line at each lap's end: `FISHMOD-AUDIO lap L level V ...` as the audio laps'; `level` bit 0: built
  ahead, bit 1: sent early. `us / frames` is the audio thread's time a game frame. (With the tasks sent early the
  scheduler no longer interrupts the update to start one, so `us` falls by that much without the driver doing less:
  compare the frame log's CPU times too.)
- The frame log by lap: frames over 52 ms, the CPU's update and draw, the RSP's graphics time on the clock, the RDP.

## In ares (the emulator's timing, not the console's)

All eight laps, then FISHMOD-DONE:

| lap | way | frames over 52 ms | frame | CPU update + draw | RSP graphics | RDP |
|---|---|---|---|---|---|---|
| 1 | as always | 22.0% | 50.84 | 45.49 | 14.71 | 20.17 |
| 5 | as always | 23.7% | 51.26 | 44.54 | 15.26 | 20.42 |
| 2 | built ahead | 23.6% | 50.84 | 44.97 | 14.70 | 19.74 |
| 6 | built ahead | 20.2% | 50.82 | 44.51 | 15.33 | 20.52 |
| 3 | sent early | 20.5% | 50.88 | 45.49 | 15.18 | 15.86 |
| 7 | sent early | 22.4% | 51.07 | 45.52 | 15.11 | 15.78 |
| 4 | both | 18.5% | 50.78 | 45.28 | 15.17 | 15.87 |
| 8 | both | 17.9% | 50.74 | 45.22 | 15.05 | 15.70 |

Both together: 18% of frames over 52 ms against 23%. Sent early takes 4.5 ms off the RDP's time as ares counts it
(the graphics task is no longer stopped mid-frame), whichever way the updates are built. ares doesn't time the RSP or
the RDP as the console does: the console is the test.

## A layout trap this run fell into first

The first build with `--expak` ran 75% of frames over 52 ms in ares against 23% without, with neither switch on. Its
100 KB of new buffers had moved the kernel's thread records onto busy lines of the data cache: `__osDispatchThread`
took three times the data cache fills and four times the code's for the same instructions, 4.3 ms a frame under the
bench's PC sampler (4,000 interrupts a second, a thread switch each). Fixed by pinning the audio driver's sections
(src/audio/fishmod_audio_pin.c, as fishmod_pin.c pins the mod's own) and keeping what `--expak` adds inside them:
every engine function is at the same address with and without the flag, and the data after the driver is a whole
number of data caches further on. 24% against 20% after.
