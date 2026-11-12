# frames9: the first fish AI work (cheaper far schools, no repeated shore raycasts)

**`fish-oot-frames9.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --gcc-all`): frames8b (see
[its notes](notes-frames8b.md)) plus:

- **School members far from attention just keep their places.** Past 300 units from the camera and the bobber (beyond
  the full rig), a follower eases into its slot behind its leader and borrows the leader's water, instead of steering
  and looking at the water itself. Up close nothing changes.
- **No repeated raycasts along the shores.** Cells of the baked floor map with a corner on top of a bank used to send
  every floor question to a fresh collision raycast (about half a millisecond each; about 2,300 such cells in Lake
  Hylia). They now go through the exact-spot cache the bank cells already used.

What to compare with frames8b: the fish update (frame log word 12) on legs 2, 9 and 10, and the slow frames on the
CPU-bound legs (2, 6, 9). The fish behave a little differently far away, so the sound log differs from frames8b's
wherever the fish make the sounds.
