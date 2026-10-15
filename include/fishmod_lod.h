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
    X(GRASS_TUFT, 3000, 1500) \
    X(RIBBON_WEED, 3000, 1200) \
    X(TALL_WEED, 3000, 800) \
    X(BROADLEAF, 3000, 1400) \
    X(BUSH_WEED, 3000, 1200) \
    X(REED_CLUMP, 3000, 800) \
    X(CATTAILS, 3000, 700) \
    X(OLD_POSTS, 3000, 500) \
    X(FLOAT_LEAVES, 2900, 500) \
    X(BRANCH, 2700, 500) \
    X(RUSH_TUFT, 3000, 500) \
    X(IRIS, 3000, 500) \
    X(BIG_PONDWEED, 3000, 1100) \
    X(SPATTERDOCK, 2900, 500) \
    X(KELP_FROND, 3000, 1800) \
    X(VINE_STRAND, 3000, 1200) \
    X(VINE_CURTAIN, 3000, 600) \
    X(BANK_ROOTS, 1500, 500) \
    X(MARIGOLD, 3000, 500) \
    X(FAN_WEED, 2800, 700) \
    X(LILY_FLOWER, 3000, 700) \
    X(LILY_CENTRE, 500, 500) \
    X(POND_REED, 3000, 1300) \
    X(LILY_PAD, 3000, 600) \
    X(ROCK, 3000, 1200) \

#endif
