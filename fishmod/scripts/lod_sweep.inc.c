// LOD sweep: the calibration over ~1100 viewpoints around Lake Hylia (fishmod_debug.c, FISHMOD_LOD_SWEEP), for draw
// distances per zone. Make the views first: fishmod/tools/gen_lod_views.py. The results go to SRAM 0x0000:
// fishmod/tools/lod_calibrate.py --sweep reads one or more save files (one per part, run side by side).
#ifndef FISHMOD_LOD_PART
#define FISHMOD_LOD_PART 0
#define FISHMOD_LOD_PARTS 1
#endif
#define FISHMOD_LOD_SWEEP 1
#include "lod_calibrate.inc.c"
#include "src/fishmod_data/fishmod_lod_views.h"
