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

First unit finished:

```
[10:48:10] audio-1 t2.0: FAIL t2.0 (vm, 2026-11-17 10:48)
           R: 8 legs, 19236 buffers
           A vs R: SAME: 19236 buffers, every sample alike; all 6387 6388 | 770878 774622 +0.5% | 1365359 1360546 -0.4%
             memory audio lines/frame: data in 5976.0 6118.3 +2.4% | data out 2788.8 2854.0 +2.3% | code in 1933.9 1818.3 -6.0%
             memory rest lines/frame: data in 8416.2 8415.0 -0.0% | data out 5408.0 5399.6 -0.2% | code in 9077.4 8972.6 -1.2%
             memory bus KB/frame: audio 441.5 440.6 -0.2% | everything 1947.6 1941.3 -0.3%
             A FAILED: FRAMES DIFFER
           B vs R: SAME: 19223 buffers, every sample alike; all 6387 6387 | 770878 767416 -0.4% | 1365359 1361558 -0.3%
             memory audio lines/frame: data in 5976.0 5895.0 -1.4% | data out 2788.8 2804.7 +0.6% | code in 1933.9 1858.2 -3.9%
```

### Before rc2

The same sweep was started on the pre-rc2 tree and stopped when rc2 landed. What had finished passed (the same
sound as the reference in every buffer, every run to its end): the ocarina leg in stereo and headphones on A and
B, and the states leg in headphones on A.
