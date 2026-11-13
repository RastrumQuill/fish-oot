# frames14

**frames14c and frames13r (13 November, evening).** Why frames14b had more hitches than frames13 (29.9% against
23.1%), found in the frame log's RDP counters and in ares:

- **Texture loading, every leg: +0.45 ms a frame** (the RDP's TMEM-busy counter, 3.1 to 3.56 ms). The terrain cut
  into areas replayed, before each area's triangles, every state command of the piece before them, every texture
  load included: 48 loads where the piece had 13. frames14c cuts by texture instead: the piece's own list loads each
  texture once, as the original, and calls each area's triangles under it (each area still culled on its own by its
  box). Back to 13 loads and 16 pipe syncs; ares: the same frame costs what frames13's does (1.86 ms of loads and
  syncs by the console's calibration, against frames14b's 2.41); the tour's 22 stops the same picture as without the
  mesh pack (mean shift -0.04 of a step).
- **Legs 3 and 4: +3 to +5 ms of RDP drawing, but another scene.** The bench seeded the game's random numbers once;
  how many a frame draws depends on timing, so a build that runs at another speed shifts every number after it. A
  Tektite then hops elsewhere and meets Link elsewhere: in frames14 it came up to the camera (big triangles to fill)
  and Link's path changed. ares: the same ROM run twice already differed by frame 100. The bench now reseeds the
  numbers every frame from the frame's number: two runs of one ROM keep Link in the same place through leg 3 and
  draw the same picture but for a few ripples. frames13's earlier runs can't be compared with the new ones: frames13r
  is frames13's drawing (no mesh pack, no rig cleanup) on the new bench, as the control.


**Rebuilt 13 November (frames14b, the same name):** the first frames14 run lit the terrain on the CPU every frame
(6.6 ms of CPU draw, 44% hitches against frames13-sound's 23%): its "did the lights change" check compared the
lights' padding, which isn't cleared each frame. Now only the bytes that light a vertex are compared, a small change
waits half a second (the sun moves 0.3 degrees a second: a day is 20 minutes), a jump is lit at once. ares: lit 2
times in 414 pictures. The first run's other numbers: 11% fewer vertices and 13% fewer triangles sent to the RSP
(4,477 to 3,985; 2,561 to 2,232), but no less RSP time (it waits on a full RDP buffer much of the time).

frames13 (with sound) plus three changes for the RSP, each checked before it went in:

- **The fish rig's lists without the commands that do nothing** (state set to the value it has, pipe syncs with
  nothing drawn since the last, a texture load repeating the last): 19-27% fewer commands in each fish's list,
  about 5% fewer in the whole frame. Proven: `fishmod/tools/rigcheck.py` walks the old and new lists and finds
  every vertex, matrix and triangle with the same state.
- **The lake's widest terrain pieces cut into areas** (build.sh `--meshpack`): each area has its own bounding box,
  so the RSP skips the areas out of view after eight corners instead of transforming all their vertices; identical
  vertices in a load are loaded once.
- **The lake's terrain lit by the CPU** (`Fishmod_Relight`): the RSP's lighting off for it, the colours computed from
  the frame's sky lights with F3DEX3's own maths, whenever the lights change (the sun moves as before), and from the
  normals as the lake-bed shading leaves them.

Checked in ares: every tour stop (22) captured with and without the mesh pack: the same picture (the average
brightness within 0.04 of a step; at most 0.04% of a stop's pixels more than 8 steps apart: edge pixels between areas
and the lighting's rounding). The first two tries found a brighter lake bed (the copies missed the bed shading).

Compare with frames13-sound (`20261112-2127`) and the RSP test's audio-on laps: the RSP's graphics time, the vertex
counts (F3DEX3's counters in the frame log), the hitches.
