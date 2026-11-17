# Sweeps

The builds of each shared sweep, as patches for Ocarina of Time NTSC 1.2 (US), with each build's symbols file.
`fishmod/sweep/queue.json` names them; `fishmod/sweep/README.md` says how a machine takes part.

## audio-1

The audio sweep on rc2 (default branch at `3c17d06a`): R the reference audio, A the current audio without the
Expansion Pak, B with `--expak`. `audio-1-R.bps` applies to the retail ROM; A and B apply on top of R's ROM.
68 units: six legs (states in two parts, ocarina, scenes, sequences, effects, scenes as child at night) in four
sound settings, headphones first.

### Status (17 November, 12:35)

Running in the audio lead's cloud container; 7 of 68 units done in the first two hours, so about a day for all of
it. Its results are kept in that container for now, not on the `sweep-results` branch, so a second machine
joining should start its worker with `--from-end`.

Every finished unit has the same sound as the reference in every buffer on A and B, every run to its end and
equal frame counts. Against the reference, per game frame (A / B):

| unit (headphones) | buffers | audio cycles | rest cycles | bus, audio | bus, everything |
|---|---|---|---|---|---|
| states 1 of 2 | 19,236 | +0.5% / -0.4% | -0.4% / -0.3% | -0.2% / -1.1% | -0.3% / -0.5% |
| states 2 of 2 | 25,032 | +1.1% / +1.2% | -0.2% / -0.7% | +0.7% / -1.9% | -0.3% / -2.4% |
| ocarina | 18,232 | +0.5% / -0.6% | -0.4% / -0.3% | -0.1% / -1.2% | -0.3% / -0.5% |
| scenes 1 of 4 | 32,528 | -0.7% / +0.4% | -2.7% / -3.9% | -1.8% / -2.5% | -2.5% / -3.9% |
| scenes 2 of 4 | 28,066 | -0.8% / -0.3% | +0.6% / -0.6% | -1.6% / -2.2% | +0.1% / -0.7% |
| scenes 3 of 4 | 15,081 | -0.8% / +1.3% | -0.2% / -1.9% | -2.4% / -3.3% | -0.6% / -3.0% |

States 1 of 2 was first marked FAIL for "frames differ" (one leg read 925 frames against 924 on A). That was the
counter, not the game: an interrupt taken exactly at the counted instruction was counted as a call. The count is
now the median over a function's first five instructions; the unit will be run again at the end.

On B, audio writes back more data lines (+9% to +21%) while the rest of the game writes back fewer (-3% to -9%);
the two together are lower than the reference (scenes 3: 7,104 lines a frame against 7,159). A written-back line
is charged to whoever evicts it, so this is a shift in who pays, from the different data placement, not more
traffic.

### Before rc2

The same sweep was started on the pre-rc2 tree and stopped when rc2 landed. What had finished passed (the same
sound as the reference in every buffer, every run to its end): the ocarina leg in stereo and headphones on A and
B, and the states leg in headphones on A.
