# The review fixes on the console (27 November)

Three things only the console can answer, queued in `fishmod/pc/queue.txt`.

**1. `fish-oot-fixab-new.bps` against `fish-oot-modstripsab-on.bps`** (old, new, old, new): the frame-log bench
(`pcprof_frames`, the short route at Lake Hylia; `fishmod/build.sh --script pcprof_frames --hw --texpack --colpack
--meshpack --gcc-all`). "Old" is the build of this morning's mod-strips test. "New" has everything since:

- the strip pass keeps the game's draw order wherever a surface blends (0.6% more triangle command bytes over the
  whole game, none at the lake);
- the remeshed lake bed's vertices carry their own normals (the bed's shading table is about 11.5 KB bigger; no work
  per frame);
- the D-pad's buttons on the C buttons' own code, the fishing fixes, the weather's guard.

What decides it: the frame time, the RDP time and the slow-frame share should be the same within the run-to-run
spread of the two "old" legs. A difference beyond that is a regression to find.

**2. `fish-oot-hwscenes.bps`** (`--script hw_scenes`): every scene in the entrance table by its first entrance, 150
frames each, Link running, rolling and swinging, opening cutscenes played out; each frame's record over USB and
`FISHMOD-DONE` at the end. In ares the walk no longer crashes (the grotto crash was the walk's own: no return point
for the grotto's exit). What it answers: that no scene crashes or hangs on the console with the strips, the texture
pack and the collision pack all on, and what each scene costs there. If it stops, the last scene in the log is the
one to look at.

**3. `releases/hwsuite/hw-replay-g14o2.bps`**: the recorded opening replayed from power-on (`fishmod/docs/hw-suite.md`).
It has never run on the console (its first upload failed on the image's size, since fixed; then it fell out of the
queue). It's an older build: the question is whether a recording replays on the console at all. It overwrites the
cart's save memory with the recording's empty one.

Not for the console: the leap, the lure, the bait and the D-pad were checked in ares by scripts (`fight_leap`,
`fight_lure`, `bait_keep`, `dpad`); how they feel is for a hand on the controller.
