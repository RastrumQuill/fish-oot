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

## Results (console, 12 Nov, run 20261112-1127: frames9 twice, frames8b between)

The same frame rate as frames8b (18.2 fps on screen; slow frames 47-49% against 44%). The CPU's update went up 1.6
ms a frame and its draw down 1.4 ms: no gain overall.
- The fish moved: the bench runs at dawn, their feeding time, so they're at their feeding places now. Their cost fell
  on legs 4-6 (leg 5: 5.8 -> 3.9 ms) and rose on legs 7-11 (leg 8: 3.7 -> 6.7 ms).
- The fish manager cost 0.6 ms more (the travelling fish looked at the water every pass, raycasts along the shores):
  fixed in frames10.
- The actor report, the whole run (ms a frame): Guays' collision 2.1 ms (3.1 of them, 0.69 ms a check), Tektites'
  1.6 ms (8.9 of them, 0.35 ms a check), Link's 0.84 ms; updates in all: the fish 5.9 ms (58 of them, 0.1 ms each),
  Tektites 3.1, Guays 2.8, Link 2.5.
