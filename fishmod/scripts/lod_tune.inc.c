// LOD tuning (fishmod_debug.c, FISHMOD_LOD_TUNE): every view of fishmod/tools/gen_lod_views.py --pitches, at four
// times of day, measuring per plant kind (above and under water) the shortest cut, single-card and half-texture
// distances that change no frame. One ROM for every part: the part comes from the save (fishmod/pc/run_jobs.py,
// "sram" in the pack). Results to SRAM 0x0000: fishmod/tools/lod_tune.py merges the parts' saves.
#define FISHMOD_LOD_TUNE 1
#include "lod_sweep.inc.c"
