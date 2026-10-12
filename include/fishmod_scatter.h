#ifndef FISHMOD_SCATTER_H
#define FISHMOD_SCATTER_H

#include "ultra64.h"

// A scene's scattered habitat props, placed at build time by fishmod/tools/gen_scatter.py and drawn by the fish
// manager (fishmod_scatter.c). Props are sorted by chunk (a grid over the scene), then by kind.

#define FISHMOD_SCATTER_MAGIC 0x46534331 // "FSC1"

typedef struct FishmodProp {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y; // the bed under it, or the surface for floating props
    /* 0x04 */ s16 z;
    /* 0x06 */ u8 kind;  // FishmodPropKind
    /* 0x07 */ u8 yaw;   // 256 per turn
    /* 0x08 */ u8 scale; // 64 = 1.0
    /* 0x09 */ u8 tint;  // 128 = 1.0, times the vertex colours
} FishmodProp; // size = 0x0A

typedef struct FishmodScatterHeader {
    /* 0x00 */ u32 magic;
    /* 0x04 */ s16 x0; // the chunk grid's corner
    /* 0x06 */ s16 z0;
    /* 0x08 */ s16 chunk; // chunk size
    /* 0x0A */ s16 width; // chunks
    /* 0x0C */ s16 depth;
    /* 0x0E */ u16 count; // props
    /* 0x10 */ u32 chunkStartOffset;   // u16[width * depth + 1]: each chunk's first prop
    /* 0x14 */ u32 chunkSurfaceOffset; // s16[width * depth]: the water level each chunk was placed for
    /* 0x18 */ u32 propsOffset;        // FishmodProp[count]
} FishmodScatterHeader; // size = 0x1C

#endif
