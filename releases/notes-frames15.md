# frames15

**Console, 14 November (run 20261114-0720), frames15 against the control frames15L (every fish lit):** RDP 44.7 ->
44.5 ms, RSP graphics 23.7 -> 23.2 ms, hitches 39.4% -> 38.3% of pictures; CPU draw +0.1 ms. Most legs within noise,
the dock leg (6) 1.3 ms a frame faster. As the RDP sweep says, the fish are 1.3 ms of the RDP in all: flat fish
can't save much more. The props (8.5 ms) are next.

**`fish-oot-frames15.bps`** and **`fish-oot-frames15L.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack
--meshpack --gcc-all`, and `--script pcprof_frames_flatoff` for 15L): the same code, the reseeded bench of frames14c.
15L draws every fish lit, as before: the control. 15 draws the fish flat past arm's length.

**Flat fish.** The fish needn't look exactly as the vanilla model's material makes them, only good; under water, past
a couple of body lengths, the lighting hardly shows. The model's material is texture x lighting x tint, then fog in
the blender: two cycles a pixel (half the fill rate) and a shaded triangle. Flat:

- **Rigged fish past 200 units from the camera** (fishmod_rig.c, `Rig_Flatten`): a second copy of the rig's list
  with the material rewritten: one cycle; the combiner texture x tint + fog colour x fog; each limb's own alpha (the
  fins still cut out) and its own anti-aliased render mode (the colour buffer not read back, as the lit fish since
  frames11); lighting, fog and shade off (a triangle without shade is a shorter RDP command, and the RSP lights no
  vertices). The fog is one amount for the fish (`EnFishmod_FogAmount`: what the RSP would give its middle, from the
  scene's fog as `Play_SetFog` sets it); the tints are scaled by the scene's light (`EnFishmod_FlatLight`: the ambient
  and 0.22 of the two directional lights, fitted on the lit fish: their brightness within 2-4%) and by (1 - fog), in
  the fish's own memory, made again only when its looks, the light or its fog (64 steps) change.
- **Far fish (billboards)**: one cycle too, the fog the same way (their corner colours by the light and 1 - fog, one
  environment colour each), instead of two cycles with blender fog.
- Closer than 200 units, and in the photo and when caught, the fish are lit as before.

**memmove a word at a time** (src/libc/memmove.c): bcopy comes to it in GCC builds, and it went byte by byte (the
props' drawing, Graph_Update, the relighting and the rod call it every frame). Four bytes at a time where source and
destination share their alignment, the same bytes in the same order of overlap. ares: its samples 1434 -> 878 over the
route.

**Checked in ares:** the fish behaviour check (`fishstate`, 14 fingerprints of every actor over 3,150 frames) the same
as before all of this; the follow camera in the Kokiri pond with every fish lit, as shipped (all near: no change) and
every fish flat (the same shape and pattern, brightness within 2-4%, without the bright spots on the side facing the
light); the bench route's pictures the same but for the ripples that differ between any two runs. CPU (ares, the
bench route): a rigged fish 230 -> 224 us, a far fish 78 -> 91 us (its fog worked out each frame).

**What to look at on the console:** RDP time (r[7]) and TMEM/pipe busy on the legs with fish in view, 15 against
15L; frames 15 should take less RDP for the same picture.
