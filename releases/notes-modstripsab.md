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
