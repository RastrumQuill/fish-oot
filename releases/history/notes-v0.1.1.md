Incremental update to the Kokiri fishing playtest (`kokiri-test1` = 0.1.0).

### Changes
- **No more terrain clipping.** The fishing line drapes over the banks and floats on the water instead of cutting
  through them. Fish know their body size: they turn away from banks before their head goes in, stay between the
  bottom and the surface, are released into water deep enough for them, and don't leap out over dry land.
- **Reeling in at the shore.** A hooked fish can be dragged through the shallows (thrashing at the surface) instead of
  jamming against the bank until the line snaps. Reeling it onto the bank lands it.
- **Much faster.** Fishing used to slow Kokiri Forest to about 12 fps. The ground under the water is now cached
  instead of re-measured every frame, and fish that are off screen aren't drawn, bringing it back to about the game's
  normal 20 fps (measured in ares).

### Install
Apply `fish-oot-v0.1.1.bps` to an **Ocarina of Time NTSC 1.2 (US)** `.z64` ROM (MD5
`57a9719ad547c516342e1a15d5c28c3d`) with [Flips](https://github.com/Alcaro/Flips) or the
[online patcher](https://www.marcrobledo.com/RomPatcher.js/). In Kokiri Forest, press **L** to put the rod on B.
See the [README](https://github.com/RastrumQuill/fish-oot#controls) for controls.
