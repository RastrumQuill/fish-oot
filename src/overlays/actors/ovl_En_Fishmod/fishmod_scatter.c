/*
 * File: fishmod_scatter.c
 * Overlay: ovl_En_Fishmod
 * Description: the scene's habitat props (FISHMOD_OPT 5): weed beds, reeds, lily pads, rocks and wood, placed at build
 * time across the water by fishmod/tools/gen_scatter.py (shore-to-deep banding, the sub-biome zones, patchy density)
 * and drawn by the fish manager.
 *
 * Only the chunks near the camera are looked at, each kind's props are drawn together (its material once), and each
 * kind has a draw distance (small tufts close by, reeds, lilies and rocks from further away). A prop is a matrix,
 * a colour and its kind's display list: no per-vertex work.
 */

#include "z_en_fishmod.h"
#include "fishmod.h"
#include "fishmod_scatter.h"

#include "gfx.h"
#include "gfx_setupdl.h"
#include "play_state.h"
#include "sys_matrix.h"
#include "z_lib.h"
#include "array_count.h"
#include "bgcheck.h"
#include "save.h"
#include "libc/math.h"
#include "ultra64.h"
#include "zelda_arena.h"
#include "rand.h"
#include "ultra64/libc.h"
#include "assets/objects/object_fish/object_fish.h"

#if FISHMOD_OPT >= 5

// the props' textures and display lists (fishmod/tools/gen_habitat.py -> fishmod/tools/assets.py, made at build time)
#include "src/fishmod_data/fishmod_props.inc.c"
#include "src/fishmod_data/fishmod_props_kinds.h"

typedef enum FishmodPropKind {
#define X(id, name) FISHMOD_PROP_##id,
    FISHMOD_PROP_KINDS(X)
#undef X
    FISHMOD_PROP_KIND_MAX
} FishmodPropKind;

typedef struct FishmodPropKindInfo {
    Gfx* material;
    Gfx* model;
    f32 drawDist;
    u8 floating; // on the surface: not drawn while the water there is lower than it was placed for
    u8 plant;    // a living plant: not drawn where the water has gone (the drained lake)
    u8 xlu;      // drawn see-through (the Fishing Pond's reed, as vanilla draws it)
    u8 sway;     // how far its top sways (per mille of its height per unit of height); 0: still
    u8 swayRate; // how fast (a turn takes 0x10000 / (swayRate * 16) frames)
} FishmodPropKindInfo;

#define DIST_SMALL 800.0f
#define DIST_MID 1200.0f
#define DIST_FAR 1600.0f
// Props drawn per frame at most. They are drawn nearest band first (whatever their kind), so the budget runs out on
// the far ones. Each is a matrix and three commands, in buffers of the props' own (allocated with the scene's props).
#define PROP_BUDGET 900
#define BAND_NEAR 550.0f
#define BAND_MID 1100.0f
#define BANDS 3
// Props in view per band at most (before the budget)
#define BAND_MAX 1200
// The view's half-angle: the screen's corner is about 44 degrees off the look direction (60 degree field of view,
// 4:3); a little wider, plus the props' size as room. Props outside it (behind, too far left or right, above or
// below) aren't drawn, so the budget goes on what's on screen.
#define VIEW_COS 0.68f
#define PROP_RADIUS 90.0f
// See-through props (the pond reeds) drawn per frame at most
#define XLU_MAX 200

static FishmodPropKindInfo sKinds[FISHMOD_PROP_KIND_MAX];

static void Scatter_InitKinds(void) {
#define X(id, name)                                                       \
    sKinds[FISHMOD_PROP_##id].material = gFishmodProp_##name##DLMaterial; \
    sKinds[FISHMOD_PROP_##id].model = gFishmodProp_##name##DL;
    // the generated kinds (the vanilla ones are set below)
    FISHMOD_PROP_KINDS_GENERATED(X)
#undef X
    sKinds[FISHMOD_PROP_POND_REED].material = gFishingReedMaterialDL;
    sKinds[FISHMOD_PROP_POND_REED].model = gFishingReedModelDL;
    sKinds[FISHMOD_PROP_LILY_PAD].material = gFishingLilyPadMaterialDL;
    sKinds[FISHMOD_PROP_LILY_PAD].model = gFishingLilyPadModelDL;
    sKinds[FISHMOD_PROP_ROCK].material = gFishingRockMaterialDL;
    sKinds[FISHMOD_PROP_ROCK].model = gFishingRockModelDL;
    {
        s32 i;

        for (i = 0; i < FISHMOD_PROP_KIND_MAX; i++) {
            sKinds[i].drawDist = DIST_FAR;
            sKinds[i].plant = true;
        }
    }
    sKinds[FISHMOD_PROP_POND_REED].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_POND_REED].xlu = true;
    sKinds[FISHMOD_PROP_GRASS_TUFT].drawDist = DIST_SMALL;
    sKinds[FISHMOD_PROP_LEAF_LITTER].drawDist = DIST_SMALL;
    sKinds[FISHMOD_PROP_PEBBLE_PATCH].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_RIBBON_WEED].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_BROADLEAF].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_BUSH_WEED].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_BRANCH].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_LILY_FLOWER].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_LILY_CENTRE].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_RUSH_TUFT].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_MARIGOLD].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_VINE_STRAND].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_BIG_PONDWEED].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_SPATTERDOCK].floating = true;
    sKinds[FISHMOD_PROP_VINE_CURTAIN].floating = true;
    sKinds[FISHMOD_PROP_LILY_PAD].floating = true;
    sKinds[FISHMOD_PROP_FLOAT_LEAVES].floating = true;
    sKinds[FISHMOD_PROP_LILY_FLOWER].floating = true;
    sKinds[FISHMOD_PROP_LILY_CENTRE].floating = true;
    // swaying: weed slowly with the water's movement, reeds above it faster in the wind. The shear moves a point by
    // its height, so tall kinds get less of it: every tip moves about the same few units.
#define SWAY(id, amount, rate)                \
    sKinds[FISHMOD_PROP_##id].sway = (amount); \
    sKinds[FISHMOD_PROP_##id].swayRate = (rate)
    SWAY(RIBBON_WEED, 70, 44);
    SWAY(TALL_WEED, 32, 36);
    SWAY(KELP_FROND, 30, 32);
    SWAY(VINE_STRAND, 40, 40);
    SWAY(BIG_PONDWEED, 60, 40);
    SWAY(FAN_WEED, 70, 44);
    SWAY(BROADLEAF, 50, 48);
    SWAY(BUSH_WEED, 40, 44);
    SWAY(GRASS_TUFT, 50, 52);
    SWAY(REED_CLUMP, 20, 80);
    SWAY(CATTAILS, 18, 80);
    SWAY(RUSH_TUFT, 45, 84);
    SWAY(IRIS, 35, 76);
    SWAY(POND_REED, 18, 72);
#undef SWAY
    sKinds[FISHMOD_PROP_DOCK_DECK].plant = false;
    sKinds[FISHMOD_PROP_DOCK_POSTS].plant = false;
    sKinds[FISHMOD_PROP_ROCK].plant = false;
    sKinds[FISHMOD_PROP_PEBBLE_PATCH].plant = false;
    sKinds[FISHMOD_PROP_SUNKEN_LOG].plant = false;
    sKinds[FISHMOD_PROP_BRANCH].plant = false;
    sKinds[FISHMOD_PROP_OLD_POSTS].plant = false;
}

static FishmodScatterHeader* sScatter;
static u16* sChunkStart;
static s16* sChunkSurface;
static FishmodProp* sProps;
// Per chunk: the water there is lower than the props were placed for (the drained lake). Placement is all done at
// build time; the one thing only the game knows is which water level this visit has (vanilla picks it from the save:
// adult before the Water Temple is cleared). So it is worked out once, on the first frame drawn (the lake's own actor
// sets the level during the scene's first update), and again only if that save flag changes.
static u8* sChunkDry;
static s8 sDryFor = -1; // the flag the table was made for; -1: not made yet

// Two frames' worth of matrices and commands (the RSP reads last frame's while this one is built), in buffers of
// their own rather than the frame's display buffer, which the scene and the fish need.
// Sway: each swaying kind has a few phases (props pick one by where they stand, so neighbours move together, like a
// wave through a bed), each a small shear applied to the prop's own shape before it is placed.
#define SWAY_PHASES 4
#define SWAY_MAX (FISHMOD_PROP_KIND_MAX * SWAY_PHASES)

// The props' commands, two frames' worth (the RSP reads last frame's while this one is built), in buffers of their
// own rather than the frame's display buffer, which the scene and the fish need.
typedef struct ScatterFrame {
    Gfx opa[PROP_BUDGET * 4 + FISHMOD_PROP_KIND_MAX * BANDS * 16 + 4];
    Gfx xlu[XLU_MAX * 3 + BANDS * 16 + 4];
    Mtx sway[SWAY_MAX];
} ScatterFrame;
// What's drawn and the props' matrices: made when the camera has moved, and kept while it holds still (fishing,
// mostly), so most frames only send the commands again. Two sets: a new one is made while the RSP may still be
// reading the last.
typedef struct ScatterSet {
    Mtx mtx[PROP_BUDGET];
    u16 prop[PROP_BUDGET]; // in drawing order: nearest band first, kind by kind within a band
    s32 count;
} ScatterSet;
typedef struct ScatterBuffers {
    ScatterFrame frame[2];
    ScatterSet set[2];
} ScatterBuffers;
static ScatterBuffers* sBufs;
static s32 sFrame;
static s32 sSet;
static s32 sMtxUsed;
// the camera the current set was made for
static Vec3f sSetEye;
static Vec3f sSetLook;
static s32 sSetAge = 1000;
// Remake the set when the camera has moved this far or turned this much, or after this many frames anyway
#define REUSE_MOVE 6.0f
#define REUSE_TURN_COS 0.9996f
#define REUSE_FRAMES 10

static void Scatter_AllocBuffers(void) {
    if (sBufs == NULL) {
        sBufs = ZeldaArena_Malloc(sizeof(ScatterBuffers));
    }
    sSetAge = 1000;
}

static void Scatter_FreeBuffers(void) {
    if (sBufs != NULL) {
        ZeldaArena_Free(sBufs);
        sBufs = NULL;
    }
}

void EnFishmod_ScatterLoad(PlayState* play) {
    EnFishmod_ScatterUnload();
    sScatter = Fishmod_LoadScatter(play->sceneId);
    if (sScatter == NULL) {
        return;
    }
    sChunkStart = (u16*)((u8*)sScatter + sScatter->chunkStartOffset);
    sChunkSurface = (s16*)((u8*)sScatter + sScatter->chunkSurfaceOffset);
    sProps = (FishmodProp*)((u8*)sScatter + sScatter->propsOffset);
    sChunkDry = ZeldaArena_Malloc(sScatter->width * sScatter->depth);
    sDryFor = -1;
    Scatter_AllocBuffers();
#if FISHMOD_DEBUG
    {
        u32 maxFree;
        u32 free;
        u32 alloc;

        ZeldaArena_GetSizes(&maxFree, &free, &alloc);
        Fishmod_DebugPrintf(10, "props %d bufs %s arena free %dK (max %dK)", sScatter->count,
                            (sBufs != NULL) ? "ok" : "NONE", (s32)(free >> 10), (s32)(maxFree >> 10));
    }
#endif
    Scatter_InitKinds();
}

/**
 * A random piece of cover (a weed bed, rock, reeds, a lily pad...) for a fish's home: fish live by cover, not spread
 * evenly over open water. False if the scene has no props.
 */
s32 EnFishmod_ScatterRandomCover(Vec3f* out) {
    s32 tries;

    if ((sScatter == NULL) || (sScatter->count == 0)) {
        return false;
    }
    for (tries = 0; tries < 8; tries++) {
        FishmodProp* prop = &sProps[(s32)Rand_ZeroFloat(sScatter->count - 0.01f)];

        if ((prop->kind == FISHMOD_PROP_LILY_FLOWER) || (prop->kind == FISHMOD_PROP_LILY_CENTRE)) {
            continue;
        }
        out->x = prop->x;
        out->y = prop->y;
        out->z = prop->z;
        return true;
    }
    return false;
}

/**
 * A plant being drawn this frame within `range` of the camera (where bubbles come from), at its base. False if none
 * was found in a few tries.
 */
s32 EnFishmod_ScatterRandomDrawnPlant(Vec3f* eye, f32 range, Vec3f* out) {
    ScatterSet* set;
    s32 tries;

    if ((sBufs == NULL) || (sScatter == NULL)) {
        return false;
    }
    set = &sBufs->set[sSet];
    if (set->count == 0) {
        return false;
    }
    for (tries = 0; tries < 6; tries++) {
        FishmodProp* prop = &sProps[set->prop[(s32)Rand_ZeroFloat(set->count - 0.01f)]];
        FishmodPropKindInfo* info = &sKinds[prop->kind];

        if (!info->plant || info->floating || info->xlu) {
            continue;
        }
        if ((SQ(prop->x - eye->x) + SQ(prop->z - eye->z)) > SQ(range)) {
            continue;
        }
        out->x = prop->x;
        out->y = prop->y;
        out->z = prop->z;
        return true;
    }
    return false;
}

void EnFishmod_ScatterUnload(void) {
    if (sScatter != NULL) {
        ZeldaArena_Free(sScatter);
        sScatter = NULL;
    }
    if (sChunkDry != NULL) {
        ZeldaArena_Free(sChunkDry);
        sChunkDry = NULL;
    }
    Scatter_FreeBuffers();
}

/** One s15.16 entry of a fixed-point Mtx (row r, column c). */
#define MTX_SET(mtx, r, c, fixed)                         \
    {                                                     \
        s32 f_ = (fixed);                                 \
        (mtx)->intPart[r][c] = (u32)f_ >> 16;             \
        (mtx)->fracPart[r][c] = f_ & 0xFFFF;              \
    }                                                     \
    (void)0

/**
 * A prop's matrix: turned about Y, scaled, placed. Written straight into the fixed-point Mtx (guMtxF2L converts all
 * sixteen entries from floats; here only the three that aren't 0, 1 or a whole-number position need it).
 */
static Mtx* Scatter_Matrix(FishmodProp* prop, s32 flatScale) {
    Mtx* mtx = &sBufs->set[sSet].mtx[sMtxUsed++];
    s16 yaw = prop->yaw << 8;
    f32 s = prop->scale * (65536.0f / 64.0f);
    s32 c = (s32)(Math_CosS(yaw) * s);
    s32 sn = (s32)(Math_SinS(yaw) * s);

    // rows are the images of the x, y and z axes; the last row is the position
    MTX_SET(mtx, 0, 0, c);
    MTX_SET(mtx, 0, 1, 0);
    MTX_SET(mtx, 0, 2, -sn);
    MTX_SET(mtx, 0, 3, 0);
    MTX_SET(mtx, 1, 0, 0);
    MTX_SET(mtx, 1, 1, flatScale ? 0x10000 : (s32)s);
    MTX_SET(mtx, 1, 2, 0);
    MTX_SET(mtx, 1, 3, 0);
    MTX_SET(mtx, 2, 0, sn);
    MTX_SET(mtx, 2, 1, 0);
    MTX_SET(mtx, 2, 2, c);
    MTX_SET(mtx, 2, 3, 0);
    mtx->intPart[3][0] = prop->x;
    mtx->intPart[3][1] = prop->y;
    mtx->intPart[3][2] = prop->z;
    mtx->intPart[3][3] = 1;
    mtx->fracPart[3][0] = 0;
    mtx->fracPart[3][1] = 0;
    mtx->fracPart[3][2] = 0;
    mtx->fracPart[3][3] = 0;
    return mtx;
}

static void Scatter_UpdateDry(PlayState* play) {
    s8 flag = GET_EVENTCHKINF(EVENTCHKINF_RESTORED_LAKE_HYLIA) ? 1 : 0;
    s32 c;

    if (flag == sDryFor) {
        return;
    }
    sDryFor = flag;
    for (c = 0; c < sScatter->width * sScatter->depth; c++) {
        WaterBox* box;
        f32 surface;

        sChunkDry[c] = false;
        if ((sChunkSurface[c] != -0x8000) && (sChunkStart[c] != sChunkStart[c + 1])) {
            f32 mx = sScatter->x0 + ((c % sScatter->width) + 0.5f) * sScatter->chunk;
            f32 mz = sScatter->z0 + ((c / sScatter->width) + 0.5f) * sScatter->chunk;

            if (!BgCheck_GetWaterSurface(play, &play->colCtx, mx, mz, &surface, &box) ||
                (surface < sChunkSurface[c] - 20.0f)) {
                sChunkDry[c] = true;
            }
        }
    }
}

/** A kind's material, at the start of its props in a band. */
static Gfx* Scatter_Material(Gfx* gfx, FishmodPropKindInfo* info, s32 kind) {
    gfx = Gfx_SetupDL(gfx, SETUPDL_25);
    if (info->xlu || (kind == FISHMOD_PROP_LILY_PAD) || (kind == FISHMOD_PROP_ROCK)) {
        // vanilla's own materials (lit by the scene's lights, like the Fishing Pond's props)
        gSPDisplayList(gfx++, info->material);
        return gfx;
    }
    gDPPipeSync(gfx++);
    gDPSetTextureLUT(gfx++, G_TT_NONE);
    gDPSetCombineLERP(gfx++, TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0, COMBINED, 0, PRIMITIVE, 0, 0, 0, 0, COMBINED);
    gDPSetRenderMode(gfx++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_TEX_EDGE2);
    gSPClearGeometryMode(gfx++, G_CULL_BACK | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gSPSetGeometryMode(gfx++, G_FOG | G_SHADE | G_SHADING_SMOOTH);
    gSPDisplayList(gfx++, info->material);
    return gfx;
}

/** The sway shears for this frame: per swaying kind, a few phases. */
static void Scatter_Sway(PlayState* play, ScatterFrame* frame) {
    s32 kind;
    s32 ph;
    s32 t = play->gameplayFrames;

    for (kind = 0; kind < FISHMOD_PROP_KIND_MAX; kind++) {
        FishmodPropKindInfo* info = &sKinds[kind];

        if (info->sway == 0) {
            continue;
        }
        for (ph = 0; ph < SWAY_PHASES; ph++) {
            Mtx* mtx = &frame->sway[kind * SWAY_PHASES + ph];
            s16 ang = (s16)(t * info->swayRate * 16 + ph * (0x10000 / SWAY_PHASES) + kind * 0x2345);
            f32 amp = info->sway * (65536.0f / 1000.0f);
            s32 a = (s32)(Math_SinS(ang) * amp);
            s32 b = (s32)(Math_CosS((s16)(ang * 7 / 10 + 0x1000)) * amp * 0.4f);

            // identity, but the y axis leans: x += a * y, z += b * y
            MTX_SET(mtx, 0, 0, 0x10000);
            MTX_SET(mtx, 0, 1, 0);
            MTX_SET(mtx, 0, 2, 0);
            MTX_SET(mtx, 0, 3, 0);
            MTX_SET(mtx, 1, 0, a);
            MTX_SET(mtx, 1, 1, 0x10000);
            MTX_SET(mtx, 1, 2, b);
            MTX_SET(mtx, 1, 3, 0);
            MTX_SET(mtx, 2, 0, 0);
            MTX_SET(mtx, 2, 1, 0);
            MTX_SET(mtx, 2, 2, 0x10000);
            MTX_SET(mtx, 2, 3, 0);
            MTX_SET(mtx, 3, 0, 0);
            MTX_SET(mtx, 3, 1, 0);
            MTX_SET(mtx, 3, 2, 0);
            MTX_SET(mtx, 3, 3, 0x10000);
        }
    }
}

/**
 * Which props to draw, nearest band first, and their matrices, into the other set: one pass over the props near
 * the camera, keeping those in view (a cone around the look direction, any pitch, per chunk then per prop) and
 * within their kind's distance.
 */
static void Scatter_Build(PlayState* play, Vec3f* eyeP, f32 lookX, f32 lookY, f32 lookZ) {
    static u16 sVisible[128];                       // chunks near the camera
    static f32 sChunkDistSq[ARRAY_COUNT(sVisible)]; // their centres' distance from the eye, squared
    static u16 sBand[BANDS][BAND_MAX];              // props in view, by distance band (kind by kind within a band)
    static u16 sKindStart[ARRAY_COUNT(sVisible)][FISHMOD_PROP_KIND_MAX + 1]; // each kind's first prop in the chunk
    Vec3f eye = *eyeP;
    s32 bandCount[BANDS];
    s32 visibleCount = 0;
    s32 chunk = sScatter->chunk;
    f32 distScale = 1.0f;
    f32 distFar = DIST_FAR * distScale;
    s32 cx0;
    s32 cx1;
    s32 cz0;
    s32 cz1;
    s32 cx;
    s32 cz;
    s32 kind;
    s32 band;
    s32 v;
    s32 budget = PROP_BUDGET;
    s32 xluLeft = XLU_MAX;
    ScatterSet* set;

    // the chunks within reach of the camera
    cx0 = CLAMP((s32)((eye.x - distFar - sScatter->x0) / chunk), 0, sScatter->width - 1);
    cx1 = CLAMP((s32)((eye.x + distFar - sScatter->x0) / chunk), 0, sScatter->width - 1);
    cz0 = CLAMP((s32)((eye.z - distFar - sScatter->z0) / chunk), 0, sScatter->depth - 1);
    cz1 = CLAMP((s32)((eye.z + distFar - sScatter->z0) / chunk), 0, sScatter->depth - 1);
    for (cz = cz0; cz <= cz1; cz++) {
        for (cx = cx0; cx <= cx1; cx++) {
            s32 c = cz * sScatter->width + cx;
            f32 mx = sScatter->x0 + (cx + 0.5f) * chunk - eye.x;
            f32 mz = sScatter->z0 + (cz + 0.5f) * chunk - eye.z;
            f32 d = SQ(mx) + SQ(mz);

            f32 my;
            f32 front;

            if ((sChunkStart[c] == sChunkStart[c + 1]) || (d > SQ(distFar + chunk))) {
                continue;
            }
            // the whole chunk outside the view cone (the same test as each prop's, with the chunk's size and the
            // water's depth as room)
            my = ((sChunkSurface[c] != -0x8000) ? sChunkSurface[c] - 150.0f : eye.y) - eye.y;
            front = mx * lookX + my * lookY + mz * lookZ + chunk * 0.71f + PROP_RADIUS + 150.0f;
            if (((front < 0.0f) || (SQ(front) < SQ(VIEW_COS) * (d + SQ(my)))) && (d > SQ(chunk * 1.5f))) {
                continue;
            }
            if (visibleCount < ARRAY_COUNT(sVisible)) {
                sChunkDistSq[visibleCount] = d;
                sVisible[visibleCount++] = c;
            }
        }
    }

    // where each kind's props start in each chunk (sorted by kind within a chunk: one pass finds them all)
    for (v = 0; v < visibleCount; v++) {
        s32 c = sVisible[v];
        s32 i = sChunkStart[c];
        s32 end = sChunkStart[c + 1];
        s32 k = 0;

        for (; i < end; i++) {
            while (k <= sProps[i].kind) {
                sKindStart[v][k++] = i;
            }
        }
        while (k <= FISHMOD_PROP_KIND_MAX) {
            sKindStart[v][k++] = end;
        }
    }

    // One pass over the props near the camera: in view (a cone around the look direction, any pitch), within their
    // kind's distance, sorted into near / mid / far bands. Kinds come in order within a chunk, and the chunks are
    // walked kind by kind, so each band is grouped by kind (one material per kind per band).
    bandCount[0] = bandCount[1] = bandCount[2] = 0;
    for (kind = 0; kind < FISHMOD_PROP_KIND_MAX; kind++) {
        FishmodPropKindInfo* info = &sKinds[kind];
        f32 maxDist = info->drawDist * distScale;
        f32 maxSq = SQ(maxDist);
        f32 reachSq = SQ(maxDist + chunk * 0.71f);

        for (v = 0; v < visibleCount; v++) {
            s32 c = sVisible[v];
            s32 i;
            s32 end;

            if (sChunkDistSq[v] > reachSq) {
                continue;
            }
            if (sChunkDry[c] && (info->floating || info->plant)) {
                continue; // floating props wait for the water, plants don't grow on dry land
            }
            end = sKindStart[v][kind + 1];
            for (i = sKindStart[v][kind]; i < end; i++) {
                FishmodProp* prop = &sProps[i];
                f32 dx = prop->x - eye.x;
                f32 dy = prop->y - eye.y;
                f32 dz = prop->z - eye.z;
                f32 dSq = SQ(dx) + SQ(dz);
                f32 front;

                if (dSq > maxSq) {
                    continue;
                }
                // outside the view cone (with room for the prop's size), unless it's right by the camera; squared,
                // no square roots: front + R < cos * dist  <=>  (front + R)^2 < cos^2 * dist^2 (when front + R >= 0)
                front = dx * lookX + dy * lookY + dz * lookZ + PROP_RADIUS;
                if (((front < 0.0f) || (SQ(front) < SQ(VIEW_COS) * (dSq + SQ(dy)))) &&
                    (dSq > SQ(PROP_RADIUS * 2.0f))) {
                    continue;
                }
                band = (dSq < SQ(BAND_NEAR)) ? 0 : (dSq < SQ(BAND_MID)) ? 1 : 2;
                if (bandCount[band] < BAND_MAX) {
                    sBand[band][bandCount[band]++] = i;
                }
            }
        }
    }

    // into the other set, nearest band first, as far as the budget goes
    sSet ^= 1;
    sMtxUsed = 0;
    set = &sBufs->set[sSet];
    set->count = 0;
    for (band = 0; band < BANDS; band++) {
        s32 n;

        for (n = 0; (n < bandCount[band]) && (budget > 0); n++) {
            FishmodProp* prop = &sProps[sBand[band][n]];

            if (sKinds[prop->kind].xlu && (xluLeft-- <= 0)) {
                continue;
            }
            budget--;
            Scatter_Matrix(prop, prop->kind == FISHMOD_PROP_LILY_PAD);
            set->prop[set->count++] = sBand[band][n];
        }
    }
}

void EnFishmod_ScatterDraw(PlayState* play) {
    Vec3f eye = play->view.eye;
    Vec3f at = play->view.at;
    f32 lookX;
    f32 lookY;
    f32 lookZ;
    f32 lookLen;
    s32 kind;
    s32 lastKind = -1;
    s32 n;
    Gfx* opa;
    Gfx* xlu;
    ScatterFrame* frame;
    ScatterSet* set;
#if FISHMOD_DEBUG
    static OSTime sT0, sT1, sT2;
    static s32 sBuilt;
    OSTime tStart = osGetTime();
#endif

    if ((sScatter == NULL) || (sChunkDry == NULL) || (sBufs == NULL)) {
        return;
    }
    Scatter_UpdateDry(play);
    // a camera being set up can give a nonsense eye for a frame (and turning a NaN into an integer is a crash on
    // the N64): skip the props that frame
    if (!(eye.x == eye.x) || !(eye.y == eye.y) || !(eye.z == eye.z) || (fabsf(eye.x) > 30000.0f) ||
        (fabsf(eye.z) > 30000.0f) || !(at.x == at.x) || !(at.y == at.y) || !(at.z == at.z)) {
        return;
    }
    lookX = at.x - eye.x;
    lookY = at.y - eye.y;
    lookZ = at.z - eye.z;
    lookLen = sqrtf(SQ(lookX) + SQ(lookY) + SQ(lookZ));
    if (lookLen < 0.01f) {
        return;
    }
    lookX /= lookLen;
    lookY /= lookLen;
    lookZ /= lookLen;

    // remake what's drawn only when the camera has moved or turned (or now and then anyway)
    sSetAge++;
    if ((sSetAge >= REUSE_FRAMES) ||
        ((SQ(eye.x - sSetEye.x) + SQ(eye.y - sSetEye.y) + SQ(eye.z - sSetEye.z)) > SQ(REUSE_MOVE)) ||
        ((lookX * sSetLook.x + lookY * sSetLook.y + lookZ * sSetLook.z) < REUSE_TURN_COS)) {
        Scatter_Build(play, &eye, lookX, lookY, lookZ);
        sSetAge = 0;
        sSetEye = eye;
        sSetLook.x = lookX;
        sSetLook.y = lookY;
        sSetLook.z = lookZ;
#if FISHMOD_DEBUG
        sBuilt++;
#endif
    }
#if FISHMOD_DEBUG
    {
        OSTime t = osGetTime();

        sT0 += t - tStart;
        tStart = t;
    }
#endif

    // the commands, every frame (the sway moves)
    sFrame ^= 1;
    frame = &sBufs->frame[sFrame];
    set = &sBufs->set[sSet];
    opa = frame->opa;
    xlu = frame->xlu;
    Scatter_Sway(play, frame);
    for (n = 0; n < set->count; n++) {
        FishmodProp* prop = &sProps[set->prop[n]];
        FishmodPropKindInfo* info;
        Gfx** disp;

        kind = prop->kind;
        info = &sKinds[kind];
        disp = info->xlu ? &xlu : &opa;
        if (kind != lastKind) {
            if ((lastKind >= 0) && !sKinds[lastKind].xlu) {
                gSPTexture(opa++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
            }
            lastKind = kind;
            *disp = Scatter_Material(*disp, info, kind);
        }
        if (!info->xlu) {
            u8 t = (prop->tint > 255 / 2) ? 255 : prop->tint * 2;

            gDPSetPrimColor(opa++, 0, 0, t, t, t, 255);
        }
        gSPMatrix((*disp)++, &set->mtx[n], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        if (info->sway != 0) {
            // neighbours share a phase: a wave through the bed
            s32 ph = ((prop->x + prop->z) >> 7) & (SWAY_PHASES - 1);

            gSPMatrix((*disp)++, &frame->sway[kind * SWAY_PHASES + ph], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        }
        gSPDisplayList((*disp)++, info->model);
    }
    if ((lastKind >= 0) && !sKinds[lastKind].xlu) {
        gSPTexture(opa++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    }
    // the dock, placed by hand: always drawn when in reach (one deck, a few posts)
    if ((play->sceneId == FISHMOD_DOCK_SCENE) &&
        ((SQ(eye.x - (FISHMOD_DOCK_X)) + SQ(eye.z - (FISHMOD_DOCK_Z + 210))) < SQ(3000.0f))) {
        static Mtx sDockMtx;
        static s32 sDockMtxMade;
        s32 k;

        if (!sDockMtxMade) {
            // a translation: the dock's own space is the world's, moved to its shore end
            bzero(&sDockMtx, sizeof(Mtx));
            sDockMtx.intPart[0][0] = sDockMtx.intPart[1][1] = sDockMtx.intPart[2][2] = sDockMtx.intPart[3][3] = 1;
            sDockMtx.intPart[3][0] = FISHMOD_DOCK_X;
            sDockMtx.intPart[3][1] = FISHMOD_DOCK_Y;
            sDockMtx.intPart[3][2] = FISHMOD_DOCK_Z;
            sDockMtxMade = true;
        }
        for (k = FISHMOD_PROP_DOCK_DECK; k <= FISHMOD_PROP_DOCK_POSTS; k++) {
            opa = Scatter_Material(opa, &sKinds[k], k);
            gDPSetPrimColor(opa++, 0, 0, 255, 255, 255, 255);
            gSPMatrix(opa++, &sDockMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(opa++, sKinds[k].model);
        }
        gSPTexture(opa++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    }
    gSPEndDisplayList(opa++);
    gSPEndDisplayList(xlu++);
#if FISHMOD_DEBUG
    sT1 += osGetTime() - tStart;
    sT2++;
    if ((play->gameplayFrames & 15) == 0) { // (printing is slow: not every frame)
        Fishmod_DebugPrintf(10, "p%d b%d us %d+%d", set->count, sBuilt, (s32)OS_CYCLES_TO_USEC(sT0 / sT2),
                            (s32)OS_CYCLES_TO_USEC(sT1 / sT2));
        sT0 = sT1 = sT2 = 0;
        sBuilt = 0;
    }
#endif

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_scatter.c", __LINE__);
    gSPDisplayList(POLY_OPA_DISP++, frame->opa);
    gSPDisplayList(POLY_XLU_DISP++, frame->xlu);
    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_scatter.c", __LINE__);
}

#endif
