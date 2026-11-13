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

- **The fish and their manager in actor lists of their own** (ACTORCAT_FISH, ACTORCAT_FISHMGR, after the others):
  the Z-target search no longer walks the fish (it read every fish's flags each frame to skip them). The manager still
  updates and draws after the fish; the fish now update after the other actors rather than among the NPCs (a change
  in order, not in what they do).
- **The fish's hot fields together**: the 20 fields the fish code waits on most (state, timers, flags, species,
  speed, sizes, water heights, swim phase) moved right after the actor, 3-4 cache lines where they were a dozen.
- **The props**: looked at and left as they are: 12-byte records already in chunk-and-kind order, the distance test
  reading only what it needs; their remaining waits are the records streaming in, which only drawing fewer props
  shrinks.

**`fish-oot-frames12-audio2.bps`**: the same, with the audio driver's own two-refresh mode (`-DFISHMOD_AUDIO_EVERY2`:
the spec's `unk_04` forced to 2; unused in vanilla, but the frame size, AI buffer limits, ticks per update, sample
DMAs and reverbs all scale with it in the driver). Half the audio thread's wake-ups and RSP tasks, the same ticks; the
sound up to a refresh (about 17 ms) later. Not bit-exact by design (game sounds start on the next audio frame): listen
for anything off. Three refreshes doesn't fit the AI buffers (1,600 samples against 1,408).

Compare with frames11b (the bisection, same queue): the CPU's update and draw, and `inst_profile.py --stalls` on both.
