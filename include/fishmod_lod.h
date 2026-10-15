#ifndef FISHMOD_LOD_H
#define FISHMOD_LOD_H

/*
 * How far each habitat prop kind is drawn, above water and with the camera under it (units). Made by
 * fishmod/tools/lod_calibrate.py from the LOD calibration (fishmod/scripts/lod_calibrate.inc.c): the shortest
 * distance at which leaving the kind out further away changes no tour view by more than the threshold, at the N64's
 * own resolution (a threshold per pixel, not an average over the frame). -1: not measured, fishmod_scatter.c's
 * default kept.
 */
#define FISHMOD_LOD_DISTANCES(X) \
    X(GRASS_TUFT, 2400, 600) \
    X(RIBBON_WEED, 2800, 700) \
    X(TALL_WEED, 1900, 600) \
    X(BROADLEAF, 2600, 700) \
    X(BUSH_WEED, 3000, 700) \
    X(REED_CLUMP, 3000, 600) \
    X(CATTAILS, 2900, 300) \
    X(OLD_POSTS, 800, 300) \
    X(FLOAT_LEAVES, 1800, 300) \
    X(RUSH_TUFT, 3000, 300) \
    X(IRIS, 300, 300) \
    X(BIG_PONDWEED, 2500, 600) \
    X(SPATTERDOCK, 600, 300) \
    X(KELP_FROND, 2200, 600) \
    X(VINE_STRAND, 2100, 500) \
    X(VINE_CURTAIN, 1000, 300) \
    X(BANK_ROOTS, 300, -1) \
    X(MARIGOLD, 2000, 300) \
    X(FAN_WEED, 300, 600) \
    X(LILY_FLOWER, 2700, 400) \
    X(LILY_CENTRE, 300, 300) \
    X(POND_REED, 3000, 500) \
    X(LILY_PAD, 2900, 400) \
    X(ROCK, 2600, 700) \

#endif
