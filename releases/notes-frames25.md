# frames25

**`fish-oot-frames25.bps`** (`fishmod/build.sh --script pcprof_frames --hw --texpack --colpack --meshpack --gcc-all`):
frames24 with:

- **The crash fixed** (TLB exception on load at 0x06013EE8): a touched plant whose model lives in an object's segment
  (outside the lake) had its model read at that segment address to cut it into bend bands. Such a plant now leans
  whole.
- **Plants bend as if held up by the water**: most at the foot, the top turning back toward upright (frames23-24 bent
  most at the top), keeping their length, leaning at most 0.6 of their height (was 1.0), springing back quicker.
- **No depth darkening** (bed and props): deep things had gone to black shapes in the haze, and the standing vine, on
  the deep bed, was black up close. The water fog shows depth.
- **The props' chunk test made safe**: a chunk is judged at the height in it nearest the view, not at one height under
  the surface; and a full list of props in view lets go of the farthest instead of whatever was gathered last.
  fishmod/tools/popin_check.py (every prop that should be on screen against the ones drawn, from a RAM dump) found
  nothing missing on the bench route before or after, so this isn't known to be the pop-in seen on the console.

Compare with frames24.
