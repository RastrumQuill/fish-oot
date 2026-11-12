# rdpbench1: the RDP calibration bench

**`fish-oot-rdpbench1.bps`** (`fishmod/build.sh --script rdpbench --hw --gcc-all`): the console's RDP measured on
controlled work, so an RDP cost model can be fitted and ares's RDP timing made to follow it. ares's own RDP timing is
about three times too fast, and ares has none of the RDP's counters, so only the console can supply this.

It boots to Lake Hylia. Then the game's own drawing switches off, and 46 tests take turns, 30 frames each, three times
round (about five minutes):
- full-screen fill rectangles;
- texture rectangles, full screen four deep, in each texture format (RGBA16 point and bilinear, CI8, I4, RGBA32),
  2-cycle, translucent, with and without Z;
- the same pixels in more and smaller rectangles (the cost of each rectangle);
- grids of 2, 8, 32 and 128-pixel quads in six modes (shade, shade with Z, textured with Z, 2-cycle with fog and
  anti-aliasing, anti-aliased, translucent);
- texture loads before each rectangle;
- syncs and render-mode switches.

Each frame's RDP time and its four counters (clock, command buffer, pipeline, TMEM) go to the frame log over USB. The
morning runner runs it first. Read it with `fishmod/tools/rdpbench.py FOLDER`.

Checked in ares: every test runs and draws what it should (F3DEX3's counters show every triangle and rectangle), and
the run ends with FISHMOD-DONE. In ares the RDP's time just follows the RSP's and its counters are all zero. That is
the gap the console run fills.
