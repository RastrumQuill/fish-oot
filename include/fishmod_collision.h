#ifndef FISHMOD_COLLISION_H
#define FISHMOD_COLLISION_H

#include "ultra64.h"
#include "z_math.h"

// Extra collision for a scene's solid habitat props (rocks), baked by fishmod/tools/gen_collision.py and added to
// the scene's own collision when it loads (Fishmod_AddPropCollision).

#define FISHMOD_COL_MAGIC 0x46434F31 // "FCO1"

typedef struct FishmodColPoly {
    /* 0x00 */ u16 vtx[3];   // into the extra vertices
    /* 0x06 */ Vec3s normal; // unit normal, 0x7FFF = 1.0
    /* 0x0C */ s16 dist;     // -(normal . vertex)
} FishmodColPoly; // size = 0x0E

typedef struct FishmodColHeader {
    /* 0x00 */ u32 magic;
    /* 0x04 */ u16 numVertices;
    /* 0x06 */ u16 numPolygons;
    /* 0x08 */ u16 surfaceType; // the scene's surface type for them (the lake bed's)
    /* 0x0A */ u16 pad;
} FishmodColHeader; // size = 0x0C; then Vec3s[numVertices], then FishmodColPoly[numPolygons]

#endif
