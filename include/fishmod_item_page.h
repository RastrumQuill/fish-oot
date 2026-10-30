#ifndef FISHMOD_ITEM_PAGE_H
#define FISHMOD_ITEM_PAGE_H

#include "ultra64.h"

// The item page's own graphics (fishmod/tools/gen_item_page.py), a ROM file the pause menu loads beside its own:
// the page background redrawn for the 6 x 5 grid, placeholder lure icons, and the rod's and lures' names.

// The page's tiles that the new pipes change: 03, 04, 11, 13, 14, 21, 22, 23, 24 (the rest are the game's own)
#define FISHMOD_ITEM_PAGE_TILES 9
// each tile under the title (column-major 01-04, 11-14, 21-24): its index in pageTex, or -1 for the game's own
#define FISHMOD_ITEM_PAGE_TILE_INDEX { -1, -1, 0, 1, 2, -1, 3, 4, 5, 6, 7, 8 }
#define FISHMOD_LURE_COUNT 3
#define FISHMOD_ITEM_NAMES 4 // the rod, then the lures

typedef struct FishmodItemPage {
    u64 pageTex[FISHMOD_ITEM_PAGE_TILES][80 * 32 / 8];   // IA8 80 x 32
    u64 lureIcons[FISHMOD_LURE_COUNT][32 * 32 * 4 / 8]; // RGBA32 32 x 32
    // (loaded only up to here: the names are read from ROM one at a time, like the game's own)
    u64 names[FISHMOD_ITEM_NAMES][128 * 16 / 2 / 8];    // IA4 128 x 16
} FishmodItemPage;

extern FishmodItemPage gFishmodItemPage;

#endif
