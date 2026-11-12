# frames9: fish AI work (schools, fish days, things told to the fish), and who pays for collision

**`fish-oot-frames9.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --gcc-all`): frames8b (see
[its notes](notes-frames8b.md)) plus:

- **No repeated raycasts along the shores.** Cells of the baked floor map with a corner on top of a bank sent every
  floor question to a fresh collision raycast (about half a millisecond each; about 2,300 such cells in Lake Hylia).
  They now go through the exact-spot cache the bank cells already used.
- **School members far from attention just keep their places** (past 300 units from the camera and the bobber):
  into their slot behind the leader, with the leader's water, instead of steering and looking at the water
  themselves.
- **Schools pass the lead on.** The first free member leads; one at the bobber, hooked or caught leaves the school and
  the next leads the rest. Members keep room from the one ahead of them.
- **Each fish has a day:** its home (cover) and a feeding place a little way off, where it goes at its species'
  active times. Unspawned, it travels on toward where its day says from where it was last seen: it appears where it
  got to, and a fish chased and left is still on its way when Link comes back.
- **Told to the fish round about** (by the manager, instead of each fish looking): Link wading or swimming, and
  natural bait's scent spreading the longer it soaks. A fish that got away with a bait or broke off on it leaves that
  bait mostly alone for the day.
- **The bench's actor report:** at the end of the run (before `FISHMOD-DONE`), `FISHMOD-BGCHECK id calls us` (each
  actor kind's collision checks over the run) and `FISHMOD-ACTOR id instance-frames us` (each kind's updates), in the
  debug log: who pays for the 6-7 ms of collision a frame.

Checked in ares: the whole short route to `FISHMOD-DONE`, no crash. What to compare with frames8b on the console: the
fish update (frame log word 12) on legs 2, 9 and 10, the slow frames on the CPU-bound legs (2, 6, 9), and the actor
report. The fish behave differently, so the sound log differs from frames8b's wherever the fish make the sounds.
