# audio-ab

**`fish-oot-audio-ab.bps`** (`fishmod/build.sh --script audio_ab --hw --texpack --colpack --meshpack --gcc-all`): what
the audio costs a frame on the console, and how much of it is the RSP's and how much the CPU's. The game is frames25's;
nothing in the audio is changed, only switched off by lap and timed.

Queue line (about 21 minutes on the console; 26 allowed):

    releases/fish-oot-audio-ab.bps 26 audio-ab

Six laps of the short bench on one power-on, every lap clear dawn:

| lap | audio |
|---|---|
| 1, 4 | whole (the baseline) |
| 2, 5 | without its RSP tasks: the driver still runs and builds them (as rsp-ab's odd laps) |
| 3, 6 | without the driver's update either: no audio work at all, on the CPU or the RSP |

Laps 2-3 and 5-6 are silent. No Expansion Pak needed (nothing here uses one).

## Reading it

- **The frame log** (`framelog.py FOLDER --by-leg`, its lap column): frame period and frames over 52 ms. Laps 1 and 4 against 2 and 5 is the
  RSP's audio work; 2 and 5 against 3 and 6 is the audio driver's CPU work; 1 against 4 (and so on) is the run's drift.
- **`debug.log`**, one line at each lap's end, counted by the console itself:

      FISHMOD-AUDIO lap L level V frames F updates U us T seq S notes N lists C n K

  `level` 0, 1, 2 as the table's rows; `frames` the game's frames in the lap; `updates` the audio thread's updates (one a
  refresh); then microseconds over the lap: `us` the whole update (`AudioThread_UpdateImpl`), `seq` the sequences with
  their notes' updates (`AudioSeq_ProcessSequences`), `notes` the per-tick note bookkeeping (`func_800DB03C`), `lists`
  the RSP command lists (`AudioSynth_DoOneAudioUpdate`); `n` the notes put in the lists (three ticks an update).
  `us / frames` is the audio's CPU time a game frame; the gap between `us` and the three parts is loads and commands.
  In the level 2 laps these are near zero (the driver doesn't run).
- The PC profile covers all six laps together, so it's lighter on audio than a normal run's: use frames25's for that.

## In ares (the emulator's timing, not the console's)

The run completes all six laps, the driver-off laps included (the audio comes back in lap 4), and says FISHMOD-DONE.

| lap | audio | frames over 52 ms | audio update, ms a game frame | sequences and notes | command lists |
|---|---|---|---|---|---|
| 1 | whole | 33% | 12.1 | 6.1 | 5.1 |
| 2 | no RSP tasks | 22% | 11.2 | 5.8 | 4.5 |
| 3 | no driver | 0% | 0 | | |
| 4 | whole | 26% | 11.0 | 5.6 | 4.6 |
| 5 | no RSP tasks | 22% | 10.7 | 5.5 | 4.3 |
| 6 | no driver | 0% | 0 | | |

About 51 notes an update (17 a tick). ares has no RDRAM wait and doesn't time the RSP as the console does, so the
console's numbers are the ones that count.

## On the console (15 November, 15:08; one power-on, the six laps)

Per lap, the bench's counted frames (framelog.py's CSV), ms a frame:

| lap | audio | over 52 ms | frame | CPU update | CPU draw | RSP graphics | RSP audio | RDP |
|---|---|---|---|---|---|---|---|---|
| 1 | whole | 47.7% | 53.97 | 23.70 | 25.31 | 22.74 | 10.91 | 42.58 |
| 2 | no RSP tasks | 30.6% | 52.45 | 21.59 | 23.88 | 27.34 | 0 | 39.50 |
| 3 | no driver | 12.8% | 51.29 | 15.69 | 19.25 | 27.15 | 0 | 37.01 |
| 4 | whole | 42.4% | 53.67 | 23.01 | 25.59 | 23.97 | 9.81 | 42.80 |
| 5 | no RSP tasks | 31.3% | 52.59 | 21.54 | 24.43 | 28.89 | 0 | 40.34 |
| 6 | no driver | 13.5% | 51.42 | 15.90 | 19.15 | 27.29 | 0 | 36.89 |

- **The audio is 32 points of slow frames** (45% with it, 13% without): **14 the RSP's tasks** (45% to 31%), **18 the
  driver's CPU work** (31% to 13%). The two rounds agree within 5 points (the whole laps), 1 point (the others).
- Without the RSP tasks the CPU's update and draw are 3.1 ms shorter and the RDP 2.8 ms; without the driver another
  10.9 ms of CPU and 3.0 ms of RDP (the driver's own time, below, and what its memory traffic costs the RDP).
- The RSP's graphics time reads 4 ms longer without the audio's tasks: the figure is the graphics task's time on the
  clock, which includes waiting on the RDP; it isn't more work.

The audio thread's own time (FISHMOD-AUDIO), ms a game frame:

| lap | whole update | sequences and notes | note bookkeeping | command lists | the rest |
|---|---|---|---|---|---|
| 1 | 12.81 | 6.78 | 0.26 | 5.03 | 0.74 |
| 2 (no RSP tasks) | 11.84 | 6.31 | 0.27 | 4.56 | 0.69 |
| 4 | 12.11 | 6.45 | 0.29 | 4.64 | 0.73 |
| 5 (no RSP tasks) | 11.69 | 6.22 | 0.28 | 4.50 | 0.70 |

The command lists cost 30-31 microseconds a note a tick. The console's update is 6-10% longer than ares' (12.1 and
11.0 ms in the same laps): for the audio thread's own time, ares is a fair stand-in.
