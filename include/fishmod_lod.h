#ifndef FISHMOD_LOD_H
#define FISHMOD_LOD_H

/*
 * How far each habitat prop kind is drawn, above water and with the camera under it (units). Made by
 * fishmod/tools/lod_calibrate.py from the LOD calibration (fishmod/scripts/lod_calibrate.inc.c): the shortest
 * distance at which leaving the kind out further away changes no tour view by more than the threshold, at the N64's
 * own resolution (a threshold per pixel, not an average over the frame). -1: not measured, fishmod_scatter.c's
 * default kept.
 */
#define FISHMOD_LOD_DISTANCES(X)

#endif
