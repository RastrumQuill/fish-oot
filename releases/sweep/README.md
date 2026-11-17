# Sweeps

The builds of each shared sweep, as patches for Ocarina of Time NTSC 1.2 (US), with each build's symbols file.
`fishmod/sweep/queue.json` names them; `fishmod/sweep/README.md` says how a machine takes part.

## audio-1

The audio sweep on rc2 (default branch at `3c17d06a`): R the reference audio, A the current audio without the
Expansion Pak, B with `--expak`. `audio-1-R.bps` applies to the retail ROM; A and B apply on top of R's ROM.
68 units: six legs (states in two parts, ocarina, scenes, sequences, effects, scenes as child at night) in four
sound settings, headphones first.

### Status (17 November, 17:08 UTC)

Running in the audio lead's cloud container; 20 of 68 units done. Its results are kept in that container for now,
not on the `sweep-results` branch, so a second machine joining should start its worker with `--from-end`.

"same" means the same sound as the reference in every buffer. Figures are against the reference, per game frame
(A / B).

| unit | sound, A and B | check | audio cycles | rest cycles | bus, audio | bus, everything |
|---|---|---|---|---|---|---|
| headphones: states 1 of 2 | same | rerun (counter) | +0.5% / -0.4% | -0.4% / -0.3% | -0.2% / -1.1% | -0.3% / -0.5% |
| headphones: states 2 of 2 | same | pass | +1.1% / +1.2% | -0.2% / -0.7% | +0.7% / -1.9% | -0.3% / -2.4% |
| headphones: ocarina | same | pass | +0.5% / -0.6% | -0.4% / -0.3% | -0.1% / -1.2% | -0.3% / -0.5% |
| headphones: scenes 1 of 4 | same | pass | -0.7% / +0.4% | -2.7% / -3.9% | -1.8% / -2.5% | -2.5% / -3.9% |
| headphones: scenes 2 of 4 | same | pass | -0.8% / -0.3% | +0.6% / -0.6% | -1.6% / -2.2% | +0.1% / -0.7% |
| headphones: scenes 3 of 4 | same | pass | -0.8% / +1.3% | -0.2% / -1.9% | -2.4% / -3.3% | -0.6% / -3.0% |
| headphones: scenes 4 of 4 | same | pass | +0.2% / +0.9% | +0.1% / -1.3% | -0.7% / -2.4% | -0.3% / -2.3% |
| headphones: sequences 1 of 4 | same | pass | +0.8% / -0.1% | -0.3% / -0.2% | +0.6% / -0.7% | -0.1% / -0.3% |
| headphones: sequences 2 of 4 | same | pass | +0.8% / -0.0% | -0.3% / -0.2% | -0.2% / -1.0% | -0.3% / -0.4% |
| headphones: sequences 3 of 4 | same | pass | +1.0% / +0.1% | -0.2% / -0.2% | +0.3% / -0.5% | -0.2% / -0.3% |
| headphones: sequences 4 of 4 | same | pass | +0.7% / -0.1% | -0.3% / -0.2% | +0.1% / -0.7% | -0.2% / -0.3% |
| headphones: effects 1 of 2 | same | pass | +0.9% / -0.8% | +0.2% / +0.4% | -0.0% / -1.4% | -0.5% / -0.5% |
| headphones: effects 2 of 2 | same | pass | +0.7% / -0.9% | +0.1% / +0.3% | +0.9% / -1.6% | -0.2% / -0.6% |
| headphones: scenes, child at night 1 of 4 | same | pass | -0.7% / +0.5% | -2.7% / -3.9% | -1.5% / -2.6% | -2.6% / -4.1% |
| headphones: scenes, child at night 2 of 4 | same | pass | -1.4% / -0.5% | +0.5% / -0.9% | -2.2% / -2.8% | -0.1% / -1.1% |
| headphones: scenes, child at night 3 of 4 | same | pass | -1.1% / +1.5% | -0.3% / -1.9% | -2.0% / -1.6% | -0.6% / -2.0% |
| headphones: scenes, child at night 4 of 4 | same | pass | -0.3% / +1.8% | -0.2% / -1.7% | -1.1% / -1.2% | -0.4% / -1.7% |
| stereo: states 1 of 2 | same | pass | +0.7% / -0.1% | -0.4% / -0.3% | +0.6% / -0.6% | -0.1% / -0.3% |
| stereo: states 2 of 2 | same | pass | +1.3% / +1.5% | -0.3% / -0.7% | +1.1% / -1.0% | -0.1% / -1.8% |
| stereo: ocarina | same | pass | +0.7% / -0.3% | -0.4% / -0.3% | +0.2% / -0.8% | -0.3% / -0.4% |

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
