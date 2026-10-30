/*
 * The rod's own graphics (its segments, the line, the hook), built into the overlay from the game's object_fish
 * pieces at build time, so the rod draws in any scene without loading that 87 KB object (only scenes with fish load
 * it, for the fish). The pieces are the extracted assets themselves, renamed.
 */
#include "ultra64.h"
#include "gfx.h"
#include "assets/objects/object_fish/object_fish.h"

#define gFishingRodSegmentStripTex gFishmodRodSegmentStripTex
#define gFishingRodSegmentBlackTex gFishmodRodSegmentBlackTex
#define gFishingRodSegmentWhiteTex gFishmodRodSegmentWhiteTex
#define gFishingRodSegmentVtx sFishmodRodSegmentVtx
#define gFishingRodMaterialDL gFishmodRodMaterialDL
#define gFishingRodSegmentDL gFishmodRodSegmentDL
#define gFishingLineModelVtx sFishmodLineModelVtx
#define gFishingLineModelDL gFishmodLineModelDL
#define gFishingLureHookTex sFishmodLureHookTex
#define gFishingLureHookVtx sFishmodLureHookVtx
#define gFishingLureHookDL gFishmodLureHookDL

u64 gFishingRodSegmentStripTex[TEX_LEN(u64, gFishingRodSegmentStripTex_WIDTH, gFishingRodSegmentStripTex_HEIGHT, 16)] = {
#include "assets/objects/object_fish/gFishingRodSegmentStripTex.rgba16.inc.c"
};

u64 gFishingRodSegmentBlackTex[TEX_LEN(u64, gFishingRodSegmentBlackTex_WIDTH, gFishingRodSegmentBlackTex_HEIGHT, 16)] = {
#include "assets/objects/object_fish/gFishingRodSegmentBlackTex.rgba16.inc.c"
};

u64 gFishingRodSegmentWhiteTex[TEX_LEN(u64, gFishingRodSegmentWhiteTex_WIDTH, gFishingRodSegmentWhiteTex_HEIGHT, 16)] = {
#include "assets/objects/object_fish/gFishingRodSegmentWhiteTex.rgba16.inc.c"
};

static Vtx gFishingRodSegmentVtx[] = {
#include "assets/objects/object_fish/gFishingRodSegmentVtx.inc.c"
};

Gfx gFishingRodMaterialDL[] = {
#include "assets/objects/object_fish/gFishingRodMaterialDL.inc.c"
};

Gfx gFishingRodSegmentDL[] = {
#include "assets/objects/object_fish/gFishingRodSegmentDL.inc.c"
};

static Vtx gFishingLineModelVtx[] = {
#include "assets/objects/object_fish/gFishingLineModelVtx.inc.c"
};

Gfx gFishingLineModelDL[] = {
#include "assets/objects/object_fish/gFishingLineModelDL.inc.c"
};

static u64 gFishingLureHookTex[TEX_LEN(u64, gFishingLureHookTex_WIDTH, gFishingLureHookTex_HEIGHT, 16)] = {
#include "assets/objects/object_fish/gFishingLureHookTex.rgba16.inc.c"
};

static Vtx gFishingLureHookVtx[] = {
#include "assets/objects/object_fish/gFishingLureHookVtx.inc.c"
};

Gfx gFishingLureHookDL[] = {
#include "assets/objects/object_fish/gFishingLureHookDL.inc.c"
};
