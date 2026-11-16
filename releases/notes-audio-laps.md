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

## On the console (16 November, 05:37; a run each, one after the other)

| | lap | frames over 52 ms | frame | CPU update + draw | RSP graphics | RSP audio | RDP | audio thread, ms a game frame |
|---|---|---|---|---|---|---|---|---|
| every refresh | 1 | 44.6% | 53.52 | 48.16 | 23.01 | 10.67 | 42.51 | 11.93 |
| | 2 | 41.0% | 53.35 | 47.32 | 22.94 | 10.06 | 42.28 | 11.35 |
| | 3 | 39.9% | 53.34 | 47.55 | 24.41 | 9.73 | 43.36 | 11.19 |
| every second refresh | 1 | 58.9% | 54.36 | 45.95 | 24.95 | 10.53 | 43.23 | 10.26 |
| | 2 | 55.9% | 53.54 | 45.41 | 24.90 | 9.93 | 42.76 | 9.82 |
| | 3 | 56.6% | 53.63 | 45.64 | 26.12 | 9.54 | 43.69 | 9.68 |

- **Every second refresh is worse on screen**: 57% of frames over 52 ms against 42%, though the audio thread's time is
  14% lower (1.6 ms a frame) and the CPU's update and draw 2.0 ms shorter. Visible hitches 23% against 21%. Not to
  be used.
- Why, most likely: a game frame is three refreshes, so with an update every two a frame gets one update or two,
  turn about, each twice the size: 6.3 ms of CPU and a 6.6 ms RSP task in one piece, or twice that. The frames with
  two go over; the frames with one can't go under the 50 ms cap, so nothing is won back. The graphics task's time on
  the clock is 1.9 ms longer (it waits out a longer audio task). Every refresh gives each frame the same three.
- **The branch as it is** (the baseline): the audio thread 11.2 to 11.9 ms a game frame; the audio A/B's whole laps
  on frames25's driver were 12.1 to 12.8 (another power-on, a day apart: about 7% lower, more than the 1% ares
  counted without the PC sampler, near the 4.5% it counted with it, which these builds carry). 42% of frames over
  52 ms against 45%: inside a lap's noise.
