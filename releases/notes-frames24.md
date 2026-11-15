# frames24

**`fish-oot-frames24.bps`** (`fishmod/build.sh --script pcprof_frames --hw --texpack --colpack --meshpack --gcc-all`):
frames23 with:

- **The water fog's seams fixed**: frames23 swapped the scene's fog colour for the water's on the triangles touching the
  water, so far off (where the game's haze takes over) they went to the water's dark colour instead of the haze: the far
  lake a dark band against the hazy shore, the pillars standing in the water dark, a seam where those triangles met
  the dry ones. Now the water fog is in the CPU-lit vertex colours (each blended toward the colour that, times its
  texture's mean, is the water's) and the game's own distance fog stays on over everything: wet and dry haze alike,
  no fog stubs, no colour swap. Only the 351 vertices under the surface are worked (447 before), by the eye's
  position alone (turning the camera costs nothing), half of them a frame while it moves.
- **The Fishing Pond's lily pads and reeds** drawn right again (texpack's palette fix: frames22 and frames23 had them
  garbled).

Compare with frames23 (the water fog's CPU cost on moving frames: it should be about half or less).
