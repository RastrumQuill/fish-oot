#ifndef FISHMOD_BEDCARD_H
#define FISHMOD_BEDCARD_H

#include "ultra64.h"

/*
 * Bed cards (fishmod/tools/bedcard.py): each scatter chunk's distant card-drawn plants as one card. The ROM file
 * (uncompressed: read in pieces) is the header, the card table, then each card's views (FISHMOD_BEDCARD_VIEWS
 * textures, 45 degrees apart, each a 16-colour palette then 4-bit texels). fishmod_scatter.c keeps the table and streams
 * the views of the cards it draws into a small cache.
 */

#define FISHMOD_BEDCARD_MAGIC 0x46424344 // 'FBCD'
#define FISHMOD_BEDCARD_VIEWS 4
#define FISHMOD_BEDCARD_TEX_W 48
#define FISHMOD_BEDCARD_TEX_H 24
#define FISHMOD_BEDCARD_VIEW_BYTES (32 + FISHMOD_BEDCARD_TEX_W * FISHMOD_BEDCARD_TEX_H / 2)
#define FISHMOD_BEDCARD_BYTES (FISHMOD_BEDCARD_VIEWS * FISHMOD_BEDCARD_VIEW_BYTES)

typedef struct FishmodBedCardHeader {
    /* 0x00 */ u32 magic;
    /* 0x04 */ u32 count;     // cards
    /* 0x08 */ u32 cardBytes; // FISHMOD_BEDCARD_BYTES (each card's views)
    /* 0x0C */ u32 pad;
} FishmodBedCardHeader; // size = 0x10

typedef struct FishmodBedCard {
    /* 0x00 */ s16 chunk;  // the scatter chunk it stands for
    /* 0x02 */ s16 x;      // where it stands (the plants' middle)
    /* 0x04 */ s16 z;
    /* 0x06 */ s16 y;      // their lowest foot
    /* 0x08 */ s16 reach;  // half its width
    /* 0x0A */ s16 height;
} FishmodBedCard; // size = 0x0C

// The scene's bed card file in ROM (fishmod.c), or false
s32 Fishmod_BedCardRom(s16 sceneId, uintptr_t* romStart, uintptr_t* romEnd);

#endif
