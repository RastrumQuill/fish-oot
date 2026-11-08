# fourway1: the occlusion plane and bed cards bench (console)

The play bench (Lake Hylia: walking, swimming, diving and fishing at every time of day and weather) with two new
ways of drawing less, each on and off. Built from commit `e36bfb8` with `fishmod/build.sh --script hwbench_fourway --hw`.

## The patch
- **`fish-oot-fourway1-hw.bps`**: console (F3DEX3 with the occlusion plane). **Blank save**: it writes over the
  save files. About an hour a lap; it saves after every leg, so stop it whenever you like and send the `.sav`.

## What it compares
Each leg is played eight ways: the first pass, then
- **base**: both on (as shipped)
- **noocc**: no occlusion plane (props hidden behind a bank, a cliff or a slope drawn anyway)
- **nobedcard**: no bed cards (distant chunks plant by plant)
- **neither**: both off
- and three references: no fish, no props, everything off.

## Checked in ares first
- The occlusion plane hides nothing that's visible: an exact frozen A/B at all 22 tour stops matches to within the
  emulator's video noise.
- The bed cards change at most about 500 pixels at a few stops, at the far edge of view.
- The bench runs through its modes without a crash. ares can't time the RDP, which is where both should help: the
  console run is the measurement.

Read with `fishmod/tools/hwbench_report.py SAVE` (bench version 9).
