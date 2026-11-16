# rc1-props: rc1 with the props and fish CPU work

**`fish-oot-rc1-props-bench.bps`** (`fishmod/build.sh --script pcprof_frames --hw --texpack --colpack --meshpack
--gcc-all`): rc1 with claude/fish-prop-headroom merged (releases/notes-frames20-headroom.md: a set kept for a turn
under 4 degrees and a move under 24 units, made in one pass, each drawn prop a 16-byte entry made with the set,
materials as lists of their own, colours sent only when they change; far school followers every 2nd or 4th frame; the
bubbles' matrices written directly). On frames20 it took the CPU's draw from 25.0 to 21.3 ms in slow frames, the
props' draw from 7 to 2.9 ms; the RDP was the ceiling there, so +0.1 fps.

Brought onto rc1's props (fishmod_scatter.c had changed a lot since frames20):
- **Plants making way**: their reach test (only chunks and props within Link's reach read) with rc1's rope and stalk
  bending, hanging plants and the spring.
- **The set**: theirs, with rc1's camera-inside test (now with room for the camera's move before the next set) and
  the chunk test at the height nearest the look. rc1's guard for a full candidate list dropped (never reached).
- **Drawing**: their entries, with rc1's water fog (each entry's depth step worked out with the set, in an array
  beside the entries) and the bending in bands for a touched plant.

Checked in ares: the bench's frames 600 and 1230 drawn, no prop missing (popin_check.py, updated for the set's new
layout); the kelp bend test bends. The route drifts a little after frame 600 (the set's random numbers come at
other times), as the frames20 pair did.

The queue: rc1 and rc1-props, A B A B.
