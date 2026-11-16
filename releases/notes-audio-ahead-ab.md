# audio-ahead-ab

**`fish-oot-audio-ahead-ab.bps`** (`fishmod/build.sh --script audio_ahead_ab --hw --texpack --colpack --meshpack
--gcc-all --expak`): the audio's updates built ahead, against one a refresh, lap by lap on one power-on. **Needs the
Expansion Pak** (the first build with `--expak`: without the pak it says so and stops).

Queue line (about 16 minutes on the console; 20 allowed):

    releases/fish-oot-audio-ahead-ab.bps 20 audio-ahead-ab

## What it tests

audio-laps-every2 (releases/notes-audio-laps.md) showed that waking the driver half as often saves its time (14%) and
loses on screen (57% of frames over 52 ms against 42%): the updates and their RSP tasks came twice the size, one or
two a frame. Building ahead keeps what it saved and not what it cost:

- the update that takes the game's commands (one a game frame) also builds the next two refreshes' command lists and
  tasks, in the same wake, while the driver's code and data are in the caches;
- **the RSP's tasks are the same three a frame, the same size, one a refresh**; the output buffers are queued when
  they always were; a sound the game starts is heard as soon as it ever was (the batch starts on its commands);
- the updates are the game's own, only made early: **the audio check's 1,894 buffers are the same, sample for
  sample**, as the build without. It stands down (one update a refresh) whenever an update would need what only
  happens between refreshes: a reverb that downsamples (it reads back the RSP's output), a reset of the audio heap.

Four laps of the short bench, every lap clear dawn: **built ahead in laps 1 and 3, one a refresh in 2 and 4**.

## Reading it

- `debug.log`, a line at each lap's end: `FISHMOD-AUDIO lap L level V ...` as the audio laps'; `level` 1: built ahead.
  `updates` counts the updates built (the same either way); `us / frames` the audio thread's time a game frame.
- The frame log by lap: frames over 52 ms, the CPU's update and draw, the RDP.

## In ares (the emulator's timing, not the console's)

The play bench, built ahead against not (two runs, 150 s each): the CPU's busy time 35.5 ms a game frame against
35.8 (-1%); the driver's code filled 45 KB a frame against 57, its data written back 47 KB against 40 (three lists
written one after the other); the audio's bus traffic 270 KB against 275. Next to nothing, in the emulator: the
console is the test (it gave 7% where the emulator said 1% for the layout).
