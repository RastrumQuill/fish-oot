#ifndef FISHMOD_WATERMAP_H
#define FISHMOD_WATERMAP_H

#include "ultra64.h"

// A scene's water floor map, baked from its collision by fishmod/tools/gen_watermap.py and loaded with the scene
// (fishmod_water.c). One grid per water box: the floor height at corners `cell` units apart, or
// FISHMOD_WATERMAP_NO_FLOOR.

#define FISHMOD_WATERMAP_MAGIC 0x46574D31 // "FWM1"
#define FISHMOD_WATERMAP_MAX_BOXES 4
#define FISHMOD_WATERMAP_NO_FLOOR -0x8000

typedef struct FishmodWaterMapBox {
    /* 0x00 */ s16 xMin; // the water box it covers (matched against the scene's boxes)
    /* 0x02 */ s16 zMin;
    /* 0x04 */ s16 xLength;
    /* 0x06 */ s16 zLength;
    /* 0x08 */ s16 cell;
    /* 0x0A */ s16 width; // corners
    /* 0x0C */ s16 depth;
    /* 0x10 */ u32 offset; // first corner, in corners from the start of the floors
} FishmodWaterMapBox; // size = 0x14

typedef struct FishmodWaterMapHeader {
    /* 0x00 */ u32 magic;
    /* 0x04 */ s16 numBoxes;
    /* 0x08 */ u32 numCorners;
    /* 0x0C */ FishmodWaterMapBox boxes[FISHMOD_WATERMAP_MAX_BOXES];
} FishmodWaterMapHeader; // size = 0x5C

#endif
