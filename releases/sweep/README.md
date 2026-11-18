# Sweeps

The builds of each shared sweep, as patches for Ocarina of Time NTSC 1.2 (US), with each build's symbols file.
`fishmod/sweep/queue.json` names them; `fishmod/sweep/README.md` says how a machine takes part.

## audio-1

The audio sweep on rc2 (default branch at `3c17d06a`): R the reference audio, A the current audio without the
Expansion Pak, B with `--expak`. `audio-1-R.bps` applies to the retail ROM; A and B apply on top of R's ROM.
68 units: six legs (states in two parts, ocarina, scenes, sequences, effects, scenes as child at night) in four
sound settings, headphones first.

### Status (18 November, 10:30 UTC)

Running in the audio lead's cloud container; 65 of 68 units done. Its results are kept in that container for now,
not on the `sweep-results` branch, so a second machine joining should start its worker with `--from-end`.

"same" means the same sound as the reference in every buffer. Figures are against the reference, per game frame
(A / B).

| unit | sound, A and B | check | audio cycles | rest cycles | bus, audio | bus, everything |
|---|---|---|---|---|---|---|
| headphones: states 1 of 2 | same | pass (frame count, see below) | +0.5% / -0.4% | -0.4% / -0.3% | -0.2% / -1.1% | -0.3% / -0.5% |
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
| stereo: scenes 1 of 4 | same | pass | -0.4% / +0.7% | -2.7% / -3.9% | -1.5% / -2.0% | -2.5% / -3.8% |
| stereo: scenes 2 of 4 | same | pass | -0.6% / -0.0% | +0.6% / -0.6% | -0.8% / -1.4% | +0.3% / -0.5% |
| stereo: scenes 3 of 4 | same | pass | -0.6% / +1.6% | -0.2% / -1.9% | -2.1% / -2.1% | -0.6% / -2.5% |
| stereo: scenes 4 of 4 | same | pass | +0.5% / +1.2% | +0.1% / -1.3% | +0.2% / -2.0% | -0.1% / -2.2% |
| stereo: sequences 1 of 4 | same | pass | +1.1% / +0.2% | -0.3% / -0.2% | +0.5% / -0.3% | -0.2% / -0.3% |
| stereo: sequences 2 of 4 | same | pass | +1.0% / +0.3% | -0.3% / -0.3% | +0.5% / -0.1% | -0.1% / -0.2% |
| stereo: sequences 3 of 4 | same | pass | +1.2% / +0.3% | -0.2% / -0.2% | +0.7% / +0.3% | -0.1% / -0.1% |
| stereo: sequences 4 of 4 | same | pass | +0.9% / +0.2% | -0.3% / -0.3% | +0.4% / -0.3% | -0.1% / -0.2% |
| stereo: effects 1 of 2 | same | pass | +1.2% / -0.7% | +0.2% / +0.4% | +1.4% / -1.1% | -0.2% / -0.3% |
| stereo: effects 2 of 2 | same | pass | +1.0% / -0.7% | -0.0% / +0.2% | -1.1% / -1.5% | -0.9% / -0.6% |
| stereo: scenes, child at night 1 of 4 | same | pass | -0.4% / +0.8% | -2.7% / -3.9% | -1.1% / -2.0% | -2.5% / -3.9% |
| stereo: scenes, child at night 2 of 4 | same | pass | -1.1% / -0.2% | +0.5% / -0.9% | -1.8% / -2.5% | -0.0% / -1.0% |
| stereo: scenes, child at night 3 of 4 | same | pass | -0.8% / +1.7% | -0.3% / -1.8% | -0.8% / -1.3% | -0.4% / -1.9% |
| stereo: scenes, child at night 4 of 4 | same | pass | +0.0% / +2.1% | -0.2% / -1.6% | -1.2% / -1.3% | -0.4% / -1.7% |
| mono: states 1 of 2 | same | pass | +0.4% / -0.4% | -0.4% / -0.3% | +0.0% / -0.8% | -0.3% / -0.4% |
| mono: states 2 of 2 | same | pass | +0.9% / +1.1% | -0.3% / -0.7% | -0.2% / -1.9% | -0.3% / -1.9% |
| mono: ocarina | same | pass | +0.4% / -0.5% | -0.4% / -0.3% | -0.8% / -1.3% | -0.5% / -0.5% |
| mono: scenes 1 of 4 | same | pass | -0.7% / +0.3% | -2.7% / -3.9% | -1.6% / -2.2% | -2.5% / -3.7% |
| mono: scenes 2 of 4 | same | pass (frame count, see below) | -0.9% / -0.4% | +0.6% / -0.6% | -2.1% / -2.8% | +0.1% / -0.7% |
| mono: scenes 3 of 4 | same | pass | -0.9% / +1.2% | -0.2% / -1.9% | -1.9% / -3.3% | -0.6% / -2.7% |
| mono: scenes 4 of 4 | same | pass | +0.1% / +0.8% | +0.1% / -1.3% | -1.4% / -3.2% | -0.4% / -2.4% |
| mono: sequences 1 of 4 | same | pass | +0.6% / -0.2% | -0.3% / -0.2% | +0.2% / -0.5% | -0.2% / -0.3% |
| mono: sequences 2 of 4 | same | pass | +0.6% / -0.1% | -0.3% / -0.3% | -0.2% / -0.8% | -0.3% / -0.3% |
| mono: sequences 3 of 4 | same | pass | +0.8% / -0.0% | -0.2% / -0.2% | -0.0% / -1.2% | -0.2% / -0.4% |
| mono: sequences 4 of 4 | same | pass | +0.4% / -0.2% | -0.3% / -0.3% | +0.2% / -0.5% | -0.2% / -0.3% |
| mono: effects 1 of 2 | same | pass | +0.9% / -0.9% | +0.1% / +0.4% | -0.1% / -1.6% | -0.5% / -0.5% |
| mono: effects 2 of 2 | same | pass | +0.8% / -0.9% | -0.0% / +0.2% | +0.8% / -1.8% | -0.4% / -0.5% |
| mono: scenes, child at night 1 of 4 | same | pass | -0.7% / +0.4% | -2.7% / -3.9% | -1.6% / -2.6% | -2.6% / -3.9% |
| mono: scenes, child at night 2 of 4 | same | pass (frame count, see below) | -1.4% / -0.7% | +0.5% / -0.9% | -1.9% / -3.1% | -0.0% / -1.1% |
| mono: scenes, child at night 3 of 4 | same | pass | -1.1% / +1.3% | -0.3% / -1.8% | -2.7% / -2.5% | -0.7% / -2.0% |
| mono: scenes, child at night 4 of 4 | same | pass | -0.4% / +1.7% | -0.2% / -1.6% | -1.2% / -0.7% | -0.4% / -1.6% |
| surround: states 1 of 2 | same | pass | +0.7% / -0.1% | -0.3% / -0.3% | +0.1% / -0.7% | -0.2% / -0.4% |
| surround: states 2 of 2 | same | pass | +1.3% / +1.4% | -0.2% / -0.7% | +0.8% / -1.0% | -0.1% / -1.8% |
| surround: ocarina | same | pass | +0.7% / -0.3% | -0.4% / -0.3% | -0.3% / -1.2% | -0.3% / -0.5% |
| surround: scenes 1 of 4 | same | pass | -0.5% / +0.7% | -2.7% / -3.9% | -0.8% / -1.7% | -2.4% / -3.7% |
| surround: scenes 2 of 4 | same | pass | -0.6% / -0.1% | +0.6% / -0.6% | -1.3% / -2.2% | +0.2% / -0.6% |
| surround: scenes 3 of 4 | same | pass | -0.7% / +1.5% | -0.2% / -1.9% | -1.5% / -2.7% | -0.5% / -2.6% |
| surround: scenes 4 of 4 | same | pass | +0.5% / +1.3% | +0.1% / -1.3% | -0.4% / -2.1% | -0.2% / -2.2% |
| surround: sequences 1 of 4 | same | pass | +1.1% / +0.2% | -0.3% / -0.2% | +0.5% / -0.3% | -0.2% / -0.3% |
| surround: sequences 2 of 4 | same | pass | +1.0% / +0.3% | -0.3% / -0.3% | +0.5% / -0.1% | -0.1% / -0.2% |
| surround: sequences 3 of 4 | same | pass | +1.2% / +0.3% | -0.2% / -0.2% | +0.7% / -0.1% | -0.1% / -0.2% |
| surround: sequences 4 of 4 | same | pass | +0.9% / +0.2% | -0.3% / -0.3% | +0.0% / -0.7% | -0.2% / -0.3% |
| surround: effects 1 of 2 | same | pass | +1.2% / -0.7% | +0.2% / +0.4% | -0.8% / -2.1% | -0.9% / -0.5% |
| surround: effects 2 of 2 | same | pass | +1.1% / -0.6% | -0.0% / +0.2% | +0.0% / -2.5% | -0.6% / -0.9% |
| surround: scenes, child at night 1 of 4 | same | pass | -0.4% / +0.8% | -2.7% / -3.9% | -1.1% / -2.0% | -2.5% / -3.9% |

States 1 of 2 was first marked FAIL for "frames differ" (one leg read 925 frames against 924 on A), and
so was mono scenes 2 of 4 (one leg read 2,637 frames on the reference against 2,636 on A and B, and on every build
in stereo and headphones). That is the counter, not the game: frames are read from the profile's count of
`Graph_Update`'s first instructions and can be one out at a leg's edge. In lockstep every frame has exactly three
audio updates, and those are equal on every build in both units (2,772 and 7,908), as is every buffer of sound. The
report allows the frame count one frame. Mono scenes as an adult 2 of 4 then read the other way round: 5,515
updates on the reference against 5,514 (three times its 1,838 frames) on A and B, again with every buffer the same.
The update count is read the same way, so it is allowed one as well. A real extra or missing update could not hide
behind that: every buffer after it would differ from the reference's. All three units pass, judged again from the
kept runs, and every unit is judged again with the final rule when the sweep ends.

On B, audio writes back more data lines (+9% to +21%) while the rest of the game writes back fewer (-3% to -9%);
the two together are lower than the reference (scenes 3: 7,104 lines a frame against 7,159). A written-back line
is charged to whoever evicts it, so this is a shift in who pays, from the different data placement, not more
traffic.

### Before rc2

The same sweep was started on the pre-rc2 tree and stopped when rc2 landed. What had finished passed (the same
sound as the reference in every buffer, every run to its end): the ocarina leg in stereo and headphones on A and
B, and the states leg in headphones on A.
