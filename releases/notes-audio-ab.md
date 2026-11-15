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
