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
#include "libc/math.h"
#include "ultra64.h"
#include "zelda_arena.h"
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
} FishmodPropKindInfo;

#define DIST_SMALL 350.0f
#define DIST_MID 600.0f
#define DIST_FAR 950.0f
// Props drawn per frame at most: each is a matrix and three commands in the frame's display buffer, which is far
// from bottomless. Close-range kinds (and nearer chunks) come first, so the budget runs out on the far ones.
#define PROP_BUDGET 260

static FishmodPropKindInfo sKinds[FISHMOD_PROP_KIND_MAX];

static void Scatter_InitKinds(void) {
#define X(id, name)                                                       \
    sKinds[FISHMOD_PROP_##id].material = gFishmodProp_##name##DLMaterial; \
    sKinds[FISHMOD_PROP_##id].model = gFishmodProp_##name##DL;
    // the generated kinds (the vanilla ones are set below)
    FISHMOD_PROP_KINDS_GENERATED(X)
#undef X
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
    sKinds[FISHMOD_PROP_GRASS_TUFT].drawDist = DIST_SMALL;
    sKinds[FISHMOD_PROP_LEAF_LITTER].drawDist = DIST_SMALL;
    sKinds[FISHMOD_PROP_PEBBLE_PATCH].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_RIBBON_WEED].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_BROADLEAF].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_BUSH_WEED].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_BRANCH].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_LILY_FLOWER].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_LILY_CENTRE].drawDist = DIST_MID;
    sKinds[FISHMOD_PROP_LILY_PAD].floating = true;
    sKinds[FISHMOD_PROP_FLOAT_LEAVES].floating = true;
    sKinds[FISHMOD_PROP_LILY_FLOWER].floating = true;
    sKinds[FISHMOD_PROP_LILY_CENTRE].floating = true;
    sKinds[FISHMOD_PROP_ROCK].plant = false;
    sKinds[FISHMOD_PROP_PEBBLE_PATCH].plant = false;
    sKinds[FISHMOD_PROP_SUNKEN_LOG].plant = false;
    sKinds[FISHMOD_PROP_BRANCH].plant = false;
    sKinds[FISHMOD_PROP_FALLEN_TREE].plant = false;
    sKinds[FISHMOD_PROP_DROWNED_TREE].plant = false;
    sKinds[FISHMOD_PROP_OLD_POSTS].plant = false;
}

static FishmodScatterHeader* sScatter;
static u16* sChunkStart;
static s16* sChunkSurface;
static FishmodProp* sProps;

void EnFishmod_ScatterLoad(PlayState* play) {
    EnFishmod_ScatterUnload();
    sScatter = Fishmod_LoadScatter(play->sceneId);
    if (sScatter == NULL) {
        return;
    }
    sChunkStart = (u16*)((u8*)sScatter + sScatter->chunkStartOffset);
    sChunkSurface = (s16*)((u8*)sScatter + sScatter->chunkSurfaceOffset);
    sProps = (FishmodProp*)((u8*)sScatter + sScatter->propsOffset);
    Scatter_InitKinds();
}

void EnFishmod_ScatterUnload(void) {
    if (sScatter != NULL) {
        ZeldaArena_Free(sScatter);
        sScatter = NULL;
    }
}

// The props' matrices, in a pool of their own (two frames' worth: the RSP reads last frame's while this one is
// built), not in the frame's display buffer, which the scene and the fish need.
static Mtx sMtxPool[2][PROP_BUDGET];
static s32 sMtxFrame;
static s32 sMtxUsed;

/** A prop's matrix: turned about Y, scaled, placed. Written straight into the fixed-point Mtx. */
static Mtx* Scatter_Matrix(GraphicsContext* gfxCtx, FishmodProp* prop, s32 flatScale) {
    Mtx* mtx = &sMtxPool[sMtxFrame][sMtxUsed++];
    MtxF mf;
    s16 yaw = prop->yaw << 8;
    f32 s = prop->scale * (1.0f / 64.0f);
    f32 c = Math_CosS(yaw) * s;
    f32 sn = Math_SinS(yaw) * s;

    // rows are the images of the x, y and z axes; the last row is the position
    mf.mf[0][0] = c;
    mf.mf[0][1] = 0.0f;
    mf.mf[0][2] = -sn;
    mf.mf[0][3] = 0.0f;
    mf.mf[1][0] = 0.0f;
    mf.mf[1][1] = flatScale ? 1.0f : s;
    mf.mf[1][2] = 0.0f;
    mf.mf[1][3] = 0.0f;
    mf.mf[2][0] = sn;
    mf.mf[2][1] = 0.0f;
    mf.mf[2][2] = c;
    mf.mf[2][3] = 0.0f;
    mf.mf[3][0] = prop->x;
    mf.mf[3][1] = prop->y;
    mf.mf[3][2] = prop->z;
    mf.mf[3][3] = 1.0f;
    guMtxF2L(mf.mf, mtx);
    return mtx;
}

void EnFishmod_ScatterDraw(PlayState* play) {
    static u16 sVisible[64]; // chunks near the camera
    Vec3f eye = play->view.eye;
    Vec3f at = play->view.at;
    f32 lookX;
    f32 lookZ;
    f32 lookLen;
    s32 visibleCount = 0;
    s32 cx0;
    s32 cx1;
    s32 cz0;
    s32 cz1;
    s32 cx;
    s32 cz;
    s32 kind;
    s32 chunk = sScatter ? sScatter->chunk : 1;
    s32 budget = PROP_BUDGET;
    s32 pass;

    if (sScatter == NULL) {
        return;
    }
    // a camera being set up can give a nonsense eye for a frame (and turning a NaN into an integer is a crash on
    // the N64): skip the props that frame
    if (!(eye.x == eye.x) || !(eye.z == eye.z) || (fabsf(eye.x) > 30000.0f) || (fabsf(eye.z) > 30000.0f) ||
        !(at.x == at.x) || !(at.z == at.z)) {
        return;
    }

    // the chunks within reach of the camera, and not behind it
    lookX = at.x - eye.x;
    lookZ = at.z - eye.z;
    lookLen = sqrtf(SQ(lookX) + SQ(lookZ));
    if (lookLen > 0.01f) {
        lookX /= lookLen;
        lookZ /= lookLen;
    }
    cx0 = CLAMP((s32)((eye.x - DIST_FAR - sScatter->x0) / chunk), 0, sScatter->width - 1);
    cx1 = CLAMP((s32)((eye.x + DIST_FAR - sScatter->x0) / chunk), 0, sScatter->width - 1);
    cz0 = CLAMP((s32)((eye.z - DIST_FAR - sScatter->z0) / chunk), 0, sScatter->depth - 1);
    cz1 = CLAMP((s32)((eye.z + DIST_FAR - sScatter->z0) / chunk), 0, sScatter->depth - 1);
    for (cz = cz0; cz <= cz1; cz++) {
        for (cx = cx0; cx <= cx1; cx++) {
            s32 c = cz * sScatter->width + cx;
            f32 mx = sScatter->x0 + (cx + 0.5f) * chunk - eye.x;
            f32 mz = sScatter->z0 + (cz + 0.5f) * chunk - eye.z;

            if (sChunkStart[c] == sChunkStart[c + 1]) {
                continue;
            }
            // behind the camera (with a chunk's width of slack)
            if ((mx * lookX + mz * lookZ) < -chunk) {
                continue;
            }
            if ((SQ(mx) + SQ(mz)) > SQ(DIST_FAR + chunk)) {
                continue;
            }
            if (visibleCount < ARRAY_COUNT(sVisible)) {
                sVisible[visibleCount++] = c;
            }
        }
    }
    if (visibleCount == 0) {
        return;
    }
    // nearest first
    {
        static f32 sDistSq[ARRAY_COUNT(sVisible)];
        s32 i;
        s32 j;

        for (i = 0; i < visibleCount; i++) {
            s32 c = sVisible[i];
            f32 mx = sScatter->x0 + ((c % sScatter->width) + 0.5f) * chunk - eye.x;
            f32 mz = sScatter->z0 + ((c / sScatter->width) + 0.5f) * chunk - eye.z;
            f32 d = SQ(mx) + SQ(mz);

            for (j = i - 1; (j >= 0) && (sDistSq[j] > d); j--) {
                sDistSq[j + 1] = sDistSq[j];
                sVisible[j + 1] = sVisible[j];
            }
            sDistSq[j + 1] = d;
            sVisible[j + 1] = c;
        }
    }

    sMtxFrame ^= 1;
    sMtxUsed = 0;

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_scatter.c", __LINE__);

    // close-range kinds first, then the mid and far ones (the budget runs out on the far ones)
    for (pass = 0; pass < 3; pass++)
    for (kind = 0; kind < FISHMOD_PROP_KIND_MAX; kind++) {
        FishmodPropKindInfo* info = &sKinds[kind];
        f32 maxSq = SQ(info->drawDist);
        s32 vanilla = (kind == FISHMOD_PROP_LILY_PAD) || (kind == FISHMOD_PROP_ROCK);
        s32 material = false;
        s32 v;

        if (info->drawDist != ((pass == 0) ? DIST_SMALL : (pass == 1) ? DIST_MID : DIST_FAR)) {
            continue;
        }

        for (v = 0; v < visibleCount; v++) {
            s32 c = sVisible[v];
            s32 i;
            s32 end = sChunkStart[c + 1];
            WaterBox* box;
            f32 surface;
            s32 lowered;

            // the water here now (the drained lake): floating props wait for it, plants don't grow on dry land
            lowered = false;
            if ((sChunkSurface[c] != -0x8000) && (info->floating || info->plant)) {
                f32 mx = sScatter->x0 + ((c % sScatter->width) + 0.5f) * chunk;
                f32 mz = sScatter->z0 + ((c / sScatter->width) + 0.5f) * chunk;

                if (!BgCheck_GetWaterSurface(play, &play->colCtx, mx, mz, &surface, &box) ||
                    (surface < sChunkSurface[c] - 20.0f)) {
                    lowered = true;
                }
            }
            if (lowered) {
                continue;
            }

            for (i = sChunkStart[c]; i < end; i++) {
                FishmodProp* prop = &sProps[i];
                f32 dx;
                f32 dz;
                u8 t;

                if (prop->kind != kind) {
                    if (prop->kind > kind) {
                        break; // sorted by kind within the chunk
                    }
                    continue;
                }
                dx = prop->x - eye.x;
                dz = prop->z - eye.z;
                if ((SQ(dx) + SQ(dz)) > maxSq) {
                    continue;
                }
                if (budget <= 0) {
                    break;
                }
                budget--;
                if (!material) {
                    material = true;
                    if (vanilla) {
                        // vanilla's own materials (lit by the scene's lights, like the Fishing Pond's props)
                        Gfx_SetupDL_25Opa(play->state.gfxCtx);
                        gSPDisplayList(POLY_OPA_DISP++, info->material);
                    } else {
                        Gfx_SetupDL_25Opa(play->state.gfxCtx);
                        gDPPipeSync(POLY_OPA_DISP++);
                        gDPSetTextureLUT(POLY_OPA_DISP++, G_TT_NONE);
                        gDPSetCombineLERP(POLY_OPA_DISP++, TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0, COMBINED, 0,
                                          PRIMITIVE, 0, 0, 0, 0, COMBINED);
                        gDPSetRenderMode(POLY_OPA_DISP++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_TEX_EDGE2);
                        gSPClearGeometryMode(POLY_OPA_DISP++, G_CULL_BACK | G_LIGHTING | G_TEXTURE_GEN |
                                                                  G_TEXTURE_GEN_LINEAR);
                        gSPSetGeometryMode(POLY_OPA_DISP++, G_FOG | G_SHADE | G_SHADING_SMOOTH);
                        gSPDisplayList(POLY_OPA_DISP++, info->material);
                    }
                }
                t = (prop->tint > 255 / 2) ? 255 : prop->tint * 2;
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, t, t, t, 255);
                gSPMatrix(POLY_OPA_DISP++, Scatter_Matrix(play->state.gfxCtx, prop, kind == FISHMOD_PROP_LILY_PAD),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(POLY_OPA_DISP++, info->model);
            }
        }
        if (material && !vanilla) {
            gSPTexture(POLY_OPA_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_scatter.c", __LINE__);
}

#endif
