// LOD calibration: the tour's stops, each held while the game measures how far every prop kind has to be drawn
// (fishmod_debug.c, FISHMOD_LOD_CALIBRATE). The results go to SRAM: fishmod/tools/lod_calibrate.py reads the save
// file and writes include/fishmod_lod.h.
#define FISHMOD_LOD_CALIBRATE 1
#include "tour.inc.c"
