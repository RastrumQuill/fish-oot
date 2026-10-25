#ifndef FISHMOD_BEDSKIN_H
#define FISHMOD_BEDSKIN_H

#include "ultra64.h"

/*
 * The bed-skin test (fishmod/build.sh --bedskin, fishmod/tools/gen_bedskin.py): a finer copy of a scene's bed under the
 * water, lying on it (a decal), with its own vertex colours (greener under the weed beds, in patches), drawn near the
 * camera when it's under water, instead of moss mats. Every part is a multiple of 8 bytes, so the parts follow each
 * other with no padding: header, textures, chunks, batches, vertices, triangles (3 bytes each).
 */

#define FISHMOD_BED_MAGIC 0x46424544 // 'FBED'

typedef struct FishmodBedHeader {
    /* 0x00 */ u32 magic;
    /* 0x04 */ u16 texCount;
    /* 0x06 */ u16 chunkCount;
    /* 0x08 */ u16 batchCount;
    /* 0x0A */ u16 pad;
    /* 0x0C */ u32 vtxCount;
    /* 0x10 */ u32 triCount;
    /* 0x14 */ u32 pad2[3];
} FishmodBedHeader; // size = 0x20

typedef struct FishmodBedTex {
    /* 0x00 */ u32 segAddr; // in the scene's segment (all RGBA 16-bit)
    /* 0x04 */ u8 width;
    /* 0x05 */ u8 height;
    /* 0x06 */ u8 cms;
    /* 0x07 */ u8 cmt;
    /* 0x08 */ u8 masks;
    /* 0x09 */ u8 maskt;
    /* 0x0A */ u8 shifts;
    /* 0x0B */ u8 shiftt;
    /* 0x0C */ u32 pad;
} FishmodBedTex; // size = 0x10

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
    /* 0x06 */ u8 tex;
    /* 0x07 */ u8 pad;
} FishmodBedBatch; // size = 0x8

#endif
