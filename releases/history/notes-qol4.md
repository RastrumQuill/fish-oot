# qol4

qol3 (see [its notes](notes-qol3.md)) plus:
- **The D-pad is always shown** with the C buttons, empty or not (an empty direction shows its pressed-in arrow). It
  used to hide until something was put on it.
- **Speedups** from the console profile: `memset` a word at a time (it was 2% of the CPU byte by byte) and a faster
  matrix conversion (1.6% + 1.1%). See [pcprof2's notes](notes-pcprof2.md).
- **The occlusion plane and the bed cards are off.** The console's four-way bench (fourway2: 4 laps, no crash) found
  them costing slightly more than they saved: with both off, frames were 0.8% faster and the RDP 0.5%. The code stays
  (a profile switch each: `f3dex3_occlusion`, `bed_cards` in `fishmod/data/profiles.json`).

Patches: `fish-oot-qol4-testmode.bps` (emulators), `fish-oot-qol4-hw-testmode.bps` (console).
