# frames8: run-ahead, the collision record, the props' screen edges

**`fish-oot-frames8.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --gcc-all`): frames7 plus the
changes below. The same route, frame log and profile. The frame log now also records how long each picture stays on
screen, so hitches can be counted as the TV shows them.

| change | checked |
|---|---|
| Run-ahead: a third frame buffer (the Expansion Pak), so a frame can be drawn while the last one waits its turn on screen; the CPU and RCP get a frame ahead and a fast frame's spare time goes to a slow one | ares: pictures held a 4th refresh 6.3% -> 5.0% of records, 19.58 -> 19.67 fps on screen. Collision log the same (1,317 frames) |
| The scheduler queues a second finished frame behind the first instead of losing its swap (needed for run-ahead; with two buffers it never happened) | as above |
| Collision: the floor search's first tests from one 16-byte record per scene poly (one cache line where the poly, three vertices and the box were up to five) | collision log: the same over 1,318 frames |
| Props: the screen's four edges as well as the round view cone; a prop wholly beyond them stays in the visible set (so the bubbles, which rise from a random plant in it, and their sounds are as before) but isn't drawn: about 8% fewer props drawn in ares | the rotation check: no pixel newly missing; audio check sample-exact; collision log the same |
| Props: each chunk's kind boundaries made once at load, not by a pass over its props in every rebuild | graphics log: byte for byte the same (2,170 frames) |
| The sample store is 724 KB (the third frame buffer takes the top of the gap); the route uses 632 KB | |
| The profiler credits samples taken while interrupts were off to the caller that had them off | |

The frame log's records are 58 words (version bit 16); `framelog.py` reads both.
