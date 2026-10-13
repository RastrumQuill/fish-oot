#ifndef FISHMOD_COLLISION_H
#define FISHMOD_COLLISION_H

#include "ultra64.h"
#include "z_math.h"

// Extra collision for a scene's solid habitat props (rocks), baked by fishmod/tools/gen_collision.py and added to
// the scene's own collision when it loads (Fishmod_AddPropCollision).

#define FISHMOD_COL_MAGIC 0x46434F31 // "FCO1"

typedef struct FishmodColPoly {
    /* 0x00 */ u16 type;     // the scene's surface type (the lake bed's for rocks, the bridges' wood for the dock)
    /* 0x02 */ u16 vtx[3];   // into the extra vertices
    /* 0x08 */ Vec3s normal; // unit normal, 0x7FFF = 1.0
    /* 0x0E */ s16 dist;     // -(normal . vertex)
} FishmodColPoly; // size = 0x10

typedef struct FishmodColHeader {
    /* 0x00 */ u32 magic;
    /* 0x04 */ u16 numVertices;
    /* 0x06 */ u16 numPolygons;
} FishmodColHeader; // size = 0x08; then Vec3s[numVertices], then FishmodColPoly[numPolygons]

#endif
