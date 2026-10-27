# Test 5n: the play bench, soak-tested

## The patch
- **`fish-oot-hwbench-play-5n.bps`**: the play bench (fresh save; send the `.sav`). It stops by itself after 8 laps
  (about 7 hours on the console) and shows "BENCH DONE".

## Tested first
This exact ROM ran for 4 hours in ares before being sent: 3 full laps and part of a fourth (clear dawn, clear dusk,
rain midday, rain night), 70 legs, every mode timed on every leg, no crash and no stall. The emulator's numbers are in
`soak-5n-ares.md`.

## If it stops
5l stopped on the console after about 70 seconds while the emulator ran on, so this build records why:
- results are saved after every timing block (about every 13 seconds);
- a crash or a graphics hang writes where it happened into the save;
- a watchdog writes where the game was stuck if frames stop for 10 seconds for any other reason (an endless loop, a
  wait that never ends).
Either way, send the `.sav`: even a short run is now readable.
