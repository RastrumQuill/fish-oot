// The animation check with level 3 (fishmod/docs/anim-verify.md): every case also drawn from 8 camera angles, each
// picture's frame buffer hashed in the ROM once the RDP has finished it. 8 frames a case: about 8 times as long as
// animcheck (hours in ares): animcheck_pix_1of4 .. _4of4 split the cases between four emulators.
// fishmod/tools/collog.sh --anim-pix saves the log; animcheck_compare.py --pix compares two.
// Build: fishmod/build.sh --script animcheck_pix --hw --colpack --gcc-all
#define FISHMOD_ANIMCHECK_PIX 1
#include "animcheck.inc.c"
