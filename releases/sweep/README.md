# Sweeps

The builds of each shared sweep, as patches for Ocarina of Time NTSC 1.2 (US), with each build's symbols file.
`fishmod/sweep/queue.json` names them; `fishmod/sweep/README.md` says how a machine takes part.

## audio-1

The audio sweep on rc2 (default branch at `3c17d06a`): R the reference audio, A the current audio without the
Expansion Pak, B with `--expak`. `audio-1-R.bps` applies to the retail ROM; A and B apply on top of R's ROM.
68 units: six legs (states in two parts, ocarina, scenes, sequences, effects, scenes as child at night) in four
sound settings, headphones first.

### Status (17 November)

Running in the audio lead's cloud container, about 20 hours for all of it. Its results are kept in that container
for now, not on the `sweep-results` branch, so a second machine joining should start its worker with `--from-end`.

First unit finished (states, first part, headphones): the same sound as the reference in every buffer on A
and B, every run to its end. It was marked FAIL for "frames differ" (one leg read 925 frames against 924 on A).
That was the counter, not the game: an interrupt taken exactly at the counted instruction was counted as a call.
The count is now the median over a function's first five instructions; the unit will be run again.

Memory on that unit, against the reference, per game frame:

| | A (no pak) | B (`--expak`) |
|---|---|---|
| audio: data lines read in | +2.4% | -1.4% |
| audio: data lines written back | +2.3% | +0.6% |
| audio: code lines read in | -6.0% | -3.9% |
| rest of the game: data in / out / code in | -0.0% / -0.2% / -1.2% | +0.3% / -0.8% / -0.8% |
| memory bus, audio's share | -0.2% | -1.1% |
| memory bus, everything | -0.3% | -0.5% |
| cycles: audio / rest | +0.5% / -0.4% | -0.4% / -0.3% |

### Before rc2

The same sweep was started on the pre-rc2 tree and stopped when rc2 landed. What had finished passed (the same
sound as the reference in every buffer, every run to its end): the ocarina leg in stereo and headphones on A and
B, and the states leg in headphones on A.
