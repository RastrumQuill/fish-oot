# frames20

**`fish-oot-frames20.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --meshpack --gcc-all`): frames19
with the lake looking right, for the console's numbers on it (compare with frames19, run 20261115-0201):

- **The caustics** fade with depth (tools/meshpack.py caustic_fade): as before down to 150 below the surface, sand
  only past 550, in four steps; and they're light alone, no colour (the fog gives the water's), their textures i8
  (half the size to load). fishmod/docs/caustics-fade.png.
- **The rocks** made for where they lie (fishmod/tools/rock_mesh.py): 88 rocks, each its own mesh, triangles by size
  (a stone 15, a boulder 21, an outcrop 36: 1,776, all of them seen), collision from the same meshes. The 80 in the
  shallows have the caustics on them as light (2-cycle: the RDP pays twice a pixel for those).
- **Plants make way for Link**: swaying plants lean away and spring back; lily pads, their flowers and floating
  leaves drift aside and back.
- **Lily flowers** on their pads (they were placed from the wrong centre).

**The anatomy sweep** (run 20261115-0221, 36 laps, releases/notes-anatomy-sweep.md): the RDP is busy the whole frame
(44.4 ms); left out, the terrain saves 14.1 ms, the other actors 6.3, Link 5.2, the effects 0.9, everything 36.6
(7.6 ms left). The cost model (fishmod/tools/rdp_cost.py, the console's own bench) gives half of each: the whole
frame 19-22 ms, the everything-out frame about 1.5 ms of real work. In the game the RDP takes about twice what it does
alone.
