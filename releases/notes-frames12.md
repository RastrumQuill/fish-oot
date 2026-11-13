# frames12: less waiting on memory

**`fish-oot-frames12.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --gcc-all`): frames11b (see
[its notes](notes-frames11b.md)) plus changes aimed at the CPU's waits on memory. The console's per-instruction profile,
read against the calibration loops (a data-cache miss puts its wait on the instruction after the load), put about
17 ms of a 55 ms frame (31%) on stalled loads. `fishmod/tools/inst_profile.py FOLDER --elf ELF --stalls 40` lists the
worst with their source lines.

- **The actor's list links and routines** (prev, next, init, destroy, update, draw) moved from 0x120 to just after
  its flags: every walk over the actor lists (update, draw, attention: three a frame) reads two lines an actor, not
  three far apart. Collision logs with and without: the same over 6,715 frames.
- **The collision lists made contiguous**: the static lookup's nodes rewritten once at load so each cell's list is in
  consecutive slots (they were scattered across the table: a miss a node). Collision logs with and without: the same
  over 8,803 frames.
- **Audio**: a channel with nothing to do is checked in one cache line (its flag moved into the first byte, checked
  before the frequency is read); each player keeps a mask of its enabled channels, so its update reads no disabled
  channel. Audio check with and without: the same over 15,221 buffers, every sample.
- **The fish manager**: the schools' ranges found once, their leaders re-picked every 4th frame (it swept all 300
  roster records every frame: my regression since frames9).
- **The bench**: the heaps' use sampled every 32nd frame (a walk of every block, every frame: it inflated the CPU
  times of every bench so far a little).

Not yet: the props' and the fish's own data (spread thin over many fields: for the layout tool, which learns which
fields are read together from recorded accesses), and the actor framework's Z-target search over the fish.

Compare with frames11b (the bisection, same queue): the CPU's update and draw, and `inst_profile.py --stalls` on both.
