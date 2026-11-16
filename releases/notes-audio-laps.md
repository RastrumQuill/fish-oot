# audio-laps and audio-laps-every2

Two runs to compare on the console: the audio driver woken at every refresh (as now) and at every second one.

- **`fish-oot-audio-laps.bps`** (`fishmod/build.sh --script audio_laps --hw --texpack --colpack --meshpack --gcc-all`):
  this branch as it is. The baseline, and the console's first run of the branch's two layout changes (the driver's
  hot functions together, the note's fields in a tick's order: 1.0% of the audio thread's cycles in ares).
- **`fish-oot-audio-laps-every2.bps`** (the same with `GCC_EXTRA_OPT=-DFISHMOD_AUDIO_EVERY2`): the driver's own
  two-refresh mode: half the wake-ups and RSP tasks, each of six ticks instead of three. The same ticks, so the same
  samples in the same order; a sound starts up to a refresh (17 ms) later. (frames13-audio2 was this mode, judged on one
  lap's hitches; here three laps each, with the audio thread's own time.)

Queue lines (about 12 minutes each on the console; 16 allowed), one after the other:

    releases/fish-oot-audio-laps.bps 16 audio-laps
    releases/fish-oot-audio-laps-every2.bps 16 audio-laps-every2

Each is three laps of the short bench on one power-on, every lap clear dawn, nothing switched. No Expansion Pak
needed beyond what frames25 uses.

## Reading it

- **The frame log** (`framelog.py FOLDER --by-leg`, its lap column; or the CSV by lap): frames over 52 ms, the CPU's
  update and draw, the RSP's audio time, the RDP's time. The RDP's is the one to watch for the memory bus: with the
  audio off altogether it fell 5.5 ms (releases/notes-audio-ab.md).
- **`debug.log`**, a line at each lap's end, as the audio A/B's: `FISHMOD-AUDIO lap L level 0 frames F updates U us T
  seq S notes N lists C n K`. `us / frames` is the audio thread's time a game frame; `updates` is halved in the
  every-2 run.

## In ares (the emulator's timing, not the console's)

The every-second-refresh ROM run to its end: three laps reported, then FISHMOD-DONE (the third lap 9.2 ms a game
frame). Two laps of each, run side by side:

| | audio thread, ms a game frame, lap 1 | lap 2 |
|---|---|---|
| every refresh | 11.3 | 10.7 |
| every second refresh | 10.3 (-9%) | 9.6 (-10%) |

RDRAM traffic a game frame (`fishmod/tools/busprof.py`, the play bench, 160 s of sound each):

| KB a game frame | every refresh | every second |
|---|---|---|
| CPU: the audio driver's code filled | 56.5 | 37.7 |
| CPU: its data filled / written back | 67.7 / 37.2 | 61.7 / 40.2 |
| RSP: the audio tasks' DMAs, in / out | 79.2 / 31.2 | 72.3 / 31.2 |
| **the audio's, together** | **271.8** | **243.1 (-11%)** |
