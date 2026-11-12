# frames8b-guard and the frames6 baseline: finding what overwrites the game's code

frames7, frames8b and rdpbench2 crashed on the console with **ordinary code overwritten in RAM** (a "TLB exception"
and a "Coprocessor unusable" at instructions that can't raise them). fourway1 crashed the same way in October; the
rebuilt bench then ran 4 laps cleanly and the cause was never found.

- **`fish-oot-frames6.bps`** (restored from the history): frames6 ran whole routes for hours before. Run under today's
  loading path (the deployer's upload, the cart's bootloader), it tells whether the problem is the new code or the
  setup.
- **`fish-oot-frames8b-guard.bps`**: frames8b with the code guard. The game's code is checksummed a block at a time, all
  of it every few seconds; when anything has changed it, the game stops with **"code changed at ..."** on screen and
  sends the changed block over USB (`FISHMOD-CODEGUARD` in the debug log). What the garbage is (audio samples, pixels,
  display list commands) says what wrote it. Checked in ares: no false alarm.

Start each from power-off: switch the console off and on when the runner beeps (not RESET).
