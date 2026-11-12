// pcprof_frames for a console test: without the reboot handshake's per-frame poll of the cart (reboot it with RESET).
// Build: fishmod/build.sh --script pcprof_frames_noaux --hw --colpack --gcc-all
#define FISHMOD_NO_AUX 1
#include "pcprof_frames.inc.c"
