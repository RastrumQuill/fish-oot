# frames14

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
