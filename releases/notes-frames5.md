# frames5: invisible fixes

**`fish-oot-frames5.bps`** (`fishmod/build.sh --script pcprof_frames --hw --colpack --gcc-all`): frames4-gcc plus
everything below. Apply to the same ROM as the other frames patches. Run it the same way as frames4-gcc (the same
route, the frame log over the SummerCart64's USB).

Every change gives the same results as before:

| change | where | how it was checked |
|---|---|---|
| Prop commands kept in place between frames while nothing they depend on changes; sway matrices' fixed entries written once | prop drawing | a verify build remakes and compares every frame: 0 differences over 3,040 frames |
| Actor distance, height and yaw to Link computed in place | `Actor_UpdateAll` | collision log, 2,488 frames alike |
| Debug timers read the CPU's count register instead of `osGetTime` | bench builds only | the same differences |
| Effect soft sprites: update and draw loops stop at the highest slot in use | water effects, dust, bubbles | collision log; a run-time check found no live effect above the mark |
| Water ripples: their matrix written directly instead of two matrices and a 64-multiply product | ripples | compared at run time against the old product: 18,337 alike |
| SkinMatrix translate-rotate(-scale) without the matrix products | many actors | `fishmod/tools/skin_check.c`: 10 million cases, 0 different |
| `Matrix_TranslateRotateZYX` skips a zero z rotation (as it already did for y and x) | every skeleton | collision log |
| Z-target search tests the attention flag first | every frame, every actor | collision log |
| Fish steering takes the heading's sin and cos once | every fish | collision log |
| The scene's static wall answers kept for questions asked again exactly (like the floor's) | walls | collision log, 2,644 frames alike |
| Bed cards' view angles: sin and cos worked out once | prop drawing | the same calls, the same values |
| Audio and libultra built with `-fno-math-errno` | audio | audio check, 8,253 buffers alike |

The new graphics log (`collog.sh` with a fifth argument) hashes every display list and allocation each frame. Two runs
of one build agree on every frame. Between builds, every matrix, light and vertex block matches, and so do the opaque
and overlay lists. The translucent list differs on some frames only in the order of one effect trail's segments: its
ring buffer is two frames out of phase. That follows the length of the scene load, which changes with the code's
speed. It isn't a change in how anything draws.

ares, the same frames as the frames4-gcc ares run (up to frame 4,915, frames that sent a packet left out):

| ms a frame | frames4-gcc | frames5 |
|---|---|---|
| frame | 56.2 | 53.4 |
| CPU update | 28.4 | 25.4 |
| CPU draw | 24.3 | 22.9 |
| fish update | 5.7 | 4.7 |
| fish draw | 7.6 | 7.3 |
| props draw | 4.7 | 4.5 |
| fps | 17.8 | 18.7 |
