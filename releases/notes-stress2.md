# stress2: the fish and plant stress benches on the current code, profiled

**`fish-oot-stress2.bps`** (`fishmod/build.sh --script stress_prof --hw --colpack --gcc-all`): the code of frames9
(see [its notes](notes-frames9.md)) running the two stress benches of stress1 (see
[its notes](history/notes-stress1.md)), now with the profile and frame log over USB instead of only the save:

1. **Fish:** adult Link in the Iron Boots at the bottom of the Lakeside Laboratory's tank, Lake Hylia's fish let in
   6 more a step.
2. **Plants:** then warped to Lake Hylia's deep basin at noon, the scene's own props off, a kelp forest ahead of him,
   40 more plants a step.

Each step settles, then is timed for 8 seconds; a pass ends one step after the first that can't hold 20 fps. Two
passes each, about 25 minutes in all, then `FISHMOD-DONE`. Each step's result comes as a `FISHMOD-STRESS` line, and
every frame record is tagged with its bench and step, so `fishmod/tools/stress_prof.py FOLDER --elf ELF` gives, per
step, the times (CPU update and draw, RSP, RDP, the fish's own, the props') and the costliest functions, and the
largest count that held 20 fps. The ELF is rebuilt from this commit with the build line above.

Checked in ares only as far as the fish bench's first ten steps (6 to 60 fish, all at 20 fps there); the rest runs on
the console.
