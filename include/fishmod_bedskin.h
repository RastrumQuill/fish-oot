#ifndef FISHMOD_BEDSKIN_H
#define FISHMOD_BEDSKIN_H

#include "ultra64.h"

/*
 * The bed-skin test (fishmod/build.sh --bedskin, fishmod/tools/gen_bedskin.py): a finer copy of a scene's bed under the
 * water, lying on it (a decal), with the bed's own materials and lighting and its own vertex colours on top (F3DEX3's
 * packed normals: normals in the vertex's flag, colours multiplying the lit result): greener under the weed beds, in
 * patches. Drawn when the camera is under water, instead of moss mats. Every part is a multiple of 8 bytes, so the
 * parts follow each other with no padding: header, materials, chunks, batches, vertices, triangles (3 bytes each).
 */

#define FISHMOD_BED_MAGIC 0x46424544 // 'FBED'

typedef struct FishmodBedHeader {
    /* 0x00 */ u32 magic;
    /* 0x04 */ u16 matCount;
    /* 0x06 */ u16 chunkCount;
    /* 0x08 */ u16 batchCount;
    /* 0x0A */ u16 pad;
    /* 0x0C */ u32 vtxCount;
    /* 0x10 */ u32 triCount;
    /* 0x14 */ u32 pad2[3];
} FishmodBedHeader; // size = 0x20

typedef struct FishmodBedTexLoad {
    /* 0x00 */ u32 segAddr; // in the scene's segment (all RGBA 16-bit); 0: none
    /* 0x04 */ u8 width;
    /* 0x05 */ u8 height;
    /* 0x06 */ u8 cms;
    /* 0x07 */ u8 cmt;
    /* 0x08 */ u8 masks;
    /* 0x09 */ u8 maskt;
    /* 0x0A */ u8 shifts;
    /* 0x0B */ u8 shiftt;
} FishmodBedTexLoad; // size = 0xC

// One of the bed's own materials, as the room draws it: two textures blended by the environment's alpha (the second
// loaded at TMEM 0x100 as tile 1), times the lit shade, and maybe scrolled like the scene's segment 8 or 9 does.
typedef struct FishmodBedMat {
    /* 0x00 */ FishmodBedTexLoad tex0;
    /* 0x0C */ FishmodBedTexLoad tex1;
    /* 0x18 */ u8 envAlpha;
    /* 0x19 */ u8 scroll; // 0, or 8 / 9: the scene's scrolling (Scene_DrawConfigLakeHylia)
    /* 0x1A */ u8 prim[3]; // (one texture: its colour times the shade times this, as the room's own)
    /* 0x1D */ u8 pad[3];
} FishmodBedMat; // size = 0x20

typedef struct FishmodBedChunk {
    /* 0x00 */ s16 x; // its centre, across the ground
    /* 0x02 */ s16 z;
    /* 0x04 */ u16 radius;
    /* 0x06 */ u16 firstBatch;
    /* 0x08 */ u16 batchCount;
    /* 0x0A */ s16 yMin;
    /* 0x0C */ s16 yMax;
    /* 0x0E */ u16 pad;
} FishmodBedChunk; // size = 0x10

typedef struct FishmodBedBatch {
    /* 0x00 */ u16 vtxStart;
    /* 0x02 */ u16 triStart;
    /* 0x04 */ u8 vtxCount; // up to 32, loaded at 0
    /* 0x05 */ u8 triCount;
    /* 0x06 */ u8 mat;
    /* 0x07 */ u8 pad;
} FishmodBedBatch; // size = 0x8

#endif
