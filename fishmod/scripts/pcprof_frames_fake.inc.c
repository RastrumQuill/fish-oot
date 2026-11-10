#define FISHMOD_FRAME_TAGS 1 // (bisect1: as frames1)
// A test of the frame-log build in an emulator (no SummerCart64 there): as pcprof_frames.inc.c, the USB's transfers
// going nowhere (FISHMOD_FAKE_SC64), so everything else on that path runs.
// Build: fishmod/build.sh --script pcprof_frames_fake --hw
#define FISHMOD_FAKE_SC64 1
#include "pcprof_frames.inc.c"
