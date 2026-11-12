# fishopt1: the play bench on the cheaper fish update (console)

The play bench (Lake Hylia: walking, swimming, diving and fishing, every time of day and weather), as fourway2's but
with today's game: compare its **base** rows with fourway2's **base** rows. Built from the commit that adds this file
with `fishmod/build.sh --script hwbench_play --hw`.

## The patch
- **`fish-oot-fishopt1-hw.bps`**: console (F3DEX3). **Blank save**: it writes over the save files. About an hour a lap,
  saved after every leg; four laps is plenty. Send the `.sav`.

## What changed since fourway2
- **The fish update** (fourway2's fish timers: 6.7 ms a frame over 44 fish updates):
  - a fish not drawn in full detail (far, or a flat card) keeps only its swim wave's bookkeeping, not the wave's
    angles (only the full model bends);
  - the attention distance once a frame, not twice;
  - the engine no longer works out each fish's distance and angle to Link every frame (a square root and an
    arctangent each; fish never read them).
  Fish behave exactly as before: the scripted regression runs' state fingerprints (every actor's position and
  rotation) are identical before and after.
- **Benches time the game as it ships:** the fish's debug readout (2.4 ms a frame in fourway2, never in the game) is
  left out of benches. So part of the fish update's drop here is the bench, not the game: the report's timers say
  which.
- The `memset` and matrix fixes (see pcprof2's notes), and the occlusion plane and bed cards off.

Read with `fishmod/tools/hwbench_report.py SAVE` and `fishmod/tools/read_stats.py --prof SAVE`.
