# frames22

**`fish-oot-frames22.bps`** (`fishmod/build.sh --script pcprof_frames --hw --texpack --colpack --meshpack --gcc-all`):
frames21 with the texture pass on (game-wide; off since frames3, when the RDP wasn't the limit) and less loading:

- **tools/texpack.py**, every texture it can rewrite in every scene and object, each as small as it goes with every
  pixel drawn the same (checked: each decodes back to its original texels; ares: dock_tour against the same tree
  without it, 0-46 of 76,800 pixels apart, the debug text and dither): 1,500 of 8,857 textures smaller, 1,516 KB less
  to load. 16-bit with <= 256 colours to 8-bit indexed (1,384), <= 16 colours to 4-bit indexed (35: one palette, or
  a 16-colour slot each), intensity-alpha whose alpha is its intensity to 8-bit intensity (25), exact narrower
  intensity formats (3), halved where they repeat (22) or mirror (51) with the tile's wrap or mirror, 91 repeated
  loads removed, 3,413 empty pipe syncs.
- **Ripples** drawn one after another load their texture once (z_eff_ss_g_ripple.c): a frame of Link swimming, 9
  loads (36 KB) to 1.
- **Plants bend**: a touched plant's vertices loaded in four bands up its height, each under more of the lean, so the
  stalk curves instead of tipping; it springs back past upright and settles (bend_test script).

Compare with frames21 (same route).
