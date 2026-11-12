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

## Results (console, 12 Nov, morning run 20261112-1127)

The fish bench (in the lab tank, two passes; `fishmod/tools/stress_prof.py`), ms a frame:

| fish | fps | over 52 ms (of 320) | CPU update | CPU draw | RSP | RDP | fish update | fish draw |
|---|---|---|---|---|---|---|---|---|
| 6 | 19.9 | 1 | 10.3 | 7.2 | 16.2 | 28.1 | 3.2 | 1.7 |
| 12 | 19.9 | 0 | 11.2 | 7.5 | 15.1 | 28.0 | 4.2 | 1.8 |
| 18 | 19.9 | 2 | 12.2 | 8.1 | 16.0 | 28.2 | 5.5 | 2.0 |
| 24 | 19.9 | 11 | 12.9 | 12.1 | 21.6 | 30.5 | 5.1 | 4.9 |
| 30 | 19.9 | 3 | 12.6 | 13.2 | 25.6 | 32.1 | 5.7 | 4.3 |
| 36 | 19.9 | 18 | 12.6 | 14.9 | 28.7 | 34.0 | 5.8 | 4.7 |
| 42 | 18.4 | 46 (of 445) | 16.2 | 13.3 | 22.3 | 31.4 | 5.5 | 3.2 |

**30 fish held full speed.** On average nothing is near the 50 ms frame (CPU 25-30, RDP 28-34, the CPU idle about 15
ms a frame): what fails the steps from 24 fish is a few slow frames (spikes), not the steady cost. The fish's own
update is 5-6 ms for all of them from 18 fish (the far ones cheap), their drawing 2-5 ms.

The plant bench (the kelp forest, two passes):

| plants | fps | over 52 ms | CPU update | CPU draw | RSP | RDP | props draw |
|---|---|---|---|---|---|---|---|
| 40 | 19.9 | 7 of 320 | 20.3 | 15.3 | 27.3 | 42.1 | 0.5 |
| 80 | 16.3 | 279 of 320 | 20.5 | 15.7 | 39.9 | 50.9 | 1.1 |
| 120 | 15.0 | 294 of 294 | 20.3 | 16.0 | 47.4 | 59.1 | 1.2 |

**40 plants held full speed** (stress1 found 40 already over the frame). Past that it's the RDP alone: about 0.22 ms
of RDP time per plant (42 -> 51 -> 59 ms), the CPU flat at about 36 ms. The profile shows nothing of the plants on the
CPU (their drawing 0.5-1.2 ms): the kelp forest is graphics-bound, as it should be.

(The first plant step's FISHMOD-STRESS line has a wrong frame count: the fish bench's pass sums carried over; fixed
for the next stress build. The frame records weren't affected.)
