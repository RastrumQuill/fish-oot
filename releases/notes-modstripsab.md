# The mod-strips A/B (27 November)

**`fish-oot-modstripsab-off.bps`** and **`fish-oot-modstripsab-on.bps`**: the frame-log bench (`pcprof_frames`, the
short route at Lake Hylia), the same build twice. Both draw the game's scenes and models as triangle snakes (now the
default). The one difference: in `-on` the mod's own models, the lake's props and plants and its rocks, are snakes too
(`fishmod/tools/snake.py`). `-off` leaves them as triangle pairs (`FISHMOD_MODSTRIPS=0 fishmod/build.sh --script
pcprof_frames --hw --texpack --colpack --meshpack --gcc-all`).

- Props and plants: 1,075 triangle commands become 710 (194 snakes), about a third fewer bytes. A triangle is turned
  only when it is small or its corners shade alike, as for the scenes.
- Plants still bend: the bending code reads snakes. Kelp and the rock tour look the same in ares; what differs between
  runs is the sway's timing and particles, not the geometry.

**Run**: as a frames bench (SummerCart64 log), off then on, twice each, on one power-on if you can. Send the logs.
What decides it: each leg's frame time and RDP time, and the RSP's stall on the RDP, at the lake.

## Results (console, run 20261127-1100: off, on, off, on)

| | off | on | difference |
|---|---|---|---|
| fps | 17.5 | 17.6 | +0.1 |
| average frame time | 57.0 ms | 56.7 ms | -0.3 ms |
| frames over 52 ms | 64% | 60% | -4 points |
| visible hitches (picture up 4+ refreshes) | 49.5% | 47% | -2.5 points |
| RDP / RSP graphics | 46.7 / 25.3 ms | 46.8 / 25.2 ms | flat |

The gain is in the plant beds: the rock garden to the west weed flat goes from 54.4 to 52.3 ms (57% to 41% slow
frames), and the weed flat itself from 56.2 to 53.5 ms (75% to 51%). Elsewhere it's within noise. The mod's strips stay on.
