# frames23

**`fish-oot-frames23.bps`** (`fishmod/build.sh --script pcprof_frames --hw --texpack --colpack --meshpack --gcc-all`):
frames22 (the texture pass, ripples loaded once, plants that bend) with:

- **The terrain anti-aliased without reading the picture back**, in every room: the terrain sweep (console, 18 laps)
  measured it at 2.9 ms of RDP a frame and 8 points fewer hitches, every leg. (The sky drawn last cost 1.0 ms: not in.)
- **Deep water darker**: the lake bed and props darkened by their depth under the surface (fishmod/docs/depth-darkening.png).
- **Water fog from above**: the lake bed fogs toward the water's colour by the light's path through the water, worked
  out by the CPU for the 447 vertices of the triangles touching the water when the camera moves (ares: 1.3-1.7 ms of
  CPU on such frames; the console's frame log says what it really costs). fishmod/docs/water-fog.png.
- **Plants making way**: up to 32 at once, floating ones reached by their size (the spatterdock now moves), the vine
  curtain swings from where it hangs.

Compare with frames22 (texture pass alone) and frames21.
