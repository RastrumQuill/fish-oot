/*
 * File: fishmod_scatter.c
 * Overlay: ovl_En_Fishmod
 * Description: the scene's habitat props: weed beds, reeds, lily pads, rocks and wood, placed at build
 * time across the water by fishmod/tools/gen_scatter.py (shore-to-deep banding, the sub-biome zones, patchy density)
 * and drawn by the fish manager.
 *
 * Only the chunks near the camera are looked at, each kind's props are drawn together (its material once), and each
 * kind has a draw distance (small tufts close by, reeds, lilies and rocks from further away). A prop is a matrix,
 * a colour and its kind's display list: no per-vertex work.
 */

#include "player.h"
#include "libc64/malloc.h"
#include "z_en_fishmod.h"
#include "segmented_address.h"
#include "stddef.h"
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


// the props' textures and display lists (fishmod/tools/gen_habitat.py -> fishmod/tools/assets.py, made at build time)
#include "src/fishmod_data/fishmod_props.inc.c"
#include "src/fishmod_data/fishmod_props_kinds.h"
#include "fishmod_lod.h"
#include "fishmod_lod_tune.h"

// Lake Hylia's bed shaded under the full lake (fishmod/tools/gen_bedshade.py, made at build time)
typedef struct FishmodBedShade {
    Vtx* vtx; // the vertex, at its address in the room's segment (the linker's: right whatever the room's layout)
    s8 normal[3];
} FishmodBedShade;
#include "src/fishmod_data/fishmod_bedshade.h"
#include "src/fishmod_data/fishmod_profile.h"
#include "sys_math.h" // (Math_Atan2S: without its prototype its floats were passed as doubles)
#include "fishmod_bedcard.h"
#include "dma.h"

typedef enum FishmodPropKind {
#define X(id, name) FISHMOD_PROP_##id,
    FISHMOD_PROP_KINDS(X)
#undef X
    FISHMOD_PROP_KIND_MAX
} FishmodPropKind;

typedef struct FishmodPropKindInfo {
    Gfx* material;
    Gfx* model;
    Gfx* modelFar; // past PROP_FAR_DIST: the cards uncut (the same picture in fewer triangles), or the same model
    Gfx* modelAlt; // the one-triangle kinds' cut outline (the bench's comparison), else the model
    Gfx* modelLow; // the cut split into low triangles (the prop sweep's comparison: gen_habitat.py low_triangulate)
    Gfx* modelHalf;     // far, on the texture at half size (...DLFarH), or NULL
    Gfx* materialHalf;  // its texture
    f32 halfDist;       // from how far (at scale 1) a texel of the full texture is under half a pixel
    Gfx* modelImp;      // the far single card (fishmod/tools/impostor.py), turned to face the camera, or NULL
    Gfx* materialImp;
    f32 impDist;        // from how far (at scale 1)
    f32 drawDist;      // drawn no further than this (above water)
    f32 drawDistUnder; // and with the camera under water (fishmod_lod.h: measured, the haze hides the rest)
    u8 floating; // on the surface: not drawn while the water there is lower than it was placed for
    u8 plant;    // a living plant: not drawn where the water has gone (the drained lake)
    u8 xlu;      // drawn see-through (the Fishing Pond's reed, as vanilla draws it)
    u8 sway;     // how far its top sways (per mille of its height per unit of height); 0: still
    u8 swayRate; // how fast (a turn takes 0x10000 / (swayRate * 16) frames)
    u8 card;     // a big plant card: not drawn with the camera inside it (NEAR_SKIP)
    u8 faceCam;  // one card turned about its up, at most enough to stay within 45 degrees of facing the camera
    u16 height;  // how tall it is at scale 1 (gen_scatter.py HEIGHT), for the camera-inside test
    u16 extent;  // how far it reaches from where it stands at scale 1 (up, or down for hanging ones): the view test
} FishmodPropKindInfo;

#define SCATTER_IS_ROCK(kind) (((kind) == FISHMOD_PROP_ROCK) || ((kind) == FISHMOD_PROP_ROCK_LIT))

// the flags in each kind's settings (FISHMOD_PROP_KIND_INFO, from fishmod/data/prop_kinds.json)
#define PROPF_PLANT (1 << 0)
#define PROPF_FLOATING (1 << 1)
#define PROPF_XLU (1 << 2)
#define PROPF_FACE_CAM (1 << 3)
#define PROPF_CARD (1 << 4)

// (the profile's: fishmod/data/profiles.json)
#define DIST_SMALL FISHMOD_PROFILE_DIST_SMALL
#define DIST_MID FISHMOD_PROFILE_DIST_MID
#define DIST_FAR FISHMOD_PROFILE_DIST_FAR
// Props drawn per frame at most. They are drawn nearest band first (whatever their kind), so the budget runs out on
// the far ones. Each is a matrix and three commands, in buffers of the props' own (allocated with the scene's props).
#define PROP_BUDGET FISHMOD_PROFILE_PROP_BUDGET
// Nearest first, in bands this wide (kind by kind within a band, so one material per kind per band): the depth
// test then throws out most hidden pixels before they're written (fishmod/tools/overdraw.py --zorder: about a
// quarter fewer pixels written than three bands, close to a perfect nearest-first order)
#define BAND_WIDTH 100.0f
#define BANDS 32
// Props in view at most (before the budget)
#define VISIBLE_MAX 2400
// Material changes per frame at most (opaque, see-through): the command buffers are sized for them
#define MAT_MAX 256 // (a kind can change material at each tier: near, far, half, single card)
#define MAT_MAX_XLU 24
// The view cone: props outside it (behind, too far left or right, above or below) aren't drawn, so the budget goes on
// what's on screen. It's made from the camera's own field of view each time the set is made (Scatter_ViewCos): the
// screen's corner, plus VIEW_MARGIN for the frames the set is kept while the camera turns, plus the props' size as
// room. (A fixed cone for 60 degrees cut off the corners whenever the camera's view was wider: pop-in at the edges.)
#define VIEW_MARGIN_TAN 0.0875f // tan 5 degrees
#define PROP_RADIUS 90.0f
// See-through props (the pond reeds) drawn per frame at most
#define XLU_MAX FISHMOD_PROFILE_XLU_MAX
// Under water the haze hides everything past about this far, whatever the kind (the measured distances can be less)
#define DIST_UNDER_FOG FISHMOD_PROFILE_DIST_UNDER_FOG
// A plant the camera is inside or right against isn't drawn: one card there is a blur across the whole screen, at
// the RDP's full cost (closer than this times the plant's height, plus a little)
#define NEAR_SKIP 0.6f
// the cut-outs' edges smoothed by the VI alone, from the coverage the RDP writes (no read of the colour buffer):
// see Scatter_Material. Only for props nearer than PROP_EDGE_RA_DIST: far ones are a few pixels across, mostly edge,
// and lose those partly covered pixels without the blend (small lotus flowers vanished in the exact A/B capture,
// fishmod/tools/edge_ab_capture.py); the fill cost is in the near ones anyway
#define PROP_EDGE_RA 1
#define PROP_EDGE_RA_DIST 500.0f
// Past this, props draw their cards whole instead of cut to the texture's outline: the cut saves pixels, which near
// props are made of, but triples the triangles, which far props are made of (hwbench-2: far props were two thirds of
// the props' RDP time, and few pixels each)
#define PROP_FAR_DIST 500.0f
// How each prop is drawn (Scatter_Tier): within PROP_FAR_DIST its cut cards; past it its whole cards; from where the
// full texture's texels are under half a pixel, the same on a texture of half the size; from where it's at most
// IMP_PX (gen_habitat.py) tall on screen, one card with a picture of the whole plant, turned to face the camera. Past
// its distance, for FADE_BAND more of it, that card dissolves away (a dithered alpha test, in the opaque pass) instead
// of the plant popping: everything within the distance is drawn as before, the fade is added beyond it.
typedef enum {
    TIER_NEAR,
    TIER_FAR,
    TIER_HALF,
    TIER_IMP,
    TIER_MAX
} ScatterTier;
#define FADE_BAND 0.15f
// The single card turns only about its own up, so it holds only for plants seen from near the side: seen from above
// (the bed through the water from a shore, looking down) it would be a sliver. Single cards only within this slope
// of the view to the plant (tan 20 degrees, squared)
#define IMP_SLOPE_SQ 0.132f
static f32 sKindMax[FISHMOD_PROP_KIND_MAX]; // how far each kind is drawn, as last built (above or under water)
static s16 sBillboardYaw;                    // the single cards' turn: facing the camera (as last built)
// Under water the haze hides detail much sooner, and the kinds' distances are shorter (500-900): the tiers' distances
// are this much nearer there, so the far tiers still do their work
#define UNDER_LOD_SCALE 0.5f
static f32 sLodScale = 1.0f; // (as last built: UNDER_LOD_SCALE under water)
// With the camera under water, each kind's distances as the LOD tuning measured them (fishmod_lod_tune.h: the
// shortest that changed no view, per time of day), at the prop's scale; 0: the defaults. Above water the defaults
// (set by eye: the tuning's views there were harsher than the game's).
static f32 sTunedCut[FISHMOD_PROP_KIND_MAX];
// (the tuned cut no further than this: past it the shapes in the haze are faint, and the budget goes on nearer
// plants; the dissolve past the cut hides the edge)
#define UNDER_CUT_MAX 1400.0f
// the tall kinds (the most scanlines per prop) no further than this under water, in the thicker haze
#define UNDER_CUT_TALL 900.0f
static f32 sTunedImp[FISHMOD_PROP_KIND_MAX];
static f32 sTunedHalf[FISHMOD_PROP_KIND_MAX];

/** The tuned distances for now: of the two measured times either side of the clock, the further (never short). */
static void Scatter_TunedDistances(s32 under) {
    // the table's times: dawn 6:30, noon, dusk 18:30, midnight
    u16 t = gSaveContext.save.dayTime;
    s32 a;
    s32 b;
    s32 k;
    s32 i;

    bzero(sTunedCut, sizeof(sTunedCut));
    bzero(sTunedImp, sizeof(sTunedImp));
    bzero(sTunedHalf, sizeof(sTunedHalf));
    if (!under || (ARRAY_COUNT(sLodTuned) != FISHMOD_PROP_KIND_MAX)) {
        return; // (above water, or the table is for another set of kinds)
    }
    if (t < CLOCK_TIME(6, 30)) {
        a = 3, b = 0;
    } else if (t < CLOCK_TIME(12, 0)) {
        a = 0, b = 1;
    } else if (t < CLOCK_TIME(18, 30)) {
        a = 1, b = 2;
    } else {
        a = 2, b = 3;
    }
    for (k = 0; k < FISHMOD_PROP_KIND_MAX; k++) {
        f32* out[3] = { &sTunedCut[k], &sTunedImp[k], &sTunedHalf[k] };

        for (i = 0; i < 3; i++) {
            *out[i] = MIN(MAX(sLodTuned[k][a][1][i], sLodTuned[k][b][1][i]), UNDER_CUT_MAX);
        }
        if ((k == FISHMOD_PROP_KELP_FROND) || (k == FISHMOD_PROP_TALL_WEED) || (k == FISHMOD_PROP_RIBBON_WEED) ||
            (k == FISHMOD_PROP_BIG_PONDWEED) || (k == FISHMOD_PROP_VINE_STRAND)) {
            sTunedCut[k] = (sTunedCut[k] > 0.0f) ? MIN(sTunedCut[k], UNDER_CUT_TALL) : 0.0f;
        }
    }
}
#if FISHMOD_DEBUG
static u32 sSetBench; // the bench flags the set was last made with
#endif
static f32 sBedViewSin[FISHMOD_BEDCARD_VIEWS]; // the bed cards' views: sin and cos of each one's angle
static f32 sBedViewCos[FISHMOD_BEDCARD_VIEWS];
static s32 sBedViewTrigMade;
static s32 sEdgeNear; // the props being drawn are within PROP_EDGE_RA_DIST
#if FISHMOD_DEBUG
// what the LOD calibration (fishmod_debug.c, FISHMOD_LOD_CALIBRATE) draws everything out to
#define DIST_CALIBRATE 3000.0f
#endif

static FishmodPropKindInfo sKinds[FISHMOD_PROP_KIND_MAX];
// each kind's own draw distance with the camera under water, 0: as above it (fishmod/data/prop_kinds.json)
#define X(id, dist, distUnder, flags, swayAmount, rate, cardHeight, reach) (distUnder),
static const f32 sDistUnderOwn[FISHMOD_PROP_KIND_MAX] = { FISHMOD_PROP_KIND_INFO(X) };
#undef X
static f32 sDistFar[2]; // the furthest any kind is drawn: above water, under it (how far the chunks are looked at)
static f32 sExtentMax; // the furthest any prop reaches from where it stands (the chunks' view test)

static void Scatter_InitKinds(void) {
#define X(id, name)                                                       \
    sKinds[FISHMOD_PROP_##id].material = gFishmodProp_##name##DLMaterial; \
    sKinds[FISHMOD_PROP_##id].model = gFishmodProp_##name##DL;            \
    sKinds[FISHMOD_PROP_##id].modelFar = gFishmodProp_##name##DLFar;     \
    sKinds[FISHMOD_PROP_##id].modelLow = gFishmodProp_##name##DLLow;
    // the generated kinds (the vanilla ones are set below)
    FISHMOD_PROP_KINDS_GENERATED(X)
#undef X
    sKinds[FISHMOD_PROP_POND_REED].material = gFishingReedMaterialDL;
    sKinds[FISHMOD_PROP_POND_REED].model = gFishingReedModelDL;
    sKinds[FISHMOD_PROP_LILY_PAD].material = gFishingLilyPadMaterialDL;
    sKinds[FISHMOD_PROP_LILY_PAD].model = gFishingLilyPadModelDL;
    // the rocks: vanilla's rock with vertex colours (moss on top, darker at the base: gen_habitat.py rock_vc), drawn
    // unlit like the plants
    sKinds[FISHMOD_PROP_ROCK].material = gFishmodProp_rockvcDLMaterial;
    sKinds[FISHMOD_PROP_ROCK].model = gFishmodProp_rockvcDL;
    sKinds[FISHMOD_PROP_POND_REED].modelFar = gFishingReedModelDL;
    sKinds[FISHMOD_PROP_LILY_PAD].modelFar = gFishingLilyPadModelDL;
    sKinds[FISHMOD_PROP_ROCK].modelFar = gFishmodProp_rockvcDL;
    // the rocks in the lake's shallows: the same, the bed's caustics on them too (Scatter_Material: in 2-cycle mode,
    // with F3DEX3's lighting for the props' colour; else drawn as the others)
    sKinds[FISHMOD_PROP_ROCK_LIT].material = gFishmodProp_rockvcDLMaterial;
    sKinds[FISHMOD_PROP_ROCK_LIT].model = gFishmodProp_rockvcDL;
    sKinds[FISHMOD_PROP_ROCK_LIT].modelFar = gFishmodProp_rockvcDL;
    {
        s32 i;

        for (i = 0; i < FISHMOD_PROP_KIND_MAX; i++) {
            sKinds[i].modelAlt = sKinds[i].model;
            if (sKinds[i].modelLow == NULL) {
                sKinds[i].modelLow = sKinds[i].model; // (the vanilla kinds)
            }
        }
    }
#define X(id, name) sKinds[FISHMOD_PROP_##id].modelAlt = gFishmodProp_##name##DLAlt;
    FISHMOD_PROP_KINDS_TRI(X)
#undef X
#define X(id, name, tex, dist)                                               \
    sKinds[FISHMOD_PROP_##id].modelHalf = gFishmodProp_##name##DLFarH;          \
    sKinds[FISHMOD_PROP_##id].materialHalf = gFishmodProp_##name##DLFarHMaterial; \
    sKinds[FISHMOD_PROP_##id].halfDist = (dist);
    FISHMOD_PROP_KINDS_HALF(X)
#undef X
#define X(id, name, dist)                                                    \
    sKinds[FISHMOD_PROP_##id].modelImp = gFishmodProp_##name##DLImp;          \
    sKinds[FISHMOD_PROP_##id].materialImp = gFishmodProp_##name##DLImpMaterial; \
    sKinds[FISHMOD_PROP_##id].impDist = (dist);
    FISHMOD_PROP_KINDS_IMP(X)
#undef X
    // each kind's settings (fishmod/data/prop_kinds.json); swaying: weed slowly with the water's movement, reeds above
    // it faster in the wind (the shear moves a point by its height, so tall kinds get less of it: every tip moves about
    // the same few units)
#define X(id, dist, distUnder, flags, swayAmount, rate, cardHeight, reach)              \
    sKinds[FISHMOD_PROP_##id].drawDist = (dist);                                        \
    sKinds[FISHMOD_PROP_##id].plant = ((flags) & PROPF_PLANT) != 0;                     \
    sKinds[FISHMOD_PROP_##id].floating = ((flags) & PROPF_FLOATING) != 0;               \
    sKinds[FISHMOD_PROP_##id].xlu = ((flags) & PROPF_XLU) != 0;                         \
    sKinds[FISHMOD_PROP_##id].faceCam = ((flags) & PROPF_FACE_CAM) != 0;                \
    sKinds[FISHMOD_PROP_##id].card = ((flags) & PROPF_CARD) != 0;                       \
    sKinds[FISHMOD_PROP_##id].sway = (swayAmount);                                      \
    sKinds[FISHMOD_PROP_##id].swayRate = (rate);                                        \
    sKinds[FISHMOD_PROP_##id].height = (cardHeight);                                    \
    sKinds[FISHMOD_PROP_##id].extent = ((reach) != 0) ? (reach) : ((cardHeight) != 0) ? (cardHeight) : 150;
    FISHMOD_PROP_KIND_INFO(X)
#undef X
    {
        // how far each reaches from where it stands (the view test's room for its size): the cards their height,
        // the rest a generous default (reeds, rocks, logs, hanging vines) or their own (the pond's tall reed)
        s32 i;

        for (i = 0; i < FISHMOD_PROP_KIND_MAX; i++) {
            sExtentMax = MAX(sExtentMax, sKinds[i].extent * 1.6f); // (the largest scale)
        }
    }
    {
        s32 i;

        // under water no further than the haze lets anything be seen, then what the calibration measured
        // (fishmod_lod.h, made by fishmod/tools/lod_calibrate.py from the game's own frames): only ever nearer, so
        // the frame is the same and cheaper
        for (i = 0; i < FISHMOD_PROP_KIND_MAX; i++) {
            // (or a kind's own, e.g. moss: nearer still in the haze)
            sKinds[i].drawDistUnder = (sDistUnderOwn[i] > 0.0f) ? sDistUnderOwn[i] : MIN(sKinds[i].drawDist, DIST_UNDER_FOG);
        }
#define X(id, above, under)                                    \
    if ((above) > 0) {                                         \
        sKinds[FISHMOD_PROP_##id].drawDist = MIN(sKinds[FISHMOD_PROP_##id].drawDist, (above)); \
    }                                                          \
    if ((under) > 0) {                                         \
        sKinds[FISHMOD_PROP_##id].drawDistUnder = MIN(sKinds[FISHMOD_PROP_##id].drawDistUnder, (under)); \
    }
        FISHMOD_LOD_DISTANCES(X)
#undef X
        sDistFar[0] = sDistFar[1] = 0.0f;
        for (i = 0; i < FISHMOD_PROP_KIND_MAX; i++) {
            f32 fade = (sKinds[i].modelImp != NULL) ? (1.0f + FADE_BAND) : 1.0f; // (the dissolve, past the distance)

            sDistFar[0] = MAX(sDistFar[0], fade * sKinds[i].drawDist);
            sDistFar[1] = MAX(sDistFar[1], fade * sKinds[i].drawDistUnder);
            // (and the tuned distances under water, at any time of day: the chunks are gathered that far)
            if (ARRAY_COUNT(sLodTuned) == FISHMOD_PROP_KIND_MAX) {
                s32 t;

                for (t = 0; t < FISHMOD_LOD_TUNE_TIMES; t++) {
                    sDistFar[1] = MAX(sDistFar[1], fade * MIN(sLodTuned[i][t][1][0], UNDER_CUT_MAX));
                }
            }
        }
    }
}

static FishmodScatterHeader* sScatter;
static Gfx** sRockList; // each rock's own list (made for where it lies: rock_mesh.py), by its prop's yaw; or NULL
/*
 * The rocks in Lake Hylia's shallows have the bed's caustics on them (FISHMOD_PROP_ROCK_LIT): the bed's own blend, the
 * caustic texture into the rock's by the environment alpha, scrolled as the bed's is, and faded with depth in the
 * bed's steps (tools/meshpack.py caustic_fade; gen_scatter.py gives each rock its step, a call to sRockStep's list).
 * Not through the scene's segments: other actors' draws use them for their own by the time the props are drawn.
 */
#define ROCK_STEPS 4
#define ROCK_STEP_TOKEN 0xF15C0000 // (rock_mesh.py STEP_TOKEN)
#define ROCK_LIGHT 28 // the caustics' light on a rock, sixteenths of the bed's share (its 87 of 255: about 0.6)
static Gfx sRockStep[ROCK_STEPS][2];
static Gfx* sRockScroll; // this frame's (in its buffer: ScatterFrame.rockScroll)
static s32 sRockStepBase = -1;
static Gfx sRockLitMaterial[] = {
    gsDPSetTextureLUT(G_TT_NONE),
    // tile 0: the bed's caustics as light (their brightness: gen_habitat.py), on the rock's own coordinates at a
    // quarter (about the bed's size in the world)
    gsDPLoadTextureBlock(gFishmodProp_causticTex, G_IM_FMT_I, G_IM_SIZ_8b, 32, 32, 0, G_TX_NOMIRROR | G_TX_WRAP,
                         G_TX_NOMIRROR | G_TX_WRAP, 5, 5, 2, 2),
    // tile 1: the rock
    gsDPLoadMultiBlock(gFishmodProp_rockvcTexRGBA, 0x100, 1, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                       G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsSPEndDisplayList(),
};
static u16* sChunkStart;
// each non-empty chunk's kind boundaries (where each kind's props start in it: they're sorted by kind), made once at
// load instead of by a pass over the chunk's props in every build; sChunkKindRow[c] its row in sChunkKindPool
static u16* sChunkKindRow;
static u16* sChunkKindPool;
static s16* sChunkSurface;
static FishmodProp* sProps;
// Per chunk: the water there is lower than the props were placed for (the drained lake). Placement is all done at
// build time; the one thing only the game knows is which water level this visit has (vanilla picks it from the save:
// adult before the Water Temple is cleared). So it is worked out once, on the first frame drawn (the lake's own actor
// sets the level during the scene's first update), and again only if that save flag changes.
static u8* sChunkDry;
static s32 sDryFor;  // the water levels (Scatter_WaterLevels) the dry table was made for
static u8 sDryMade;  // made at all yet
static void* sBedShadedRoom; // the room whose bed was shaded (a room loaded again is the ROM's again)

// Two frames' worth of matrices and commands (the RSP reads last frame's while this one is built), in buffers of
// their own rather than the frame's display buffer, which the scene and the fish need.
// Sway: each swaying kind has a few phases (props pick one by where they stand, so neighbours move together, like a
// wave through a bed), each a small shear applied to the prop's own shape before it is placed.
#define SWAY_PHASES 4
#define SWAY_MAX (FISHMOD_PROP_KIND_MAX * SWAY_PHASES)

// (Scatter_Touch's: the props making way for Link)
#define TOUCH_MAX 32
#define TOUCH_NONE 0xFF
#define TOUCH_LEAN 0.6f      // the most a plant's top moves sideways, per unit of its height (held up by the water)
#define TOUCH_PUSH 32.0f     // the furthest a floating plant drifts
#define TOUCH_FLOAT_R 46.0f  // how near a floating plant's middle Link pushes it (at scale 1; plus TOUCH_FLOAT_R0)
#define TOUCH_FLOAT_R0 18.0f
#define TOUCH_HANG 90.0f     // how far a hanging plant (the vine curtain) reaches down from the surface, at scale 1
// a floating kind that hangs from the surface: it swings from where it's held, as a plant from its foot, not drifts
#define TOUCH_HANGS(kind) ((kind) == FISHMOD_PROP_VINE_CURTAIN)
#define TOUCH_DRIFTS(kind) (sKinds[kind].floating && !TOUCH_HANGS(kind))
#define TOUCH_EASE 0.3f      // how fast they follow (a share of the way each frame)
#define TOUCH_BACK 0.03f     // and come back (floating: slowly, his wake lingers, as the Fishing Pond's pads)
#define TOUCH_SPRING 0.13f   // (a plant: springs back up, its stiffness (its buoyancy)...
#define TOUCH_DAMP 0.22f     //  ...and how quickly its swing dies down: a little past upright, back, and still)
// A touched plant bends, not tips: its vertices loaded in bands up its height, each band under more of the lean than
// the one below (Scatter_Bend): the top goes furthest and the stalk curves. The kind's model cut into the bands once.
#define BEND_BANDS 4
#define BEND_VTX_MAX (ENABLE_F3DEX3 ? 56 : 32) // (the microcode's vertex buffer: every band's vertices in it at once)
typedef struct ScatterTouch {
    u16 prop;
    u8 used;
    u8 hit; // touched this frame
    f32 x;  // the lean (or the drift, floating), in the world's x and z
    f32 z;
    f32 tx; // where it's heading this frame
    f32 tz;
    f32 vx; // (a plant springing back: how fast)
    f32 vz;
} ScatterTouch;
typedef struct ScatterBend {
    Vtx* vtx;                     // the model's vertices cut at the bands' heights, band by band
    u8 start[BEND_BANDS + 1];     // each band's first
    Gfx* tris;                    // the triangles, drawn once every band is loaded
} ScatterBend;
static ScatterBend* sBend[FISHMOD_PROP_KIND_MAX]; // (made the first time one of the kind is touched)
static u8 sBendTried[FISHMOD_PROP_KIND_MAX];
static ScatterTouch sTouch[TOUCH_MAX];
static u8* sTouchSlot; // [sScatter->count]: each prop's touch, or TOUCH_NONE
static u32 sTouchGen;  // a touch begun or ended
static u8 sTouchable[FISHMOD_PROP_KIND_MAX]; // the kinds that make way (swaying plants, floating ones)

// The props' commands, two frames' worth (the RSP reads last frame's while this one is built), in buffers of their
// own rather than the frame's display buffer, which the scene and the fish need.
typedef struct ScatterFrame {
    // (the dock, the far edge mode's material, the touched; the props' water fog's copied models)
    Gfx opa[PROP_BUDGET * 7 + MAT_MAX * 16 + 96 + TOUCH_MAX * (BEND_BANDS * 4 + 2)];
    Gfx xlu[XLU_MAX * 3 + MAT_MAX_XLU * 16 + 4 + TOUCH_MAX * (BEND_BANDS * 4 + 2)];
    Mtx sway[SWAY_MAX];
    Mtx bend[TOUCH_MAX]; // the props Link is touching: bent or pushed aside (Scatter_Touch)
    Mtx bendBand[TOUCH_MAX][BEND_BANDS]; // (a touched plant's bands' leans: Scatter_Bend)
    Gfx rockScroll[5];   // the shallow rocks' caustics scrolled (Scatter_RockCaustics)
    Lights lights; // the scene's lights, this frame's (here so the kept commands' address for them stays the same)
} ScatterFrame;
// What's drawn and the props' matrices: made when the camera has moved, and kept while it holds still (fishing,
// mostly), so most frames only send the commands again. Two sets: a new one is made while the RSP may still be
// reading the last.
#define BEDCARD_SET_MAX 64 // bed cards drawn in a frame at most
typedef struct ScatterSet {
    Mtx mtx[PROP_BUDGET];
    Mtx* mtxp[PROP_BUDGET]; // each drawn prop's matrix: one of mtx, or its own kept one (sPropMtx)
    u16 prop[PROP_BUDGET]; // in drawing order: nearest band first, kind by kind within a band
    u8 tier[PROP_BUDGET];  // ScatterTier, as its matrix was made
    u8 hidden[PROP_BUDGET]; // wholly beyond the screen's edges: in the set (as the cone chose it), not drawn
    s32 count;
    s16 cards[BEDCARD_SET_MAX]; // the bed cards drawn in place of their chunks' card-drawn plants
    s32 cardCount;
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
#if FISHMOD_DEBUG
static u32 sSetPropTest; // the prop sweep's test the set was made for
#endif
static s32 sSetAge = 1000;
// Remake the set when the camera has moved this far or turned this much, or after this many frames anyway
#define REUSE_MOVE 6.0f
#define REUSE_TURN_COS 0.9996f
#define REUSE_FRAMES 10

/**
 * The props' commands kept (fish-oot). While the set, the light on the props and the mode stay the same, the commands
 * for the set's props up to the first one fading out (the single cards, which fade with the camera's every move) are
 * the same every frame, and each frame's buffer still holds what it was given two frames ago: those commands are left
 * where they are, and only the fading props after them are made again. The addresses in them (the matrices, this
 * buffer's sway matrices and scene lights) are this buffer's own, so they hold too. The same commands, byte for byte
 * (SCATTER_VERIFY checks it every frame).
 */
typedef struct ScatterKeep {
    s32 opaLen;  // the kept commands (0: none kept)
    s32 xluLen;
    s32 opaFrom; // where they start in the buffer
    s32 xluFrom;
    s32 until;   // the props they cover: the set's first so many
    u32 setGen;  // the set they were made from (its generation)
    u32 light[3];
    s32 oneCycle;
    s32 numLights;
    u32 bench;
    u32 touchGen; // the touched props (Scatter_Touch): which have a bend matrix
    s32 lastOpa; // the loop's state where they end
    s32 lastXlu;
    s32 lastNear;
    s32 edgeNear;
    u32 fogKey;     // the props' water fog's settings (Scatter_PropFogSetup)
    s32 fogVtxUsed; // its copies where they end
    s32 fogCmdUsed;
} ScatterKeep;
static ScatterKeep sKeep[2]; // a frame buffer's each
static Mtx* sPropMtx;     // each prop's own matrix, kept once made ([sScatter->count]), or NULL: no room
static u8* sPropMtxMade;  // [sScatter->count]
static s32 sSwayInit[2];     // a frame buffer's sway matrices have their fixed entries
static u32 sSetGen; // a new set every Scatter_Build

static void Scatter_FogAlloc(void);
static void Scatter_FogFree(void);
static void* Scatter_BendAddr(u32 a, u8* seg6);

static void Scatter_AllocBuffers(void) {
    if (sBufs == NULL) {
        sBufs = ZeldaArena_Malloc(sizeof(ScatterBuffers));
    }
    Scatter_FogAlloc();
    sKeep[0].opaLen = sKeep[1].opaLen = 0;
    sSwayInit[0] = sSwayInit[1] = false;
    sSetAge = 1000;
}

static void Scatter_FreeBuffers(void) {
    if (sBufs != NULL) {
        ZeldaArena_Free(sBufs);
        sBufs = NULL;
    }
    Scatter_FogFree();
    sKeep[0].opaLen = sKeep[1].opaLen = 0;
    sSwayInit[0] = sSwayInit[1] = false;
}


static void BedCard_Load(PlayState* play);

void EnFishmod_ScatterLoad(PlayState* play) {
    EnFishmod_ScatterUnload();
    sScatter = Fishmod_LoadScatter(play->sceneId);
    if (sScatter == NULL) {
        return;
    }
    sChunkStart = (u16*)((u8*)sScatter + sScatter->chunkStartOffset);
    sChunkSurface = (s16*)((u8*)sScatter + sScatter->chunkSurfaceOffset);
    sProps = (FishmodProp*)((u8*)sScatter + sScatter->propsOffset);
    sRockList = NULL;
    if (sScatter->rockCount != 0) {
        // the rocks' lists: offsets in the file made addresses, and their vertex loads too
        u32* list = (u32*)((u8*)sScatter + sScatter->rockListOffset);
        s32 i;

        for (i = 0; i < sScatter->rockCount; i++) {
            u32* w = (u32*)((u8*)sScatter + list[i]);

            list[i] = (u32)w;
            for (; (w[0] >> 24) != G_ENDDL; w += 2) {
                if ((w[0] >> 24) == G_VTX) {
                    w[1] += (u32)sScatter;
                } else if (((w[0] >> 24) == G_DL) && ((w[1] & 0xFFFF0000) == ROCK_STEP_TOKEN)) {
                    w[1] = (u32)sRockStep[CLAMP_MAX(w[1] & 0xFFFF, ROCK_STEPS - 1)];
                }
            }
        }
        osWritebackDCache((u8*)sScatter + sScatter->rockDataOffset, sScatter->rockDataWords * sizeof(u32));
        sRockList = (Gfx**)list;
    }
    sChunkDry = ZeldaArena_Malloc(sScatter->width * sScatter->depth);
    sDryMade = false;
    {
        s32 chunks = sScatter->width * sScatter->depth;
        s32 rows = 0;
        s32 c;

        for (c = 0; c < chunks; c++) {
            rows += (sChunkStart[c] != sChunkStart[c + 1]);
        }
        sChunkKindRow = ZeldaArena_Malloc(chunks * sizeof(u16));
        sChunkKindPool = ZeldaArena_Malloc(rows * (FISHMOD_PROP_KIND_MAX + 1) * sizeof(u16));
        if ((sChunkKindRow == NULL) || (sChunkKindPool == NULL) || (rows >= 0xFFFF)) {
            if (sChunkKindRow != NULL) {
                ZeldaArena_Free(sChunkKindRow);
            }
            if (sChunkKindPool != NULL) {
                ZeldaArena_Free(sChunkKindPool);
            }
            sChunkKindRow = sChunkKindPool = NULL; // (each build finds them as before)
        } else {
            rows = 0;
            for (c = 0; c < chunks; c++) {
                u16* row;
                s32 i = sChunkStart[c];
                s32 end = sChunkStart[c + 1];
                s32 k = 0;

                if (i == end) {
                    sChunkKindRow[c] = 0xFFFF;
                    continue;
                }
                sChunkKindRow[c] = rows;
                row = &sChunkKindPool[rows++ * (FISHMOD_PROP_KIND_MAX + 1)];
                // (the same pass as Scatter_Build's was)
                for (; i < end; i++) {
                    while (k <= sProps[i].kind) {
                        row[k++] = i;
                    }
                }
                while (k <= FISHMOD_PROP_KIND_MAX) {
                    row[k++] = end;
                }
            }
        }
    }
    sBedShadedRoom = NULL;
    Scatter_AllocBuffers();
    sTouchSlot = ZeldaArena_Malloc(sScatter->count);
    if (sTouchSlot != NULL) {
        s32 i;

        for (i = 0; i < sScatter->count; i++) {
            sTouchSlot[i] = TOUCH_NONE;
        }
    }
    bzero(sTouch, sizeof(sTouch));
    if (osMemSize >= 0x800000) {
        // (the Expansion Pak: room for every prop's matrix; without it, or if it doesn't fit, made in each set)
        sPropMtx = ZeldaArena_Malloc(sScatter->count * sizeof(Mtx));
        sPropMtxMade = ZeldaArena_Malloc(sScatter->count);
        if ((sPropMtx == NULL) || (sPropMtxMade == NULL)) {
            if (sPropMtx != NULL) {
                ZeldaArena_Free(sPropMtx);
            }
            if (sPropMtxMade != NULL) {
                ZeldaArena_Free(sPropMtxMade);
            }
            sPropMtx = NULL;
            sPropMtxMade = NULL;
        } else {
            bzero(sPropMtxMade, sScatter->count);
        }
    }
    BedCard_Load(play);
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
    {
        // (the kinds that make way for Link: Scatter_Touch)
        s32 k;

        for (k = 0; k < FISHMOD_PROP_KIND_MAX; k++) {
            sTouchable[k] = sKinds[k].floating || (sKinds[k].plant && (sKinds[k].sway != 0));
        }
    }
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
        FishmodProp* prop = &sProps[(s32)Fishmod_RandZeroFloat(gFishmodRngNow, sScatter->count - 0.01f)];

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
        FishmodProp* prop = &sProps[set->prop[(s32)Fishmod_RandZeroFloat(gFishmodRngNow, set->count - 0.01f)]];
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

/* ---------------------------------------------------------------------------------------------------------------
 * Bed cards (fishmod/tools/bedcard.py): past BEDCARD_DIST a chunk's card-drawn plants are one card, laid in the plane of
 * the view nearest the camera's direction to it. The card table stays loaded; each card's views (2.4 KB) are read from
 * ROM into a cache the first time it's wanted (a few a frame; until then its plants are drawn as before).
 */
#define BEDCARD_DIST 1300.0f // from this far (to the card's middle, across the ground)
#define BEDCARD_CACHE 24     // cards' views held at once
#define BEDCARD_LOADS 2      // read from ROM in a frame at most

static FishmodBedCard* sBedCards;
static s32 sBedCount;
static s16* sChunkCard; // per chunk: its card, or -1
static uintptr_t sBedRom; // the first card's views in ROM
static u8* sBedCache;
static s16 sBedSlotCard[BEDCARD_CACHE];
static u32 sBedSlotUsed[BEDCARD_CACHE]; // the frame it was last drawn (a slot the RSP may still read isn't reused)
static s8* sBedCardSlot; // per card: its slot, or -1
static s32 sBedLoads;    // read this frame
static u32 sBedFrame;

static void BedCard_Unload(void) {
    if (sBedCards != NULL) {
        ZeldaArena_Free(sBedCards);
        sBedCards = NULL;
    }
    if (sChunkCard != NULL) {
        ZeldaArena_Free(sChunkCard);
        sChunkCard = NULL;
    }
    if (sBedCache != NULL) {
        ZeldaArena_Free(sBedCache);
        sBedCache = NULL;
    }
    if (sBedCardSlot != NULL) {
        ZeldaArena_Free(sBedCardSlot);
        sBedCardSlot = NULL;
    }
    sBedCount = 0;
}

static void BedCard_Load(PlayState* play) {
    static u64 sHeadBuf[sizeof(FishmodBedCardHeader) / sizeof(u64)]; // (8-byte aligned, for the DMA)
    FishmodBedCardHeader* head = (FishmodBedCardHeader*)sHeadBuf;
    uintptr_t romStart;
    uintptr_t romEnd;
    s32 n;
    s32 i;

    BedCard_Unload();
    if ((sScatter == NULL) || !Fishmod_BedCardRom(play->sceneId, &romStart, &romEnd)) {
        return;
    }
    DmaMgr_RequestSync(head, romStart, sizeof(FishmodBedCardHeader));
    if ((head->magic != FISHMOD_BEDCARD_MAGIC) || (head->count == 0) || (head->cardBytes != FISHMOD_BEDCARD_BYTES)) {
        return;
    }
    n = head->count;
    sBedCards = ZeldaArena_Malloc(ALIGN16(n * sizeof(FishmodBedCard)));
    sChunkCard = ZeldaArena_Malloc(sScatter->width * sScatter->depth * sizeof(s16));
    sBedCache = ZeldaArena_Malloc(BEDCARD_CACHE * FISHMOD_BEDCARD_BYTES);
    sBedCardSlot = ZeldaArena_Malloc(ALIGN16(n));
    if ((sBedCards == NULL) || (sChunkCard == NULL) || (sBedCache == NULL) || (sBedCardSlot == NULL)) {
        BedCard_Unload();
        return;
    }
    // the table (its size rounded up: the read may take a few bytes past it, which are the views' start)
    DmaMgr_RequestSync(sBedCards, romStart + sizeof(FishmodBedCardHeader), ALIGN16(n * sizeof(FishmodBedCard)));
    sBedRom = romStart + sizeof(FishmodBedCardHeader) + n * sizeof(FishmodBedCard);
    for (i = 0; i < sScatter->width * sScatter->depth; i++) {
        sChunkCard[i] = -1;
    }
    for (i = 0; i < n; i++) {
        sBedCardSlot[i] = -1;
        if ((sBedCards[i].chunk >= 0) && (sBedCards[i].chunk < sScatter->width * sScatter->depth)) {
            sChunkCard[sBedCards[i].chunk] = i;
        }
    }
    for (i = 0; i < BEDCARD_CACHE; i++) {
        sBedSlotCard[i] = -1;
        sBedSlotUsed[i] = 0;
    }
    sBedCount = n;
}

/** The card's views in the cache (read from ROM now if there's a free slot and reads left this frame), or NULL. */
static u8* BedCard_Views(s32 card) {
    s32 slot = sBedCardSlot[card];
    s32 i;

    if (slot < 0) {
        u32 oldest = 0xFFFFFFFF;

        if (sBedLoads >= BEDCARD_LOADS) {
            return NULL;
        }
        // a free slot, or the one least lately drawn (not in the last two frames: the RSP may still be reading it)
        for (i = 0; i < BEDCARD_CACHE; i++) {
            if (sBedSlotCard[i] < 0) {
                slot = i;
                break;
            }
            if ((sBedFrame - sBedSlotUsed[i] > 2) && (sBedSlotUsed[i] < oldest)) {
                oldest = sBedSlotUsed[i];
                slot = i;
            }
        }
        if (slot < 0) {
            return NULL;
        }
        if (sBedSlotCard[slot] >= 0) {
            sBedCardSlot[sBedSlotCard[slot]] = -1;
        }
        DmaMgr_RequestSync(sBedCache + slot * FISHMOD_BEDCARD_BYTES, sBedRom + card * FISHMOD_BEDCARD_BYTES,
                           FISHMOD_BEDCARD_BYTES);
        sBedLoads++;
        sBedSlotCard[slot] = card;
        sBedCardSlot[card] = slot;
    }
    sBedSlotUsed[slot] = sBedFrame;
    return sBedCache + slot * FISHMOD_BEDCARD_BYTES;
}

void EnFishmod_ScatterUnload(void) {
    BedCard_Unload();
    if (sScatter != NULL) {
        ZeldaArena_Free(sScatter);
        sScatter = NULL;
    }
    sRockList = NULL;
    if (sChunkDry != NULL) {
        ZeldaArena_Free(sChunkDry);
        sChunkDry = NULL;
    }
    if (sChunkKindRow != NULL) {
        ZeldaArena_Free(sChunkKindRow);
        sChunkKindRow = NULL;
    }
    if (sChunkKindPool != NULL) {
        ZeldaArena_Free(sChunkKindPool);
        sChunkKindPool = NULL;
    }
    Scatter_FreeBuffers();
    if (sTouchSlot != NULL) {
        ZeldaArena_Free(sTouchSlot);
        sTouchSlot = NULL;
    }
    {
        s32 k;

        for (k = 0; k < FISHMOD_PROP_KIND_MAX; k++) {
            if (sBend[k] != NULL) {
                ZeldaArena_Free(sBend[k]);
                sBend[k] = NULL;
            }
            sBendTried[k] = false;
        }
    }
    if (sPropMtx != NULL) {
        ZeldaArena_Free(sPropMtx);
        sPropMtx = NULL;
    }
    if (sPropMtxMade != NULL) {
        ZeldaArena_Free(sPropMtxMade);
        sPropMtxMade = NULL;
    }
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
/**
 * A rock's stretch along its own x, y and z, from where it stands: every rock is the same vanilla model, and this
 * varies their shapes with no per-prop data. gen_scatter.py rock_stretch must match it (placement and collision).
 */
static void Scatter_RockStretch(FishmodProp* prop, f32* out) {
    u32 h = ((u32)(s32)prop->x * 0x9E3779B1u) ^ ((u32)(s32)prop->z * 0x85EBCA77u);

    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    out[0] = 0.75f + (h & 0xFF) * (0.6f / 255.0f);
    out[1] = 0.5f + ((h >> 8) & 0xFF) * (0.3f / 255.0f); // (flat: gen_scatter.py rock_stretch)
    out[2] = 0.75f + ((h >> 16) & 0xFF) * (0.6f / 255.0f);
}

/*
 * fishmod: with the Expansion Pak's room, each prop's matrix kept once made, for the props whose matrix depends on the
 * prop alone (all but the cards that turn toward the camera and the far single cards): a new set then points at it
 * instead of making it again. The same matrix, made by the same code (Scatter_MatrixInto), once per scene.
 */

static void Scatter_MatrixInto(Mtx* mtx, FishmodProp* prop, s32 flatScale, s16 yaw);

static Mtx* Scatter_Matrix(FishmodProp* prop, s32 flatScale, s16 yaw) {
    Mtx* mtx = &sBufs->set[sSet].mtx[sMtxUsed++];

    Scatter_MatrixInto(mtx, prop, flatScale, yaw);
    return mtx;
}

static void Scatter_MatrixInto(Mtx* mtx, FishmodProp* prop, s32 flatScale, s16 yaw) {
    f32 s = prop->scale * (65536.0f / 64.0f);
    s32 c = (s32)(Math_CosS(yaw) * s);
    s32 sn = (s32)(Math_SinS(yaw) * s);

    if (SCATTER_IS_ROCK(prop->kind) && (sRockList != NULL)) {
        // (a rock of its own: its mesh is made where it lies, in the world's directions: only placed)
        MTX_SET(mtx, 0, 0, 0x10000);
        MTX_SET(mtx, 0, 1, 0);
        MTX_SET(mtx, 0, 2, 0);
        MTX_SET(mtx, 0, 3, 0);
        MTX_SET(mtx, 1, 0, 0);
        MTX_SET(mtx, 1, 1, 0x10000);
        MTX_SET(mtx, 1, 2, 0);
        MTX_SET(mtx, 1, 3, 0);
        MTX_SET(mtx, 2, 0, 0);
        MTX_SET(mtx, 2, 1, 0);
        MTX_SET(mtx, 2, 2, 0x10000);
        MTX_SET(mtx, 2, 3, 0);
        mtx->intPart[3][0] = prop->x;
        mtx->intPart[3][1] = prop->y;
        mtx->intPart[3][2] = prop->z;
        mtx->intPart[3][3] = 1;
        mtx->fracPart[3][0] = 0;
        mtx->fracPart[3][1] = 0;
        mtx->fracPart[3][2] = 0;
        mtx->fracPart[3][3] = 0;
        return;
    }
    if ((prop->tiltX != 0) || (prop->tiltZ != 0) || (prop->kind == FISHMOD_PROP_ROCK)) {
        // lying on the bed (logs, rocks): tilted about its length and width before it is turned,
        // M = stretch * Rz * Rx * Ry * scale (only rocks are stretched)
        f32 k = prop->scale * (1.0f / 64.0f);
        f32 ct = Math_CosS(prop->tiltZ << 8);
        f32 st = Math_SinS(prop->tiltZ << 8);
        f32 cp = Math_CosS(prop->tiltX << 8);
        f32 sp = Math_SinS(prop->tiltX << 8);
        f32 cy = Math_CosS(yaw);
        f32 sy = Math_SinS(yaw);
        // Rz * Rx (rows: where the model's axes go before the turn)
        f32 r[3][3];
        f32 stretch[3] = { 1.0f, 1.0f, 1.0f };
        s32 i;

        r[0][0] = ct;
        r[0][1] = st * cp;
        r[0][2] = st * sp;
        r[1][0] = -st;
        r[1][1] = ct * cp;
        r[1][2] = ct * sp;
        r[2][0] = 0.0f;
        r[2][1] = -sp;
        r[2][2] = cp;
        if (prop->kind == FISHMOD_PROP_ROCK) {
            Scatter_RockStretch(prop, stretch);
        }
        for (i = 0; i < 3; i++) {
            r[i][0] *= stretch[i];
            r[i][1] *= stretch[i];
            r[i][2] *= stretch[i];
        }
        for (i = 0; i < 3; i++) {
            // then turned about y: (x, y, z) -> (x cy + z sy, y, -x sy + z cy)
            MTX_SET(mtx, i, 0, (s32)((r[i][0] * cy + r[i][2] * sy) * k * 65536.0f));
            MTX_SET(mtx, i, 1, (s32)(r[i][1] * k * 65536.0f));
            MTX_SET(mtx, i, 2, (s32)((-r[i][0] * sy + r[i][2] * cy) * k * 65536.0f));
            MTX_SET(mtx, i, 3, 0);
        }
        mtx->intPart[3][0] = prop->x;
        mtx->intPart[3][1] = prop->y;
        mtx->intPart[3][2] = prop->z;
        mtx->intPart[3][3] = 1;
        mtx->fracPart[3][0] = 0;
        mtx->fracPart[3][1] = 0;
        mtx->fracPart[3][2] = 0;
        mtx->fracPart[3][3] = 0;
        return;
    }

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
    return;
}

/**
 * Lake Hylia's bed under the full lake, shaded: darker under dense weed and lily pads and in deep water (shorter
 * normals take less of the sun: F3DEX2 doesn't normalise them). Written into the loaded room once; the drained lake
 * is left alone.
 */
static void Scatter_ShadeBed(PlayState* play) {
    u8* room = play->roomCtx.curRoom.segment;
    u32 last = 0;
    s32 i;

    f32 surface;
    WaterBox* box;

    // only the scene it was made for, with the water at the level it was made for (not a drained lake)
    if ((play->sceneId != FISHMOD_BEDSHADE_SCENE) || (room == NULL) || (room == sBedShadedRoom) ||
        !BgCheck_GetWaterSurface(play, &play->colCtx, FISHMOD_BEDSHADE_X, FISHMOD_BEDSHADE_Z, &surface, &box) ||
        (surface < FISHMOD_BEDSHADE_SURFACE - 20.0f)) {
        return;
    }
    for (i = 0; i < FISHMOD_BEDSHADE_COUNT; i++) {
        const FishmodBedShade* shade = &sFishmodBedShade[i];
        u32 offset = SEGMENT_OFFSET(shade->vtx) + offsetof(Vtx_tn, n);

        room[offset + 0] = shade->normal[0];
        room[offset + 1] = shade->normal[1];
        room[offset + 2] = shade->normal[2];
        last = MAX(last, offset + 3);
    }
    osWritebackDCache(room, last);
    sBedShadedRoom = room;
    {
        extern s32 gFishmodRelitDirty;

        gFishmodRelitDirty = true; // (the mesh pack's CPU lighting reads these normals: light the bed again)
    }
}

/** The scene's water levels in one number: it changes when any water box rises or falls (a lake refilling). */
static s32 Scatter_WaterLevels(PlayState* play) {
    WaterBox* boxes = SEGMENTED_TO_VIRTUAL(play->colCtx.colHeader->waterBoxes);
    s32 sum = 0;
    s32 i;

    for (i = 0; i < play->colCtx.colHeader->numWaterBoxes; i++) {
        sum = sum * 31 + boxes[i].ySurface + ((boxes[i].properties & WATERBOX_IS_DISABLED) ? 7 : 0);
    }
    return sum;
}

static void Scatter_UpdateDry(PlayState* play) {
    s32 levels = Scatter_WaterLevels(play);
    s32 c;

    Scatter_ShadeBed(play);
    if (sDryMade && (levels == sDryFor)) {
        return;
    }
    sDryMade = true;
    sDryFor = levels;
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
/*
 * 1-cycle props (F3DEX3 only). The 2-cycle way gave the combiner's second cycle the light (texel x vertex colour x
 * primitive) and the blender's first the fog: 2-cycle mode, half the RDP's pixel rate. Here the light is in the vertex
 * colour instead: the RSP lights each vertex with an ambient light of the prop's colour (F3DEX3's packed normals mode
 * multiplies the lighting by the vertex colour, so the vertex colours stay), the combiner takes texel x shade, and the
 * blender (forced, every pixel) the fog. The per-prop colour goes in the ambient light, not the primitive colour.
 */
#if ENABLE_F3DEX3
#define SCATTER_ONE_CYCLE() (!FISHMOD_BENCH_SKIP(FISHMOD_BENCH_TWOCYCLE))
#else
#define SCATTER_ONE_CYCLE() 0
#endif

static Lights* sSceneLights; // this frame's scene lights (1-cycle: put back for the lit materials and after the props)

#ifdef SCATTER_VERIFY
// (F3DEX3's gDPPipeSync, 0xE7, writes only its first word: the second is whatever the list held, and unused. The check
// skips it.)
static Gfx sCheckOpa[ARRAY_COUNT(((ScatterFrame*)0)->opa)];
static Gfx sCheckXlu[ARRAY_COUNT(((ScatterFrame*)0)->xlu)];

static s32 Scatter_CheckSame(Gfx* a, Gfx* b, s32 n, s32 list) {
    s32 i;

    for (i = 0; i < n; i++) {
        if ((a[i].words.w0 != b[i].words.w0) ||
            ((a[i].words.w1 != b[i].words.w1) && (((a[i].words.w0 >> 24) & 0xFF) != 0xE7))) {
            gFishmodScatterCacheDiff[0] = list;
            gFishmodScatterCacheDiff[1] = i;
            gFishmodScatterCacheDiff[2] = a[i].words.w0;
            gFishmodScatterCacheDiff[3] = a[i].words.w1;
            gFishmodScatterCacheDiff[4] = b[i].words.w0;
            gFishmodScatterCacheDiff[5] = b[i].words.w1;
            return false;
        }
    }
    return true;
}
#endif

/** Where the kept part ends (the first fading prop, or the end): noted if it was made, checked if verifying. */
/*
 * The props' water fog, with the camera over the lake's water: the same fog as the bed's (fishmod.c
 * Fishmod_WaterFogAmount: the camera-under-water fog over the length of the eye's ray inside the water), in the
 * props' colours, under the scene's own fog (left on, by distance, to the haze). A prop near the camera gets it vertex
 * by vertex: its model's commands copied with its vertices copied too, each one's colour blended toward the colour
 * that, times its texture's mean and the prop's light, is the water's (so a tall plant is clearer at its top than its
 * foot); a far one (a few pixels: no difference to see) by its middle, in its light. Above the water, nothing.
 * From the set's own eye (the kept commands hold while the set does).
 */
#define WATERFOG_VTX_MAX 3072                 // copied vertices a frame at most
#define WATERFOG_CMD_MAX (PROP_BUDGET * 3)    // and commands for them (the lists are sized for these)
#define WATERFOG_VERTEX_DIST 1100.0f          // vertex by vertex within this (across the ground)
#define WATERFOG_TPL_CMDS 128
typedef struct ScatterFogTpl {
    s16 n;       // commands (the model's, its end left off)
    s16 nv;      // vertices loaded, in all
    s16 nLoads;
    u8 vmean[3]; // the model's vertex colours' mean
    Gfx* cmds;
    u8* loadAt;  // each load's command
    Vtx** loadSrc;
} ScatterFogTpl;
static ScatterFogTpl* sFogTplKind[FISHMOD_PROP_KIND_MAX][2]; // (near model, far model)
static u8 sFogTplTried[FISHMOD_PROP_KIND_MAX][2];
static ScatterFogTpl** sFogTplRock; // [256]: each rock's
static u8* sFogTplRockTried;
static Vtx* sFogVtxPool[2];         // a frame buffer's each
static const u8 sKindMean[FISHMOD_PROP_KIND_MAX][3] = {
#define X(id, r, g, b) [FISHMOD_PROP_##id] = { r, g, b },
    FISHMOD_PROP_KINDS_MEAN(X)
#undef X
};
static struct {
    s32 on;
    Vec3f eye;
    f32 h;            // the eye's height over the water
    u8 water[3];
    u32 key;
    s32 vtxUsed;      // this frame's copies so far
    s32 cmdUsed;
} sPropFog;

static void Scatter_PropFogSetup(void) {
    s32 c;

    sPropFog.on = gFishmodWaterFogOn && (sSetEye.y > FISHMOD_LAKE_SURFACE) && (sFogVtxPool[0] != NULL);
    sPropFog.eye = sSetEye;
    sPropFog.h = sSetEye.y - FISHMOD_LAKE_SURFACE;
    for (c = 0; c < 3; c++) {
        sPropFog.water[c] = gFishmodWaterFogColor[c];
    }
    sPropFog.key = !sPropFog.on ? 0
                                : (0x80000000 | ((u32)sPropFog.water[0] << 16) | ((u32)sPropFog.water[1] << 8) |
                                   sPropFog.water[2]) ^ (u32)(gFishmodWaterFog.fo * 16.0f);
}

static void Scatter_FogAlloc(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (sFogVtxPool[i] == NULL) {
            sFogVtxPool[i] = ZeldaArena_Malloc(WATERFOG_VTX_MAX * sizeof(Vtx));
        }
    }
    if ((sFogVtxPool[0] == NULL) || (sFogVtxPool[1] == NULL)) {
        Scatter_FogFree(); // (no room: the props' water fog off, as everything else goes on)
        return;
    }
    if (sFogTplRock == NULL) {
        sFogTplRock = ZeldaArena_Malloc(256 * (sizeof(ScatterFogTpl*) + 1));
        if (sFogTplRock != NULL) {
            sFogTplRockTried = (u8*)(sFogTplRock + 256);
            bzero(sFogTplRock, 256 * (sizeof(ScatterFogTpl*) + 1));
        }
    }
}

/** (at a scene's end: the copies' sources, the reed's among them in object_fish, move with the next scene) */
static void Scatter_FogFree(void) {
    s32 i;
    s32 w;

    for (i = 0; i < 2; i++) {
        if (sFogVtxPool[i] != NULL) {
            ZeldaArena_Free(sFogVtxPool[i]);
            sFogVtxPool[i] = NULL;
        }
    }
    for (i = 0; i < FISHMOD_PROP_KIND_MAX; i++) {
        for (w = 0; w < 2; w++) {
            if (sFogTplKind[i][w] != NULL) {
                ZeldaArena_Free(sFogTplKind[i][w]);
                sFogTplKind[i][w] = NULL;
            }
            sFogTplTried[i][w] = false;
        }
    }
    if (sFogTplRock != NULL) {
        for (i = 0; i < 256; i++) {
            if (sFogTplRock[i] != NULL) {
                ZeldaArena_Free(sFogTplRock[i]);
            }
        }
        ZeldaArena_Free(sFogTplRock);
        sFogTplRock = NULL;
        sFogTplRockTried = NULL;
    }
}

/** A model's commands, its vertex loads noted (NULL if it isn't one this can copy: a branch, too long). */
static ScatterFogTpl* Scatter_FogTplBuild(Gfx* dlIn, u8* seg6) {
    u32* w = Scatter_BendAddr((u32)dlIn, seg6);
    ScatterFogTpl* tpl;
    s32 n;
    s32 loads = 0;
    s32 nv = 0;
    s32 i;
    u32 sum[3] = { 0, 0, 0 };

    if (w == NULL) {
        return NULL;
    }
    for (n = 0; n < WATERFOG_TPL_CMDS; n++) {
        u32 op = w[n * 2] >> 24;

        if (op == G_ENDDL) {
            break;
        }
        if ((op == G_DL) && (((w[n * 2] >> 16) & 0xFF) != G_DL_PUSH)) {
            return NULL; // (a branch: the rest isn't this list's)
        }
        if (op == G_VTX) {
            if (Scatter_BendAddr(w[n * 2 + 1], seg6) == NULL) {
                return NULL;
            }
            loads++;
            nv += (w[n * 2] >> 12) & 0xFF;
        }
    }
    if ((n == WATERFOG_TPL_CMDS) || (loads == 0) || (nv > 255)) {
        return NULL;
    }
    tpl = ZeldaArena_Malloc(sizeof(ScatterFogTpl) + n * sizeof(Gfx) + loads * (sizeof(Vtx*) + 1) + 8);
    if (tpl == NULL) {
        return NULL;
    }
    tpl->n = n;
    tpl->nv = nv;
    tpl->nLoads = loads;
    tpl->cmds = (Gfx*)(tpl + 1);
    tpl->loadSrc = (Vtx**)(tpl->cmds + n);
    tpl->loadAt = (u8*)(tpl->loadSrc + loads);
    bcopy(w, tpl->cmds, n * sizeof(Gfx));
    loads = 0;
    nv = 0;
    for (i = 0; i < n; i++) {
        u32* c = (u32*)&tpl->cmds[i];

        if ((c[0] >> 24) == G_VTX) {
            s32 cnt = (c[0] >> 12) & 0xFF;
            Vtx* v = Scatter_BendAddr(c[1], seg6);
            s32 k;

            tpl->loadAt[loads] = i;
            tpl->loadSrc[loads++] = v;
            for (k = 0; k < cnt; k++) {
                sum[0] += v[k].v.cn[0];
                sum[1] += v[k].v.cn[1];
                sum[2] += v[k].v.cn[2];
            }
            nv += cnt;
        }
    }
    for (i = 0; i < 3; i++) {
        tpl->vmean[i] = CLAMP(sum[i] / nv, 16, 255);
    }
    return tpl;
}

/** How much of the water's fog is at a point (0-1), with the water's surface over it at surf. */
static f32 Scatter_FogAt(f32 x, f32 y, f32 z, s32 surf) {
    f32 depth = surf - y;
    f32 dx, dy, dz;

    if (depth <= 0.0f) {
        return 0.0f;
    }
    dx = x - sPropFog.eye.x;
    dy = y - sPropFog.eye.y;
    dz = z - sPropFog.eye.z;
    return Fishmod_WaterFogAmount(sqrtf(SQ(dx) + SQ(dy) + SQ(dz)) * depth / (sPropFog.h + depth));
}

/** The prop's surface (its chunk's), or -0x8000: none, or its fog's off. */
static s32 Scatter_FogSurface(FishmodProp* prop, FishmodPropKindInfo* info) {
    s32 cx;
    s32 cz;

    if (!sPropFog.on || info->floating || info->xlu) {
        return -0x8000;
    }
    cx = (s32)((prop->x - sScatter->x0) / sScatter->chunk);
    cz = (s32)((prop->z - sScatter->z0) / sScatter->chunk);
    if ((cx < 0) || (cz < 0) || (cx >= sScatter->width) || (cz >= sScatter->depth)) {
        return -0x8000;
    }
    return sChunkSurface[cz * sScatter->width + cx];
}

/** The colour that, times this mean (of the texture, and of the vertex colours or the prop's light), is the water's. */
static u32 Scatter_FogTarget(s32 c, u32 mean1, u32 mean2) {
    u32 t = (u32)sPropFog.water[c] * 255 * 255 / (MAX(mean1, 1) * MAX(mean2, 1));

    return CLAMP_MAX(t, 255);
}

/** The prop's model as commands of its own, its vertices copied, each coloured by its water fog (NULL: no room). */
static Gfx* Scatter_FogEmit(Gfx* gfx, ScatterFogTpl* tpl, FishmodProp* prop, s32 kind, s32 surf, u32 r, u32 g,
                            u32 b) {
    Vtx* out;
    f32 k = SCATTER_IS_ROCK(kind) ? 1.0f : prop->scale * (1.0f / 64.0f);
    f32 c = SCATTER_IS_ROCK(kind) ? 1.0f : Math_CosS(prop->yaw << 8);
    f32 sn = SCATTER_IS_ROCK(kind) ? 0.0f : Math_SinS(prop->yaw << 8);
    u32 target[3];
    s32 i;
    s32 at = 0;
    s32 load = 0;

    if ((sPropFog.vtxUsed + tpl->nv > WATERFOG_VTX_MAX) || (sPropFog.cmdUsed + tpl->n > WATERFOG_CMD_MAX)) {
        return NULL;
    }
    out = &sFogVtxPool[sFrame][sPropFog.vtxUsed];
    sPropFog.vtxUsed += tpl->nv;
    sPropFog.cmdUsed += tpl->n;
    target[0] = Scatter_FogTarget(0, sKindMean[kind][0], r);
    target[1] = Scatter_FogTarget(1, sKindMean[kind][1], g);
    target[2] = Scatter_FogTarget(2, sKindMean[kind][2], b);
    for (i = 0; i < tpl->n; i++) {
        if ((load < tpl->nLoads) && (tpl->loadAt[load] == i)) {
            u32 w0 = ((u32*)&tpl->cmds[i])[0];
            s32 cnt = (w0 >> 12) & 0xFF;
            Vtx* src = tpl->loadSrc[load++];
            s32 v;

            for (v = 0; v < cnt; v++) {
                Vtx* o = &out[at + v];
                f32 lx = src[v].v.ob[0] * k;
                f32 lz = src[v].v.ob[2] * k;
                // (turned as the prop's matrix turns it: x to (c, -sn), z to (sn, c))
                f32 a = Scatter_FogAt(prop->x + lx * c + lz * sn, prop->y + src[v].v.ob[1] * k,
                                      prop->z - lx * sn + lz * c, surf);
                s32 ch;

                *o = src[v];
                if (a > 0.0f) {
                    s32 ai = (s32)(a * 256.0f);

                    for (ch = 0; ch < 3; ch++) {
                        o->v.cn[ch] = src[v].v.cn[ch] + ((((s32)target[ch] - src[v].v.cn[ch]) * ai) >> 8);
                    }
                }
            }
            ((u32*)gfx)[0] = w0;
            ((u32*)gfx)[1] = (u32)&out[at];
            gfx++;
            at += cnt;
        } else {
            *gfx++ = tpl->cmds[i];
        }
    }
    return gfx;
}

static void Scatter_KeepAt(ScatterKeep* keep, s32 kept, Gfx* opa0, Gfx* opa, Gfx* xlu0, Gfx* xlu, ScatterKeep* now,
                           s32 lastOpa, s32 lastXlu, s32 lastNear) {
    if (!kept) {
        *keep = *now;
        keep->opaLen = (opa - opa0) + 1; // (+1: 0 is none kept)
        keep->xluLen = xlu - xlu0;
        keep->lastOpa = lastOpa;
        keep->lastXlu = lastXlu;
        keep->lastNear = lastNear;
        keep->edgeNear = sEdgeNear;
        keep->fogVtxUsed = sPropFog.vtxUsed;
        keep->fogCmdUsed = sPropFog.cmdUsed;
        return;
    }
#ifdef SCATTER_VERIFY
    if (((opa - opa0) != keep->opaLen - 1) || ((xlu - xlu0) != keep->xluLen) || (lastOpa != keep->lastOpa) ||
        (lastXlu != keep->lastXlu) || (lastNear != keep->lastNear) || (sEdgeNear != keep->edgeNear)) {
        gFishmodScatterCacheDiff[0] = 3; // the lengths or the loop's state
        gFishmodScatterCacheMismatch++;
    } else if (!Scatter_CheckSame(opa0, sCheckOpa, opa - opa0, 1) || !Scatter_CheckSame(xlu0, sCheckXlu, xlu - xlu0, 2)) {
        gFishmodScatterCacheMismatch++;
    }
#endif
}

/** The scene's lights again, as Lights_Draw sets them (the 1-cycle props' ambient-only light replaced them). */
static Gfx* Scatter_SceneLights(Gfx* gfx) {
#if ENABLE_F3DEX3
    if (sSceneLights != NULL) {
        gSPSetLights(gfx++, sSceneLights->numLights, sSceneLights->l);
    }
#endif
    return gfx;
}

/** The prop's colour (r, g, b) and alpha: the ambient light in 1-cycle mode, the primitive colour in 2-cycle. */
static Gfx* Scatter_PropColor(Gfx* gfx, s32 tier, u32 r, u32 g, u32 b, u32 a) {
#if ENABLE_F3DEX3
    if (SCATTER_ONE_CYCLE()) {
        gSPLightColor(gfx++, LIGHT_1, (r << 24) | (g << 16) | (b << 8));
        if (tier == TIER_IMP) {
            gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, a); // (its alpha: the dissolve)
        }
        return gfx;
    }
#endif
    gDPSetPrimColor(gfx++, 0, 0, r, g, b, a);
    return gfx;
}

static Gfx* Scatter_Material(Gfx* gfx, FishmodPropKindInfo* info, s32 kind, s32 tier) {
    gfx = Gfx_SetupDL(gfx, SETUPDL_25);
    if (info->xlu || (kind == FISHMOD_PROP_LILY_PAD)) {
        // vanilla's own materials (lit by the scene's lights, like the Fishing Pond's props)
        if (SCATTER_ONE_CYCLE()) {
            gfx = Scatter_SceneLights(gfx);
        }
        gSPDisplayList(gfx++, info->material);
        return gfx;
    }
    gDPPipeSync(gfx++);
    gDPSetTextureLUT(gfx++, G_TT_NONE);
    {
        // The cut-out edge. Anti-aliased in the RDP (AA_ZB_TEX_EDGE) reads the colour buffer for every pixel;
        // without that read (as libultra's "RA" modes), the coverage is still written and the VI smooths the edges.
        s32 ra = PROP_EDGE_RA && sEdgeNear;
        u32 mode;

#if FISHMOD_DEBUG
        if (gFishmodBench & FISHMOD_BENCH_ALTEDGE) {
            ra = false; // the old way: every prop anti-aliased in the RDP
        }
#endif
        mode = ra ? (AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP | CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE)
                  : (AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP | CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE);
#if ENABLE_F3DEX3
        if (SCATTER_ONE_CYCLE()) {
            // the fog blended into every pixel (forced: no colour-buffer read, as the RA mode); the coverage still
            // written for the VI to smooth the cut-out edges
            u32 mode1 = AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP | CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE | FORCE_BL;

            gDPSetCycleType(gfx++, G_CYC_1CYCLE);
            if (tier == TIER_IMP) {
                gDPSetCombineLERP(gfx++, TEXEL0, 0, SHADE, 0, TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, SHADE, 0, TEXEL0, 0,
                                  PRIMITIVE, 0);
                gDPSetAlphaCompare(gfx++, G_AC_DITHER);
            } else {
                gDPSetCombineLERP(gfx++, TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0);
            }
            gDPSetRenderMode(gfx++, mode1 | GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA),
                             mode1 | GBL_c2(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA));
        } else
#endif
        {
            if (tier == TIER_IMP) {
                // the single card: its alpha times the primitive's (how far it has faded), against noise
                gDPSetCombineLERP(gfx++, TEXEL0, 0, SHADE, 0, TEXEL0, 0, PRIMITIVE, 0, COMBINED, 0, PRIMITIVE, 0, 0,
                                  0, 0, COMBINED);
            } else {
                gDPSetCombineLERP(gfx++, TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0, COMBINED, 0, PRIMITIVE, 0, 0, 0, 0,
                                  COMBINED);
            }
            gDPSetRenderMode(gfx++, G_RM_FOG_SHADE_A, mode | GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM));
            if (tier == TIER_IMP) {
                gDPSetAlphaCompare(gfx++, G_AC_DITHER);
            }
        }
    }
    gSPClearGeometryMode(gfx++, G_CULL_BACK | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gSPSetGeometryMode(gfx++, G_FOG | G_SHADE | G_SHADING_SMOOTH);
#if ENABLE_F3DEX3
    if (SCATTER_ONE_CYCLE()) {
        // lit by the ambient light alone (its colour: the prop's), times the vertex colour (packed normals mode)
        gSPSetGeometryMode(gfx++, G_LIGHTING | G_PACKED_NORMALS);
        gSPNumLights(gfx++, 0);
    }
#endif
    if (SCATTER_IS_ROCK(kind) || (kind == FISHMOD_PROP_DOCK_POSTS) || (kind == FISHMOD_PROP_DOCK_DECK)) {
        // (solid: their far sides cost nothing. The dock's posts are tall and thin: their hidden back faces cost the
        // RDP a scanline each all the way down, as much as the faces in front)
        gSPSetGeometryMode(gfx++, G_CULL_BACK);
    }
#if ENABLE_F3DEX3
    if ((kind == FISHMOD_PROP_ROCK_LIT) && SCATTER_ONE_CYCLE() && (sRockList != NULL) && (sRockScroll != NULL)) {
        // a rock in the shallows: the bed's caustics on it as light (2-cycle: the rock's texture, plus the caustic
        // light less its own average, times a strength: brighter under the bright lines, a little darker
        // between. The bed's blend toward the caustic texture shows on its light sand, but only tints a dark rock.)
        // The strength is each rock's step of the fade (its list calls sRockStep's: the environment colour); then the
        // props' light and the vertex colours, as the others
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_2CYCLE);
        gDPSetCombineLERP(gfx++, TEXEL0, ENVIRONMENT, ENV_ALPHA, TEXEL1, 0, 0, 0, 1, COMBINED, 0, SHADE, 0, 0, 0, 0,
                          COMBINED);
        gDPSetRenderMode(gfx++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);
        gSPDisplayList(gfx++, sRockLitMaterial);
        gSPDisplayList(gfx++, sRockScroll);
        return gfx;
    }
#endif
    if (tier == TIER_IMP) {
        gSPDisplayList(gfx++, info->materialImp);
    } else if (tier == TIER_HALF) {
        gSPDisplayList(gfx++, info->materialHalf);
    } else {
        gSPDisplayList(gfx++, info->material);
    }
    return gfx;
}

/**
 * The shallow rocks' caustics for this frame: scrolled as the bed's (Scene_DrawConfigLakeHylia: a quarter texel a
 * frame), and the fade's steps from the scene's own colour (255 when the lake is drained: no caustics).
 */
static void Scatter_RockCaustics(PlayState* play, ScatterFrame* frame) {
    u32 t = (u32)play->gameplayFrames % (512 << 2);
    Gfx* g = frame->rockScroll;
    s32 base = (play->sceneId == SCENE_LAKE_HYLIA) ? (play->roomCtx.drawParams[0] + 168) : 255;

    gDPTileSync(g++);
    gDPSetTileSize(g++, G_TX_RENDERTILE, t, t, t + (31 << 2), t + (31 << 2));
    gDPTileSync(g++);
    gDPSetTileSize(g++, 1, 0, 0, 31 << 2, 31 << 2);
    gSPEndDisplayList(g++);
    sRockScroll = frame->rockScroll;
    base = CLAMP(base, 0, 255);
    if (base != sRockStepBase) {
        // (only when it changes: the RSP may be reading them)
        s32 i;

        sRockStepBase = base;
        for (i = 0; i < ROCK_STEPS; i++) {
            // (the bed's step: its share of the caustics, 255 less its alpha, made the light's strength; the colour
            // the caustic light's average brightness, so the light adds nothing on the whole)
            s32 bed = base + (255 - base) * i / ROCK_STEPS;

            gDPSetEnvColor(&sRockStep[i][0], 103, 103, 103, CLAMP_MAX((255 - bed) * ROCK_LIGHT / 16, 255));
            gSPEndDisplayList(&sRockStep[i][1]);
        }
    }
}

/*
 * fish-oot: plants make way for Link, as the game's own grass and the Fishing Pond's reeds and lily pads do. A plant
 * he walks or swims into leans away from him (a shear: its foot stays, its top goes), and a floating one (lily pads,
 * their flowers, floating leaves) drifts aside; both ease back once he has gone. A few at a time (TOUCH_MAX), each
 * with a matrix of this frame's (ScatterFrame.bend) multiplied in after its own, in its own frame (before its turn).
 */

/** A floating prop's middle: the lily pad's leaf is off its model's origin (gen_scatter.py float_space). */
static void Scatter_TouchMiddle(FishmodProp* prop, f32* x, f32* z) {
    *x = prop->x;
    *z = prop->z;
    if (prop->kind == FISHMOD_PROP_LILY_PAD) {
        f32 k = prop->scale * (1.0f / 64.0f);
        f32 c = Math_CosS(prop->yaw << 8);
        f32 sn = Math_SinS(prop->yaw << 8);

        // (the model's (-8, 5), turned as Scatter_MatrixInto turns it)
        *x += (-8.0f * c + 5.0f * sn) * k;
        *z += (8.0f * sn + 5.0f * c) * k;
    }
}

/** Which props Link touches this frame, and how far each has gone toward making way. */
static void Scatter_Touch(PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 px;
    f32 py;
    f32 pz;
    s32 cx;
    s32 cz;
    s32 i;

    if (sTouchSlot == NULL) {
        return;
    }
    for (i = 0; i < TOUCH_MAX; i++) {
        sTouch[i].hit = false;
        sTouch[i].tx = sTouch[i].tz = 0.0f;
    }
    if (player != NULL) {
        px = player->actor.world.pos.x;
        py = player->actor.world.pos.y;
        pz = player->actor.world.pos.z;
#if FISHMOD_DEBUG
        if (gFishmodTouchTest[4] > 0.0f) {
            // (a script's test, FISHMOD_TOUCH_TEST: a point swept to and fro through a bed along x instead of Link)
            s16 ang = (s16)(play->gameplayFrames * (s32)(65536.0f / gFishmodTouchTest[4]));

            px = gFishmodTouchTest[0] + Math_SinS(ang) * gFishmodTouchTest[3];
            py = gFishmodTouchTest[1];
            pz = gFishmodTouchTest[2];
        }
#endif
        cx = (s32)((px - sScatter->x0) / sScatter->chunk);
        cz = (s32)((pz - sScatter->z0) / sScatter->chunk);
        for (i = 0; i < 9; i++) {
            s32 gx = cx + (i % 3) - 1;
            s32 gz = cz + (i / 3) - 1;
            s32 c;
            s32 n;

            if ((gx < 0) || (gz < 0) || (gx >= sScatter->width) || (gz >= sScatter->depth)) {
                continue;
            }
            c = gz * sScatter->width + gx;
            if ((sChunkStart[c] == sChunkStart[c + 1]) ||
                (SQ(px - (sScatter->x0 + (gx + 0.5f) * sScatter->chunk)) +
                     SQ(pz - (sScatter->z0 + (gz + 0.5f) * sScatter->chunk)) >
                 SQ(sScatter->chunk * 0.7072f + 60.0f))) {
                continue; // (empty, or no nearer to Link than the reach of its corner)
            }
            for (n = sChunkStart[c]; n < sChunkStart[c + 1]; n++) {
                FishmodProp* prop = &sProps[n];

                if (!sTouchable[prop->kind]) {
                    // (kinds that don't make way, as rocks, logs, the dock: past them all at once, the props being
                    // in kind order)
                    if (sChunkKindRow != NULL) {
                        n = sChunkKindPool[sChunkKindRow[c] * (FISHMOD_PROP_KIND_MAX + 1) + prop->kind + 1] - 1;
                    }
                    continue;
                }
                FishmodPropKindInfo* info = &sKinds[prop->kind];
                f32 k = prop->scale * (1.0f / 64.0f);
                f32 mx;
                f32 mz;
                f32 dx;
                f32 dz;
                f32 r;
                f32 d;
                f32 amount;
                s32 slot;

                if (TOUCH_HANGS(prop->kind)) {
                    // (hanging down from the surface: Link anywhere along it)
                    if ((py > prop->y + 10.0f) || (py + 50.0f < prop->y - TOUCH_HANG * k)) {
                        continue;
                    }
                    r = 14.0f + CLAMP_MAX(TOUCH_HANG * k * 0.3f, 30.0f);
                } else if (info->floating) {
                    // (on the surface: Link swimming, or wading where it's shallow; the bigger, the further it reaches)
                    if (fabsf(py - prop->y) > 40.0f) {
                        continue;
                    }
                    r = TOUCH_FLOAT_R0 + TOUCH_FLOAT_R * k;
                } else if (info->plant && (info->sway != 0)) {
                    f32 top = prop->y + info->extent * k;

                    // (his feet to his head against its foot to its top; hanging ones reach down instead)
                    if ((py > MAX(top, prop->y) + 5.0f) || (py + 50.0f < MIN(top, prop->y))) {
                        continue;
                    }
                    r = 14.0f + CLAMP_MAX(info->height * k * 0.3f, 30.0f);
                } else {
                    continue;
                }
                Scatter_TouchMiddle(prop, &mx, &mz);
                dx = mx - px;
                dz = mz - pz;
                if ((fabsf(dx) > r) || (fabsf(dz) > r)) {
                    continue;
                }
                d = sqrtf(SQ(dx) + SQ(dz));
                if (d >= r) {
                    continue;
                }
                slot = sTouchSlot[n];
                if (slot == TOUCH_NONE) {
                    for (slot = 0; (slot < TOUCH_MAX) && sTouch[slot].used; slot++) {}
                    if (slot == TOUCH_MAX) {
                        continue; // (a few at a time)
                    }
                    sTouch[slot].used = true;
                    sTouch[slot].prop = n;
                    sTouch[slot].x = sTouch[slot].z = 0.0f;
                    sTouch[slot].vx = sTouch[slot].vz = 0.0f;
                    sTouchSlot[n] = slot;
                    sTouchGen++;
                }
                if (d < 1.0f) {
                    // (right on it: away the way Link faces)
                    dx = Math_SinS(player->actor.shape.rot.y);
                    dz = Math_CosS(player->actor.shape.rot.y);
                    d = 1.0f;
                }
                amount = (r - d) / r;
                amount = TOUCH_DRIFTS(prop->kind) ? (amount * TOUCH_PUSH) : (amount * TOUCH_LEAN);
                sTouch[slot].tx = dx / d * amount;
                sTouch[slot].tz = dz / d * amount;
                sTouch[slot].hit = true;
            }
        }
    }
    for (i = 0; i < TOUCH_MAX; i++) {
        ScatterTouch* t = &sTouch[i];

        if (!t->used) {
            continue;
        }
        if (t->hit) {
            t->x += (t->tx - t->x) * TOUCH_EASE;
            t->z += (t->tz - t->z) * TOUCH_EASE;
            t->vx = t->vz = 0.0f;
        } else if (TOUCH_DRIFTS(sProps[t->prop].kind)) {
            t->x -= t->x * TOUCH_BACK;
            t->z -= t->z * TOUCH_BACK;
        } else {
            // a spring with a little damping: up, a little past, back
            t->vx += -t->x * TOUCH_SPRING - t->vx * TOUCH_DAMP;
            t->vz += -t->z * TOUCH_SPRING - t->vz * TOUCH_DAMP;
            t->x += t->vx;
            t->z += t->vz;
        }
        if (!t->hit &&
            (SQ(t->x) + SQ(t->z) < SQ(TOUCH_DRIFTS(sProps[t->prop].kind) ? 0.3f : 0.01f)) &&
            (SQ(t->vx) + SQ(t->vz) < SQ(0.005f))) {
            t->used = false;
            sTouchSlot[t->prop] = TOUCH_NONE;
            sTouchGen++;
        }
    }
}

/**
 * The touched props' matrices for this frame, in each prop's own frame (they're multiplied in before its turn and
 * scale): a lean (x += a y, z += b y) or a drift (a move of its own, along the water).
 */
static void Scatter_TouchMatrices(ScatterFrame* frame) {
    s32 i;

    for (i = 0; i < TOUCH_MAX; i++) {
        ScatterTouch* t = &sTouch[i];
        FishmodProp* prop;
        Mtx* mtx;
        f32 c;
        f32 sn;
        f32 a;
        f32 b;

        if (!t->used) {
            continue;
        }
        prop = &sProps[t->prop];
        mtx = &frame->bend[i];
        // the world's direction in the prop's own frame (Scatter_MatrixInto turns x to (c, -sn), z to (sn, c))
        c = Math_CosS(prop->yaw << 8);
        sn = Math_SinS(prop->yaw << 8);
        a = t->x * c - t->z * sn;
        b = t->x * sn + t->z * c;
        MTX_SET(mtx, 0, 0, 0x10000);
        MTX_SET(mtx, 0, 1, 0);
        MTX_SET(mtx, 0, 2, 0);
        MTX_SET(mtx, 0, 3, 0);
        MTX_SET(mtx, 1, 1, 0x10000);
        MTX_SET(mtx, 1, 3, 0);
        MTX_SET(mtx, 2, 0, 0);
        MTX_SET(mtx, 2, 1, 0);
        MTX_SET(mtx, 2, 2, 0x10000);
        MTX_SET(mtx, 2, 3, 0);
        MTX_SET(mtx, 3, 3, 0x10000);
        if (TOUCH_DRIFTS(prop->kind)) {
            // a move, in its own units (its scale undone)
            f32 k = 64.0f / prop->scale;

            MTX_SET(mtx, 1, 0, 0);
            MTX_SET(mtx, 1, 2, 0);
            MTX_SET(mtx, 3, 0, (s32)(a * k * 65536.0f));
            MTX_SET(mtx, 3, 1, 0);
            MTX_SET(mtx, 3, 2, (s32)(b * k * 65536.0f));
        } else {
            s32 k;

            MTX_SET(mtx, 1, 0, (s32)(a * 65536.0f));
            MTX_SET(mtx, 1, 2, (s32)(b * 65536.0f));
            MTX_SET(mtx, 3, 0, 0);
            MTX_SET(mtx, 3, 1, 0);
            MTX_SET(mtx, 3, 2, 0);
            // the bands' (Scatter_Bend): each a share of the lean by its height, so a vertex at height y moves about
            // lean * y * (y / height): the stalk curves, its foot stays put, its top goes as far as the whole lean
            for (k = 0; k < BEND_BANDS; k++) {
                Mtx* m = &frame->bendBand[i][k];
                // (a hanging plant held at its top: the bottom band leans most, and its height is below the
                // surface, so the lean's sign turns)
                // (a rooted plant, held up by the water: bent most at its foot, its top turning back up toward
                // upright: a vertex at height t (of the whole) moves lean * t * (2 - t), steepest at the foot, level
                // at the top; shortened as it leans, so the stalk keeps its length rather than stretching)
                f32 t = (k + 0.5f) / BEND_BANDS;
                f32 f = TOUCH_HANGS(prop->kind) ? (BEND_BANDS - k - 0.5f) * (-1.15f / BEND_BANDS) : (2.0f - t);
                f32 sh = SQ(a * f) + SQ(b * f);

                *m = *mtx;
                MTX_SET(m, 1, 0, (s32)(a * f * 65536.0f));
                MTX_SET(m, 1, 1, (s32)(65536.0f / sqrtf(1.0f + sh)));
                MTX_SET(m, 1, 2, (s32)(b * f * 65536.0f));
            }
        }
    }
}

/**
 * A kind's model cut into BEND_BANDS height bands (its triangles clipped at the bands' heights, their corners
 * interpolated), the vertices band by band: loaded each under its own band's lean and then all the triangles drawn,
 * each vertex is moved once, so nothing parts. NULL if the model isn't one this reads (geometry only), or its
 * vertices don't all fit the vertex buffer at once (the plant then leans whole).
 */
#define BEND_TRI_MAX 160
typedef struct BendV {
    f32 p[3];
    f32 st[2];
    f32 c[4];
} BendV;

static void Bend_Lerp(BendV* out, BendV* a, BendV* b, f32 f) {
    s32 i;

    for (i = 0; i < 3; i++) {
        out->p[i] = a->p[i] + (b->p[i] - a->p[i]) * f;
    }
    for (i = 0; i < 2; i++) {
        out->st[i] = a->st[i] + (b->st[i] - a->st[i]) * f;
    }
    for (i = 0; i < 4; i++) {
        out->c[i] = a->c[i] + (b->c[i] - a->c[i]) * f;
    }
}

/** the part of polygon in (n corners) on the side y >= h (keep > 0) or y <= h (keep < 0) */
static s32 Bend_Clip(BendV* in, s32 n, BendV* out, f32 h, s32 keep) {
    s32 m = 0;
    s32 i;

    for (i = 0; i < n; i++) {
        BendV* a = &in[i];
        BendV* b = &in[(i + 1) % n];
        s32 ia = (a->p[1] - h) * keep >= 0.0f;
        s32 ib = (b->p[1] - h) * keep >= 0.0f;

        if (ia) {
            out[m++] = *a;
        }
        if (ia != ib) {
            Bend_Lerp(&out[m++], a, b, (h - a->p[1]) / (b->p[1] - a->p[1]));
        }
    }
    return m;
}

/** A model's address the CPU can read: as it is (in RAM), or in segment 6 (object_fish's: the Fishing Pond's reed)
 * at where that object is loaded (seg6); NULL for any other. */
static void* Scatter_BendAddr(u32 a, u8* seg6) {
    if ((a >> 24) == 0x80) {
        return (void*)a;
    }
    if (((a >> 24) == 0x06) && (seg6 != NULL)) {
        return seg6 + (a & 0xFFFFFF);
    }
    return NULL;
}

static ScatterBend* Scatter_BendBuild(Gfx* modelIn, u8* seg6) {
    Gfx* model = Scatter_BendAddr((u32)modelIn, seg6);
    static BendV sTri[BEND_TRI_MAX][3];
    static Vtx sOut[64];
    static u8 sOutBand[64];
    static u8 sIdx[BEND_TRI_MAX * 3][3];
    Vtx* slot[64];
    u32* w = (u32*)model;
    s32 nTri = 0;
    s32 nOut = 0;
    s32 nIdx = 0;
    f32 y0 = 1e9f;
    f32 y1 = -1e9f;
    s32 i;
    s32 k;
    s32 n;
    ScatterBend* b;

    if (model == NULL) {
        // (an address the CPU can't read: reading the reed's segment address as it was is what crashed, a TLB
        // exception on 0x06...; such a plant leans whole)
        return NULL;
    }
    bzero(slot, sizeof(slot));
    // the model's triangles (a list of vertex loads and triangles)
    for (n = 0; n < 1024; n++, w += 2) {
        u32 op = w[0] >> 24;

        if (op == G_ENDDL) {
            break;
        } else if (op == G_VTX) {
            s32 cnt = (w[0] >> 12) & 0xFF;
            s32 v0 = ((w[0] & 0xFF) >> 1) - cnt;
            Vtx* v = Scatter_BendAddr(w[1], seg6);

            if ((v0 < 0) || (v0 + cnt > 64) || (v == NULL)) {
                return NULL;
            }
            for (i = 0; i < cnt; i++) {
                slot[v0 + i] = &v[i];
            }
        } else if ((op == G_TRI1) || (op == G_TRI2)) {
            s32 t;

            for (t = 0; t < ((op == G_TRI2) ? 2 : 1); t++) {
                u32 x = (t == 0) ? w[0] : w[1];
                s32 c;

                if (nTri >= BEND_TRI_MAX) {
                    return NULL;
                }
                for (c = 0; c < 3; c++) {
                    Vtx* v = slot[((x >> (16 - 8 * c)) & 0xFF) >> 1];
                    BendV* o = &sTri[nTri][c];

                    if (v == NULL) {
                        return NULL;
                    }
                    o->p[0] = v->v.ob[0];
                    o->p[1] = v->v.ob[1];
                    o->p[2] = v->v.ob[2];
                    o->st[0] = v->v.tc[0];
                    o->st[1] = v->v.tc[1];
                    o->c[0] = v->v.cn[0];
                    o->c[1] = v->v.cn[1];
                    o->c[2] = v->v.cn[2];
                    o->c[3] = v->v.cn[3];
                    y0 = MIN(y0, o->p[1]);
                    y1 = MAX(y1, o->p[1]);
                }
                nTri++;
            }
        } else if ((op != G_RDPPIPESYNC) && (op != G_CULLDL) && (op != G_NOOP)) {
            return NULL; // (not geometry alone)
        }
    }
    if ((nTri == 0) || (y1 - y0 < 1.0f)) {
        return NULL;
    }
    // each triangle cut at the bands' heights; the pieces' corners, the same ones merged
    for (i = 0; i < nTri; i++) {
        for (k = 0; k < BEND_BANDS; k++) {
            BendV a[8];
            BendV c[8];
            f32 lo = y0 + (y1 - y0) * k / BEND_BANDS;
            f32 hi = y0 + (y1 - y0) * (k + 1) / BEND_BANDS;
            s32 m = Bend_Clip(sTri[i], 3, a, lo, 1);
            s32 j;
            u8 ids[8];

            m = Bend_Clip(a, m, c, hi, -1);
            if (m < 3) {
                continue;
            }
            for (j = 0; j < m; j++) {
                Vtx v;
                s32 q;

                v.v.ob[0] = (s16)(c[j].p[0] + ((c[j].p[0] < 0.0f) ? -0.5f : 0.5f));
                v.v.ob[1] = (s16)(c[j].p[1] + ((c[j].p[1] < 0.0f) ? -0.5f : 0.5f));
                v.v.ob[2] = (s16)(c[j].p[2] + ((c[j].p[2] < 0.0f) ? -0.5f : 0.5f));
                v.v.flag = 0;
                v.v.tc[0] = (s16)(c[j].st[0] + ((c[j].st[0] < 0.0f) ? -0.5f : 0.5f));
                v.v.tc[1] = (s16)(c[j].st[1] + ((c[j].st[1] < 0.0f) ? -0.5f : 0.5f));
                v.v.cn[0] = (u8)(c[j].c[0] + 0.5f);
                v.v.cn[1] = (u8)(c[j].c[1] + 0.5f);
                v.v.cn[2] = (u8)(c[j].c[2] + 0.5f);
                v.v.cn[3] = (u8)(c[j].c[3] + 0.5f);
                for (q = 0; q < nOut; q++) {
                    if (bcmp(&sOut[q], &v, sizeof(Vtx)) == 0) {
                        break;
                    }
                }
                if (q == nOut) {
                    if (nOut >= BEND_VTX_MAX) {
                        return NULL;
                    }
                    sOut[nOut] = v;
                    // (its band by its height: a corner on a band's edge belongs to the one above)
                    sOutBand[nOut] = CLAMP((s32)((v.v.ob[1] - y0) * BEND_BANDS / (y1 - y0)), 0, BEND_BANDS - 1);
                    nOut++;
                }
                ids[j] = q;
            }
            for (j = 1; j < m - 1; j++) {
                if (nIdx >= BEND_TRI_MAX * 3) {
                    return NULL;
                }
                sIdx[nIdx][0] = ids[0];
                sIdx[nIdx][1] = ids[j];
                sIdx[nIdx][2] = ids[j + 1];
                nIdx++;
            }
        }
    }
    b = ZeldaArena_Malloc(sizeof(ScatterBend) + nOut * sizeof(Vtx) + ((nIdx + 1) / 2 + 1) * sizeof(Gfx) + 16);
    if (b == NULL) {
        return NULL;
    }
    {
        // the vertices band by band; the triangles in those numbers
        u8 at[64];
        s32 o = 0;
        Gfx* g;

        b->vtx = (Vtx*)ALIGN16((u32)(b + 1));
        for (k = 0; k < BEND_BANDS; k++) {
            b->start[k] = o;
            for (i = 0; i < nOut; i++) {
                if (sOutBand[i] == k) {
                    at[i] = o;
                    b->vtx[o++] = sOut[i];
                }
            }
        }
        b->start[BEND_BANDS] = o;
        g = b->tris = (Gfx*)(b->vtx + nOut);
        for (i = 0; i < nIdx; i += 2) {
            if (i + 1 < nIdx) {
                gSP2Triangles(g++, at[sIdx[i][0]], at[sIdx[i][1]], at[sIdx[i][2]], 0, at[sIdx[i + 1][0]],
                              at[sIdx[i + 1][1]], at[sIdx[i + 1][2]], 0);
            } else {
                gSP1Triangle(g++, at[sIdx[i][0]], at[sIdx[i][1]], at[sIdx[i][2]], 0);
            }
        }
        gSPEndDisplayList(g++);
    }
    return b;
}

/** The sway shears for this frame: per swaying kind, a few phases. */
static void Scatter_Sway(PlayState* play, ScatterFrame* frame) {
    s32 kind;
    s32 ph;
    s32 t = play->gameplayFrames;
    s32 init = !sSwayInit[sFrame]; // (the buffer's first frame: its matrices' fixed entries too)

    sSwayInit[sFrame] = true;
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

            // identity, but the y axis leans: x += a * y, z += b * y (the identity's entries written once: the
            // buffer keeps them, and only these two change)
            if (init) {
                MTX_SET(mtx, 0, 0, 0x10000);
                MTX_SET(mtx, 0, 1, 0);
                MTX_SET(mtx, 0, 2, 0);
                MTX_SET(mtx, 0, 3, 0);
                MTX_SET(mtx, 1, 1, 0x10000);
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
            MTX_SET(mtx, 1, 0, a);
            MTX_SET(mtx, 1, 2, b);
#ifdef SCATTER_VERIFY
            {
                // the entries kept from before: still the identity's
                s32 r;
                s32 c;

                for (r = 0; r < 4; r++) {
                    for (c = 0; c < 4; c++) {
                        if (!((r == 1) && ((c == 0) || (c == 2))) &&
                            ((mtx->intPart[r][c] != ((r == c) ? 1 : 0)) || (mtx->fracPart[r][c] != 0))) {
                            gFishmodScatterCacheMismatch++;
                            gFishmodScatterCacheDiff[0] = 4;
                        }
                    }
                }
            }
#endif
        }
    }
}

/**
 * Which props to draw, nearest band first, and their matrices, into the other set: one pass over the props near
 * the camera, keeping those in view (a cone around the look direction, any pitch, per chunk then per prop) and
 * within their kind's distance.
 */
/** How a prop at this (squared, flat) distance is drawn: see ScatterTier. */
static s32 Scatter_Tier(FishmodProp* prop, FishmodPropKindInfo* info, s32 kind, f32 dSq, f32 eyeY) {
    f32 s = prop->scale * (1.0f / 64.0f) * sLodScale;
    f32 farDist = PROP_FAR_DIST * sLodScale;
    f32 dy = eyeY - prop->y;
    f32 impDist = (sTunedImp[kind] > 0.0f) ? sTunedImp[kind] * prop->scale * (1.0f / 64.0f) : info->impDist * s;
    f32 halfDist = (sTunedHalf[kind] > 0.0f) ? sTunedHalf[kind] * prop->scale * (1.0f / 64.0f) : info->halfDist * s;

    if (info->xlu) {
        return TIER_NEAR;
    }
#if FISHMOD_DEBUG
    if (gFishmodLodTest.active && !gFishmodLodTest.asGame) {
        // the LOD tuning: the distances under test (0: not used), at the prop's scale, no dissolve
        f32 s1 = prop->scale * (1.0f / 64.0f);

        if ((info->modelImp != NULL) && (gFishmodLodTest.imp[kind] > 0) && (SQ(dy) <= IMP_SLOPE_SQ * dSq) &&
            (dSq >= SQ(gFishmodLodTest.imp[kind] * s1)) && (dSq >= SQ(farDist))) {
            return TIER_IMP;
        }
        if (dSq < SQ(farDist)) {
            return TIER_NEAR;
        }
        if ((info->modelHalf != NULL) && (gFishmodLodTest.half[kind] > 0) &&
            (dSq >= SQ(gFishmodLodTest.half[kind] * s1))) {
            return TIER_HALF;
        }
        return TIER_FAR;
    }
#endif
    // (the dissolve past the cutoff comes first: under water the cutoffs can be nearer than PROP_FAR_DIST)
    if ((info->modelImp != NULL) && !FISHMOD_BENCH_SKIP(FISHMOD_BENCH_NOIMP) && (SQ(dy) <= IMP_SLOPE_SQ * dSq) &&
        (((dSq >= SQ(impDist)) && (dSq >= SQ(farDist))) ||
         (dSq >= SQ(sKindMax[kind])))) {
        return TIER_IMP;
    }
    if (dSq < SQ(farDist)) {
        return TIER_NEAR;
    }
    if ((info->modelHalf != NULL) && !FISHMOD_BENCH_SKIP(FISHMOD_BENCH_NOFARTEX) && (dSq >= SQ(halfDist))) {
        return TIER_HALF;
    }
    return TIER_FAR;
}

/** The cosine of the angle from the look direction to the screen's corner, with VIEW_MARGIN: from the view's field of
 * view (vertical) and the 4:3 screen. */
static f32 Scatter_ViewCos(PlayState* play) {
    f32 half = DEG_TO_RAD(play->view.fovy * 0.5f);
    f32 t = (sinf(half) / cosf(half)) * (5.0f / 3.0f); // (the corner: the half-height and half-width, 3:4:5)

    t = (t + VIEW_MARGIN_TAN) / (1.0f - t * VIEW_MARGIN_TAN);
    if (t <= 0.0f) {
        return 0.0f; // (wider than a right angle: everything in front)
    }
    return 1.0f / sqrtf(1.0f + SQ(t));
}

static s16 sCards[BEDCARD_SET_MAX]; // (the build's bed cards, until it has its set)
static s32 sCardCount;
static u8* sUseCard; // per visible chunk: drawn as its card

static void Scatter_Build(PlayState* play, Vec3f* eyeP, f32 lookX, f32 lookY, f32 lookZ) {
    static u16 sVisible[256];                       // chunks near the camera
    static f32 sChunkDistSq[ARRAY_COUNT(sVisible)]; // their centres' distance from the eye, squared
    static u16 sCand[VISIBLE_MAX];                  // props in view (kind by kind)
    static u8 sCandBand[VISIBLE_MAX];               // and their distance bands
    static u8 sCandHidden[VISIBLE_MAX];             // and whether they're beyond the screen's edges
    static u16 sOrdered[VISIBLE_MAX];               // nearest band first, kind by kind within a band
    static u8 sOrderedHidden[VISIBLE_MAX];
    static u16 sKindStart[ARRAY_COUNT(sVisible)][FISHMOD_PROP_KIND_MAX + 1]; // each kind's first prop in the chunk
    Vec3f eye = *eyeP;
    s32 bandStart[BANDS + 1];
    s32 bandCount[BANDS];   // the props gathered in each band
    s32 bandLimit = BANDS;  // (gathered only nearer than this band: past it let go, the list being full)
    s32 candCount = 0;
    s32 visibleCount = 0;
    s32 chunk = sScatter->chunk;
    s32 under = (gFishmodHaze > 0.5f);
    f32 distFar = sDistFar[under];
    s32 mats = 0;
    s32 matsXlu = 0;
    s32 lastOpa = -1;
    s32 lastXlu = -1;
    s32 n;
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
    f32 viewCos = Scatter_ViewCos(play); // the view cone (and the tuned distances, under water)
    f32 viewRoom = 0.0f;     // extra room round each prop and chunk for it
    // the screen's own edges (inside the cone, which reaches the corners: the cone takes in a band above and below the
    // picture, about 40% more than the screen at any distance): the camera's right and up, and the half-width and
    // half-height as tangents, each with the cone's margin; a prop's sphere wholly outside one edge isn't drawn
    s32 useEdges = true;
    f32 rightX, rightY, rightZ, upX, upY, upZ;
    f32 tanH, tanV, secH, secV;

    {
        f32 t = sinf(DEG_TO_RAD(play->view.fovy * 0.5f)) / cosf(DEG_TO_RAD(play->view.fovy * 0.5f));
        f32 th = t * ((f32)SCREEN_WIDTH / SCREEN_HEIGHT);
        f32 len;

        tanV = (t + VIEW_MARGIN_TAN) / (1.0f - t * VIEW_MARGIN_TAN);
        tanH = (th + VIEW_MARGIN_TAN) / (1.0f - th * VIEW_MARGIN_TAN);
        secV = sqrtf(1.0f + SQ(tanV));
        secH = sqrtf(1.0f + SQ(tanH));
        // right = look x the view's up; up = right x look
        rightX = lookY * play->view.up.z - lookZ * play->view.up.y;
        rightY = lookZ * play->view.up.x - lookX * play->view.up.z;
        rightZ = lookX * play->view.up.y - lookY * play->view.up.x;
        len = sqrtf(SQ(rightX) + SQ(rightY) + SQ(rightZ));
        if ((len < 0.01f) || (tanV <= 0.0f) || (tanH <= 0.0f) || (t * VIEW_MARGIN_TAN >= 1.0f) ||
            (th * VIEW_MARGIN_TAN >= 1.0f)) {
            useEdges = false; // (looking straight along the up vector, or a view wider than a right angle)
        } else {
            rightX /= len;
            rightY /= len;
            rightZ /= len;
            upX = rightY * lookZ - rightZ * lookY;
            upY = rightZ * lookX - rightX * lookZ;
            upZ = rightX * lookY - rightY * lookX;
        }
    }

    Scatter_TunedDistances(under);
#if FISHMOD_DEBUG
    // the rotation check (FISHMOD_LOD_ROTATE) draws a reference with a much wider cone and nothing cut by distance
    if (gFishmodLodTest.active && gFishmodLodTest.wide) {
        viewCos = 0.3f;
        viewRoom = 400.0f;
        useEdges = false;
    }
    if (gFishmodLodTest.active && !gFishmodLodTest.asGame) {
        distFar = DIST_CALIBRATE;
        gFishmodLodTest.under = under;
        gFishmodLodTest.kindCount = FISHMOD_PROP_KIND_MAX;
        {
            s32 k;

            gFishmodLodTest.hasImp = gFishmodLodTest.hasHalf = 0;
            for (k = 0; k < FISHMOD_PROP_KIND_MAX; k++) {
                gFishmodLodTest.hasImp |= (u64)(sKinds[k].modelImp != NULL) << k;
                gFishmodLodTest.hasHalf |= (u64)(sKinds[k].modelHalf != NULL) << k;
            }
        }
        bzero(gFishmodLodTest.drawn, sizeof(gFishmodLodTest.drawn));
    }
#endif

    bzero(bandCount, sizeof(bandCount));
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
            // the whole chunk outside the view cone (the same test as each prop's, with the chunk's size as room),
            // at the height in it nearest the look's line: its props are anywhere from its bed to its surface (one
            // assumed height, 150 under the surface, could cull a deep bed's chunk beside a swimmer near the bottom
            // with its props on screen; fishmod/tools/popin_check.py found no prop missing on the bench's route
            // either way: a guard)
            my = (lookY / sqrtf(MAX(1.0f - SQ(lookY), 0.0001f))) * sqrtf(d); // (the look's line, this far out)
            if ((sChunkSurface[c] != -0x8000) && (my > sChunkSurface[c] + sExtentMax - eye.y)) {
                my = sChunkSurface[c] + sExtentMax - eye.y; // (nothing above the surface but what floats there)
            }
            front = mx * lookX + my * lookY + mz * lookZ + chunk * 0.71f + PROP_RADIUS + sExtentMax + viewRoom;
            if (((front < 0.0f) || (SQ(front) < SQ(viewCos) * (d + SQ(my)))) && (d > SQ(chunk * 1.5f))) {
                continue;
            }
            if (visibleCount < ARRAY_COUNT(sVisible)) {
                sChunkDistSq[visibleCount] = d;
                sVisible[visibleCount++] = c;
            } else {
                // (full: in place of the farthest, if this one is nearer, rather than whichever the grid reaches last,
                // a side of the view; the bench's views come nowhere near full: a guard)
                s32 far = 0;

                for (v = 1; v < visibleCount; v++) {
                    if (sChunkDistSq[v] > sChunkDistSq[far]) {
                        far = v;
                    }
                }
                if (d < sChunkDistSq[far]) {
                    sChunkDistSq[far] = d;
                    sVisible[far] = c;
                }
            }
        }
    }

    // where each kind's props start in each chunk (sorted by kind within a chunk: one pass finds them all)
    for (v = 0; v < visibleCount; v++) {
        s32 c = sVisible[v];
        s32 i = sChunkStart[c];
        s32 end = sChunkStart[c + 1];
        s32 k = 0;

        if ((sChunkKindRow != NULL) && (sChunkKindRow[c] != 0xFFFF)) {
            // (made at load: fishmod_scatter's load)
            bcopy(&sChunkKindPool[sChunkKindRow[c] * (FISHMOD_PROP_KIND_MAX + 1)], sKindStart[v],
                  (FISHMOD_PROP_KIND_MAX + 1) * sizeof(u16));
            continue;
        }
        for (; i < end; i++) {
            while (k <= sProps[i].kind) {
                sKindStart[v][k++] = i;
            }
        }
        while (k <= FISHMOD_PROP_KIND_MAX) {
            sKindStart[v][k++] = end;
        }
    }

    // Bed cards: a visible chunk far enough off (above water: under it the haze ends the props first) is drawn as its
    // card, and its card-drawn plants are left out below
    {
        static u8 sUse[ARRAY_COUNT(sVisible)];

        sCardCount = 0;
        for (v = 0; v < visibleCount; v++) {
            s32 card = (sChunkCard != NULL) ? sChunkCard[sVisible[v]] : -1;

            sUse[v] = false;
            if ((card < 0) || under || FISHMOD_BENCH_SKIP(FISHMOD_BENCH_NOBEDCARD) || (sCardCount >= BEDCARD_SET_MAX)) {
                continue;
            }
            {
                f32 dx = sBedCards[card].x - eye.x;
                f32 dz = sBedCards[card].z - eye.z;
                f32 dSq = SQ(dx) + SQ(dz);

                if ((dSq > SQ(BEDCARD_DIST)) && (dSq < SQ(distFar * (1.0f + FADE_BAND))) &&
                    (BedCard_Views(card) != NULL)) {
                    sUse[v] = true;
                    sCards[sCardCount++] = card;
                }
            }
        }
        sUseCard = sUse;
    }

    // One pass over the props near the camera: in view (a cone around the look direction, any pitch) and within their
    // kind's distance (above or under water), kind by kind, each with its distance band.
    for (kind = 0; kind < FISHMOD_PROP_KIND_MAX; kind++) {
        FishmodPropKindInfo* info = &sKinds[kind];
        f32 maxDist = under ? ((sTunedCut[kind] > 0.0f) ? sTunedCut[kind] : info->drawDistUnder) : info->drawDist;
        f32 maxSq;
        f32 reachSq;

#if FISHMOD_DEBUG
        if (gFishmodLodTest.active && !gFishmodLodTest.asGame) {
            // calibrating: everything out to DIST_CALIBRATE (the test's cut comes later, so it changes nothing else)
            maxDist = DIST_CALIBRATE;
        }
#endif
        sKindMax[kind] = maxDist;
        if ((info->modelImp != NULL) && !FISHMOD_BENCH_SKIP(FISHMOD_BENCH_NOIMP | FISHMOD_BENCH_NODISSOLVE)
#if FISHMOD_DEBUG
            && !gFishmodLodTest.active
#endif
        ) {
            maxDist *= 1.0f + FADE_BAND; // the dissolve, past the kind's distance
        }
        maxSq = SQ(maxDist);
        reachSq = SQ(maxDist + chunk * 0.71f);

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
            if (sUseCard[v] && (info->modelImp != NULL)) {
                continue; // (in the chunk's bed card)
            }
            end = sKindStart[v][kind + 1];
            for (i = sKindStart[v][kind]; i < end; i++) {
                FishmodProp* prop = &sProps[i];
                f32 dx = prop->x - eye.x;
                f32 dy = prop->y - eye.y;
                f32 dz = prop->z - eye.z;
                f32 dSq = SQ(dx) + SQ(dz);
                f32 front;
                s32 hidden;

                if (dSq > maxSq) {
                    continue;
                }
#if FISHMOD_DEBUG
                if ((gFishmodPropTest.mode == FISHMOD_PROPTEST_OFF) && (info == &sKinds[gFishmodPropTest.kind])) {
                    continue; // (the prop sweep: this kind left out)
                }
#endif
                if (info->card) {
                    // the camera inside it or right against it
                    f32 h = info->height * prop->scale * (1.0f / 64.0f);
                    f32 r = h * NEAR_SKIP + 20.0f;

                    if ((dSq + SQ(prop->y + h * 0.5f - eye.y)) < SQ(r)) {
                        continue;
                    }
                }
                // outside the view cone (with room for the prop's size: all of its height, as a tall weed's top can be
                // in view with its foot below the screen), unless it's right by the camera; squared, no square roots:
                // front + R < cos * dist  <=>  (front + R)^2 < cos^2 * dist^2 (when front + R >= 0)
                {
                    f32 room = PROP_RADIUS + info->extent * prop->scale * (1.0f / 64.0f) + viewRoom;

                    front = dx * lookX + dy * lookY + dz * lookZ + room;
                    if (((front < 0.0f) || (SQ(front) < SQ(viewCos) * (dSq + SQ(dy)))) && (dSq > SQ(room * 2.0f))) {
                        continue;
                    }
                    // wholly beyond the screen's left or right, top or bottom edge (the sphere's distance outside the
                    // edge's plane: (|x| - tan * z) / sec > room): kept in the set, so everything that reads it (the
                    // budget, the bubbles' plants, the random numbers they draw) is as it was, but not drawn
                    hidden = false;
                    if (useEdges && (dSq > SQ(room * 2.0f))) {
                        f32 z = front - room;
                        f32 x = dx * rightX + dy * rightY + dz * rightZ;
                        f32 y = dx * upX + dy * upY + dz * upZ;

                        hidden = (fabsf(x) > (tanH * z + room * secH)) || (fabsf(y) > (tanV * z + room * secV));
                    }
                }
                {
                    s32 b = CLAMP_MAX((s32)(sqrtf(dSq) * (1.0f / BAND_WIDTH)), BANDS - 1);

                    if (b >= bandLimit) {
                        continue;
                    }
                    if (candCount == VISIBLE_MAX) {
                        // full: the farthest band there is let go (all of it: the props are gathered kind by kind,
                        // chunk by chunk, so what came last is a kind or a side of the view, not the farthest), if
                        // this one is nearer
                        s32 far = bandLimit - 1;
                        s32 m;

                        while ((far > 0) && (bandCount[far] == 0)) {
                            far--;
                        }
                        if (far <= b) {
                            continue;
                        }
                        bandLimit = far;
                        for (n = 0, m = 0; n < candCount; n++) {
                            if (sCandBand[n] < bandLimit) {
                                sCandBand[m] = sCandBand[n];
                                sCandHidden[m] = sCandHidden[n];
                                sCand[m++] = sCand[n];
                            }
                        }
                        candCount = m;
                        bandCount[far] = 0;
                    }
                    bandCount[b]++;
                    sCandBand[candCount] = b;
                    sCandHidden[candCount] = hidden;
                    sCand[candCount++] = i;
                }
            }
        }
    }

    // nearest band first, kind by kind within a band: a counting sort by band (stable, so the kinds stay together)
    bzero(bandStart, sizeof(bandStart));
    for (n = 0; n < candCount; n++) {
        bandStart[sCandBand[n] + 1]++;
    }
    for (band = 0; band < BANDS; band++) {
        bandStart[band + 1] += bandStart[band];
    }
    for (n = 0; n < candCount; n++) {
        sOrderedHidden[bandStart[sCandBand[n]]] = sCandHidden[n];
        sOrdered[bandStart[sCandBand[n]]++] = sCand[n];
    }
    // into the other set, as far as the budget (and the room for material changes) goes
    sSet ^= 1;
    sMtxUsed = 0;
    set = &sBufs->set[sSet];
    set->count = 0;
    set->cardCount = sCardCount;
    bcopy(sCards, set->cards, sCardCount * sizeof(s16));
    // the single cards face the camera: they turn about their own up, by the look's direction across the ground
    sBillboardYaw = Math_Atan2S(-lookZ, -lookX);
    sLodScale = under ? UNDER_LOD_SCALE : 1.0f;
#if FISHMOD_DEBUG
    // (props in view the caps leave out: the frame budget, the see-through cap, the material changes' room)
    gFishmodScatterDropped[3] = MAX(gFishmodScatterDropped[3], (u32)candCount);
#endif
    for (n = 0; (n < candCount) && (budget > 0); n++) {
        FishmodProp* prop = &sProps[sOrdered[n]];
        s32 k = prop->kind;
        s32 tier = Scatter_Tier(prop, &sKinds[k], k, SQ(prop->x - eye.x) + SQ(prop->z - eye.z), eye.y);
        s32 key = k + tier * FISHMOD_PROP_KIND_MAX; // (a material per kind and tier)

        if (sKinds[k].xlu) {
            if ((xluLeft <= 0) || ((k != lastXlu) && (matsXlu >= MAT_MAX_XLU))) {
#if FISHMOD_DEBUG
                gFishmodScatterDropped[1]++;
#endif
                continue;
            }
            xluLeft--;
            if (k != lastXlu) {
                matsXlu++;
                lastXlu = k;
            }
        } else {
            if ((key != lastOpa) && (mats >= MAT_MAX)) {
#if FISHMOD_DEBUG
                gFishmodScatterDropped[2]++;
#endif
                continue;
            }
            if (key != lastOpa) {
                mats++;
                lastOpa = key;
            }
        }
        budget--;
#if FISHMOD_DEBUG
        if (gFishmodLodTest.active) {
            // the test's cut: the reference's props less these, so the budget and the materials go as before
            if ((gFishmodLodTest.dist[k] > 0) &&
                ((SQ(prop->x - eye.x) + SQ(prop->z - eye.z)) > SQ(gFishmodLodTest.dist[k]))) {
                continue;
            }
            gFishmodLodTest.drawn[k]++;
        }
#endif
        set->hidden[set->count] = sOrderedHidden[n];
        if (sOrderedHidden[n]) {
            set->mtxp[set->count] = NULL; // (not drawn: no matrix)
        } else {
            s16 yaw = (tier == TIER_IMP) ? sBillboardYaw : (s16)(prop->yaw << 8);

            if (sKinds[k].faceCam && (tier != TIER_IMP)) {
                // one card that only turns as far as it must: kept within 45 degrees of facing the camera (by where
                // the camera is, not where it looks), so it never shows edge-on and doesn't spin as the view pans
                s16 own = yaw;
                s16 delta = (s16)(Math_Atan2S(-(prop->z - eye.z), -(prop->x - eye.x)) - own);

                if (delta > 0x4000) { // (the card is the same from either side)
                    delta -= 0x8000;
                } else if (delta < -0x4000) {
                    delta += 0x8000;
                }
                if (delta > 0x2000) {
                    yaw = own + (delta - 0x2000);
                } else if (delta < -0x2000) {
                    yaw = own + (delta + 0x2000);
                }
            }
            if ((sPropMtx != NULL) && (tier != TIER_IMP) && !sKinds[k].faceCam) {
                // (its own matrix, made once: it depends on the prop alone)
                s32 id = sOrdered[n];

                if (!sPropMtxMade[id]) {
                    Scatter_MatrixInto(&sPropMtx[id], prop, k == FISHMOD_PROP_LILY_PAD, yaw);
                    sPropMtxMade[id] = true;
                }
                set->mtxp[set->count] = &sPropMtx[id];
#ifdef SCATTER_VERIFY
                {
                    // the check: made again and compared with the kept one
                    Mtx check;

                    Scatter_MatrixInto(&check, prop, k == FISHMOD_PROP_LILY_PAD, yaw);
                    gFishmodScatterCacheUsed++;
                    s32 w;

                    for (w = 0; w < (s32)(sizeof(Mtx) / sizeof(u32)); w++) {
                        if (((u32*)&check)[w] != ((u32*)&sPropMtx[id])[w]) {
                            gFishmodScatterCacheMismatch++;
                            gFishmodScatterCacheDiff[0] = 5;
                            break;
                        }
                    }
                }
#endif
            } else {
                set->mtxp[set->count] = Scatter_Matrix(prop, k == FISHMOD_PROP_LILY_PAD, yaw);
            }
        }
        set->tier[set->count] = tier;
        set->prop[set->count++] = sOrdered[n];
    }
#if FISHMOD_DEBUG
    if (n < candCount) {
        gFishmodScatterDropped[0] += candCount - n; // (the budget ran out: the rest, farthest first, not drawn)
    }
    {
        // (for fishmod/tools/popin_check.py: what this set was made from, to check it against every prop in view)
        static f32 sView[12];
        static u8 sCardChunk[BEDCARD_SET_MAX + 1];
        s32 v2;
        s32 nc = 0;

        sView[0] = eye.x;
        sView[1] = eye.y;
        sView[2] = eye.z;
        sView[3] = lookX;
        sView[4] = lookY;
        sView[5] = lookZ;
        sView[6] = play->view.up.x;
        sView[7] = play->view.up.y;
        sView[8] = play->view.up.z;
        sView[9] = play->view.fovy;
        sView[10] = under;
        sView[11] = distFar;
        for (v2 = 0; (v2 < visibleCount) && (nc < BEDCARD_SET_MAX); v2++) {
            if (sUseCard[v2]) {
                sCardChunk[1 + nc++] = 0; // (count only: the chunks' numbers don't fit a byte)
            }
        }
        sCardChunk[0] = nc;
        gFishmodScatterDbg[0] = sScatter;
        gFishmodScatterDbg[1] = sProps;
        gFishmodScatterDbg[2] = set;
        gFishmodScatterDbg[3] = sKindMax;
        gFishmodScatterDbg[4] = sView;
        gFishmodScatterDbg[5] = sChunkDry;
        gFishmodScatterDbg[6] = sCardChunk;
        gFishmodScatterDbg[7] = sKinds;
    }
#endif
}

void EnFishmod_ScatterDraw(PlayState* play) {
    Vec3f eye = play->view.eye;
    Vec3f at = play->view.at;
    f32 lookX;
    f32 lookY;
    f32 lookZ;
    f32 lookLen;
    s32 kind;
    s32 lastOpa = -1; // the material each list has now (the two are interleaved: nearest first, whatever the kind)
    s32 lastXlu = -1;
    s32 lastNear = -1; // the edge mode the opaque list has now
    s32 n;
    Gfx* opa;
    Gfx* xlu;
    u32 light[3];
    ScatterFrame* frame;
    ScatterSet* set;
    s32 cacheUntil;
    s32 cacheStart;
    s32 cacheValid;
    Gfx* cacheOpa0;
    Gfx* cacheXlu0;
    ScatterKeep keepNow;
#if FISHMOD_DEBUG
    static OSTime sT0, sT1, sT2;
    static s32 sBuilt;
    OSTime tStart = osGetTime();
#endif

    if ((sScatter == NULL) || (sChunkDry == NULL) || (sBufs == NULL)) {
        return;
    }
    Scatter_UpdateDry(play);
    sBedFrame++;
    sBedLoads = 0;
    // a camera being set up can give a nonsense eye for a frame (and turning a NaN into an integer is a crash on
    // the N64): skip the props that frame
    if (!(eye.x == eye.x) || !(eye.y == eye.y) || !(eye.z == eye.z) || (fabsf(eye.x) > 30000.0f) ||
        (fabsf(eye.z) > 30000.0f) || !(at.x == at.x) || !(at.y == at.y) || !(at.z == at.z)) {
        return;
    }
    // The scene's light on the props, which are unlit (their colours are baked for daylight): the ambient light and
    // half of each directional light, as a vertical card facing any way gets on average. Lake Hylia's midday comes to
    // about 255 (so the day looks as it did), its night to a dim blue, dusk to orange.
    {
        EnvironmentContext* env = &play->envCtx;
        s32 i;

        for (i = 0; i < 3; i++) {
            s32 l = play->lightCtx.ambientColor[i] + (env->dirLight1.params.dir.color[i] >> 1) +
                    (env->dirLight2.params.dir.color[i] >> 1);

            light[i] = CLAMP_MAX(l, 255);
        }
    }
#if ENABLE_F3DEX3
    sSceneLights = NULL;
    if (SCATTER_ONE_CYCLE()) {
        // the scene's lights as the fish manager's draw had them (to put back after the 1-cycle props' own)
        sSceneLights = LightContext_NewLights(&play->lightCtx, play->state.gfxCtx);
        Lights_BindAll(sSceneLights, play->lightCtx.listHead, NULL);
        sSceneLights->l.l[sSceneLights->numLights].l.col[0] = sSceneLights->l.l[sSceneLights->numLights].l.colc[0] =
            sSceneLights->l.a.l.col[0];
        sSceneLights->l.l[sSceneLights->numLights].l.col[1] = sSceneLights->l.l[sSceneLights->numLights].l.colc[1] =
            sSceneLights->l.a.l.col[1];
        sSceneLights->l.l[sSceneLights->numLights].l.col[2] = sSceneLights->l.l[sSceneLights->numLights].l.colc[2] =
            sSceneLights->l.a.l.col[2];
    }
#endif
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
#if FISHMOD_DEBUG
        gFishmodLodTest.active || // calibrating: what's drawn changes with the test, not the camera
        ((u32)gFishmodBench != sSetBench) || // the bench switched a mode: the tiers are chosen here
        (gFishmodPropTest.serial != sSetPropTest) || // the prop sweep switched its test
#endif
        ((SQ(eye.x - sSetEye.x) + SQ(eye.y - sSetEye.y) + SQ(eye.z - sSetEye.z)) > SQ(REUSE_MOVE)) ||
        ((lookX * sSetLook.x + lookY * sSetLook.y + lookZ * sSetLook.z) < REUSE_TURN_COS)) {
        Scatter_Build(play, &eye, lookX, lookY, lookZ);
        sSetGen++;
#if FISHMOD_DEBUG
        sSetBench = gFishmodBench;
        sSetPropTest = gFishmodPropTest.serial;
#endif
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
    if (sSceneLights != NULL) {
        frame->lights = *sSceneLights;
        sSceneLights = &frame->lights;
    }
    Scatter_Sway(play, frame);
    Scatter_Touch(play);
    Scatter_TouchMatrices(frame);
    Scatter_RockCaustics(play, frame);
#if FISHMOD_DEBUG
    gFishmodScatterDrawn = set->count;
#endif
    // the commands up to the first fading prop: left as they are in the buffer if nothing they depend on has changed
    {
        ScatterKeep* keep = &sKeep[sFrame];
        s32 k;

        cacheUntil = set->count;
        for (k = 0; k < set->count; k++) {
            if (set->tier[k] == TIER_IMP) {
                cacheUntil = k;
                break;
            }
        }
        cacheOpa0 = opa;
        cacheXlu0 = xlu;
        keepNow.opaFrom = opa - frame->opa;
        keepNow.xluFrom = xlu - frame->xlu;
        keepNow.until = cacheUntil;
        keepNow.setGen = sSetGen;
        keepNow.light[0] = light[0];
        keepNow.light[1] = light[1];
        keepNow.light[2] = light[2];
        keepNow.oneCycle = SCATTER_ONE_CYCLE();
        keepNow.numLights = (sSceneLights != NULL) ? sSceneLights->numLights : -1;
        keepNow.touchGen = sTouchGen;
        Scatter_PropFogSetup();
        keepNow.fogKey = sPropFog.key;
#if FISHMOD_DEBUG
        keepNow.bench = (u32)gFishmodBench;
#else
        keepNow.bench = 0;
#endif
        cacheValid = (keep->opaLen != 0) && (keep->opaFrom == keepNow.opaFrom) && (keep->xluFrom == keepNow.xluFrom) &&
                     (keep->until == keepNow.until) && (keep->setGen == keepNow.setGen) &&
                     (keep->light[0] == keepNow.light[0]) && (keep->light[1] == keepNow.light[1]) &&
                     (keep->light[2] == keepNow.light[2]) && (keep->oneCycle == keepNow.oneCycle) &&
                     (keep->numLights == keepNow.numLights) && (keep->bench == keepNow.bench) &&
                     (keep->touchGen == keepNow.touchGen) && (keep->fogKey == keepNow.fogKey)
#if FISHMOD_DEBUG
                     && !gFishmodLodTest.active
#endif
#ifdef SCATTER_NOCACHE
                     && false
#endif
            ;
        cacheStart = 0;
        sPropFog.vtxUsed = sPropFog.cmdUsed = 0;
        if (!cacheValid) {
            keep->opaLen = 0; // (the buffer is about to be written over: nothing kept until this frame's are noted)
        } else {
#if FISHMOD_DEBUG
            gFishmodScatterCacheUsed++;
#endif
#ifdef SCATTER_VERIFY
            // the check: the kept commands set aside, then made afresh over them as before, and the two compared
            for (k = 0; k < keep->opaLen - 1; k++) {
                sCheckOpa[k] = opa[k];
            }
            for (k = 0; k < keep->xluLen; k++) {
                sCheckXlu[k] = xlu[k];
            }
#else
            opa += keep->opaLen - 1;
            xlu += keep->xluLen;
            lastOpa = keep->lastOpa;
            lastXlu = keep->lastXlu;
            lastNear = keep->lastNear;
            sEdgeNear = keep->edgeNear;
            sPropFog.vtxUsed = keep->fogVtxUsed;
            sPropFog.cmdUsed = keep->fogCmdUsed;
            cacheStart = cacheUntil;
#endif
        }
    }
    for (n = cacheStart; n < set->count; n++) {
        FishmodProp* prop = &sProps[set->prop[n]];
        ScatterFogTpl* fogTpl;
        s32 fogSurf;
        u32 fogCol[3];
        FishmodPropKindInfo* info;
        Gfx** disp;
        f32 dSq;
        s32 tier;
        s32 key;

        if (set->hidden[n]) {
            continue; // (beyond the screen's edges)
        }

        if (n == cacheUntil) {
            Scatter_KeepAt(&sKeep[sFrame], cacheValid, cacheOpa0, opa, cacheXlu0, xlu, &keepNow, lastOpa, lastXlu,
                           lastNear);
        }
        kind = prop->kind;
        info = &sKinds[kind];
        dSq = SQ(prop->x - eye.x) + SQ(prop->z - eye.z);
        disp = info->xlu ? &xlu : &opa;
        tier = set->tier[n];
        key = kind + tier * FISHMOD_PROP_KIND_MAX;
        if (!info->xlu) {
            // (nearest first, in bands: this changes once, from near to far)
            s32 near = (tier == TIER_NEAR);

            if (near != lastNear) {
                lastNear = near;
                sEdgeNear = near;
                lastOpa = -2; // the material again, with the other edge mode (-2: no texture to turn off first)
            }
        }
        if ((info->xlu ? kind : key) != (info->xlu ? lastXlu : lastOpa)) {
            if (info->xlu) {
                lastXlu = kind;
            } else {
                if (lastOpa >= 0) {
                    gSPTexture(opa++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
                }
                lastOpa = key;
            }
            *disp = Scatter_Material(*disp, info, kind, tier);
        }
        fogSurf = -0x8000;
        fogTpl = NULL;
        if (!info->xlu) {
            u32 t = (prop->tint > 255 / 2) ? 255 : prop->tint * 2;
            s32 a = 255;
            u32 col[3];

            if ((tier == TIER_IMP) && !FISHMOD_BENCH_SKIP(FISHMOD_BENCH_NODISSOLVE)
#if FISHMOD_DEBUG
                && !gFishmodLodTest.active
#endif
            ) {
                // past its distance: dissolving away (a dithered alpha test, no blending)
                f32 from = sKindMax[kind];

                if (dSq > SQ(from)) {
                    a = (s32)((from * (1.0f + FADE_BAND) - sqrtf(dSq)) * (255.0f / (from * FADE_BAND)));
                    a = CLAMP(a, 0, 255);
                }
            }
            if (SCATTER_IS_ROCK(kind)) {
                // each rock a slightly different stone: neutral, warmer or cooler grey (by where it stands)
                static const u8 sStone[4][3] = { { 255, 255, 255 }, { 255, 240, 222 }, { 228, 240, 255 }, { 240, 248, 232 } };
                const u8* c = sStone[((u32)(prop->x * 7 + prop->z * 13) >> 4) & 3];

                col[0] = (t * light[0] * c[0]) >> 16;
                col[1] = (t * light[1] * c[1]) >> 16;
                col[2] = (t * light[2] * c[2]) >> 16;
            } else {
                col[0] = (t * light[0]) >> 8;
                col[1] = (t * light[1]) >> 8;
                col[2] = (t * light[2]) >> 8;
            }
            // the water's fog over it, seen from above: near, vertex by vertex (its model copied); else by its middle
            fogSurf = Scatter_FogSurface(prop, info);
            if ((fogSurf != -0x8000) && (prop->y < fogSurf)) {
                s32 isRock = SCATTER_IS_ROCK(kind) && (sRockList != NULL);

                if ((dSq < SQ(WATERFOG_VERTEX_DIST)) && (tier != TIER_IMP) && (tier != TIER_HALF) &&
                    !((sTouchSlot != NULL) && (sTouchSlot[set->prop[n]] != TOUCH_NONE))) {
                    if (isRock) {
                        if ((sFogTplRock != NULL) && !sFogTplRockTried[prop->yaw]) {
                            sFogTplRockTried[prop->yaw] = true;
                            sFogTplRock[prop->yaw] = Scatter_FogTplBuild(sRockList[prop->yaw], NULL);
                        }
                        fogTpl = (sFogTplRock != NULL) ? sFogTplRock[prop->yaw] : NULL;
                    } else {
                        s32 which = (tier == TIER_FAR);

                        if (!sFogTplTried[kind][which]) {
                            s32 objSlot = Object_GetSlot(&play->objectCtx, OBJECT_FISH);

                            sFogTplTried[kind][which] = true;
                            sFogTplKind[kind][which] = Scatter_FogTplBuild(
                                which ? info->modelFar : info->model,
                                (objSlot >= 0) ? (u8*)play->objectCtx.slots[objSlot].segment : NULL);
                        }
                        fogTpl = sFogTplKind[kind][which];
                    }
                }
                if (fogTpl == NULL) {
                    // by its middle: its light blended toward the water's (over the texture's and the vertex
                    // colours' means)
                    f32 k = SCATTER_IS_ROCK(kind) ? 1.0f : prop->scale * (1.0f / 64.0f);
                    f32 fa = Scatter_FogAt(prop->x, prop->y + info->extent * k * 0.5f, prop->z, fogSurf);
                    s32 ai = (s32)(fa * 256.0f);
                    s32 ch;

                    for (ch = 0; ch < 3; ch++) {
                        s32 tc = Scatter_FogTarget(ch, sKindMean[kind][ch], 160);

                        col[ch] = col[ch] + (((tc - (s32)col[ch]) * ai) >> 8);
                    }
                }
            } else {
                fogSurf = -0x8000;
            }
            opa = Scatter_PropColor(opa, tier, col[0], col[1], col[2], a);
            fogCol[0] = col[0];
            fogCol[1] = col[1];
            fogCol[2] = col[2];
        }
        if ((sTouchSlot != NULL) && (sTouchSlot[set->prop[n]] != TOUCH_NONE) && !TOUCH_DRIFTS(kind) &&
            (tier == TIER_NEAR)) {
            // a touched plant, bending: its bands loaded each under its own share of the lean, then its triangles
            s32 slot = sTouchSlot[set->prop[n]];
            ScatterBend* bend;

            if (!sBendTried[kind]) {
                sBendTried[kind] = true;
                {
                    // (object_fish's place, for the Fishing Pond's reed: segment 6 as the props' manager set it)
                    s32 objSlot = Object_GetSlot(&play->objectCtx, OBJECT_FISH);

                    sBend[kind] = Scatter_BendBuild(
                        info->model, (objSlot >= 0) ? (u8*)play->objectCtx.slots[objSlot].segment : NULL);
                }
            }
            bend = sBend[kind];
            if (bend != NULL) {
                s32 k;

                for (k = 0; k < BEND_BANDS; k++) {
                    s32 cnt = bend->start[k + 1] - bend->start[k];

                    if (cnt == 0) {
                        continue;
                    }
                    gSPMatrix((*disp)++, set->mtxp[n], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                    if (info->sway != 0) {
                        s32 ph = ((prop->x + prop->z) >> 7) & (SWAY_PHASES - 1);

                        gSPMatrix((*disp)++, &frame->sway[kind * SWAY_PHASES + ph],
                                  G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
                    }
                    gSPMatrix((*disp)++, &frame->bendBand[slot][k], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
                    gSPVertex((*disp)++, &bend->vtx[bend->start[k]], cnt, bend->start[k]);
                }
                gSPDisplayList((*disp)++, bend->tris);
                continue;
            }
        }
        gSPMatrix((*disp)++, set->mtxp[n], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        if (info->sway != 0) {
            // neighbours share a phase: a wave through the bed
            s32 ph = ((prop->x + prop->z) >> 7) & (SWAY_PHASES - 1);

            gSPMatrix((*disp)++, &frame->sway[kind * SWAY_PHASES + ph], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        }
        if ((sTouchSlot != NULL) && (sTouchSlot[set->prop[n]] != TOUCH_NONE)) {
            // (making way for Link)
            gSPMatrix((*disp)++, &frame->bend[sTouchSlot[set->prop[n]]], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        }
        {
            Gfx* near = info->model;

#if FISHMOD_DEBUG
            // (the prop sweep: this kind's near model whole, or its cut split low)
            if ((gFishmodPropTest.mode != FISHMOD_PROPTEST_NONE) && (info == &sKinds[gFishmodPropTest.kind])) {
                near = (gFishmodPropTest.mode == FISHMOD_PROPTEST_WHOLE) ? info->modelFar
                       : (gFishmodPropTest.mode == FISHMOD_PROPTEST_LOW) ? info->modelLow
                                                                         : near;
            }
#endif
            if (fogTpl != NULL) {
                // (its model's commands, its vertices its own, coloured by the water's fog)
                Gfx* g = Scatter_FogEmit(*disp, fogTpl, prop, kind, fogSurf, fogCol[0], fogCol[1], fogCol[2]);

                if (g != NULL) {
                    *disp = g;
                    continue;
                }
            }
            if (SCATTER_IS_ROCK(kind) && (sRockList != NULL)) {
                near = sRockList[prop->yaw]; // (its own, near or far: few triangles, all of them shown)
                gSPDisplayList((*disp)++, near);
            } else {
                gSPDisplayList((*disp)++, (tier == TIER_IMP)    ? info->modelImp
                                          : (tier == TIER_HALF) ? info->modelHalf
                                          : (tier == TIER_FAR)  ? info->modelFar
                                                                : near);
            }
        }
    }
    if (cacheUntil == set->count) { // (no fading prop: the whole loop is the part kept)
        Scatter_KeepAt(&sKeep[sFrame], cacheValid, cacheOpa0, opa, cacheXlu0, xlu, &keepNow, lastOpa, lastXlu,
                       lastNear);
    }
    if (lastOpa >= 0) {
        gSPTexture(opa++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    }
    // the bed cards (each in the plane of its view nearest the camera's direction to it: four views 45 degrees apart,
    // and from the other side the double-sided card shows that view's mirror image)
    if (set->cardCount > 0) {
        s32 k;

        opa = Gfx_SetupDL(opa, SETUPDL_25);
        gDPPipeSync(opa++);
        gDPSetCombineLERP(opa++, TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0, COMBINED, 0, PRIMITIVE, 0, 0, 0, 0, COMBINED);
        gDPSetRenderMode(opa++, G_RM_FOG_SHADE_A,
                         AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP | CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |
                             GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM));
        gSPClearGeometryMode(opa++, G_CULL_BACK | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
        gSPSetGeometryMode(opa++, G_FOG | G_SHADE | G_SHADING_SMOOTH);
        gDPSetPrimColor(opa++, 0, 0, light[0], light[1], light[2], 255);
        gSPTexture(opa++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureLUT(opa++, G_TT_RGBA16);
        for (k = 0; k < set->cardCount; k++) {
            s32 card = set->cards[k];
            FishmodBedCard* bc = &sBedCards[card];
            u8* views = BedCard_Views(card);
            Mtx* mtx;
            Vtx* vtx;
            f32 dx = bc->x - eye.x;
            f32 dz = bc->z - eye.z;
            f32 best = -1.0f;
            s32 view = 0;
            s32 v;
            f32 rx;
            f32 rz;

            if (views == NULL) {
                continue; // (pushed out of the cache: its plants come back with the next build)
            }
            // the view looking most nearly along the camera's direction to the card: view v looks along
            // (sin t, -cos t) with t = v x 45 degrees
            if (!sBedViewTrigMade) {
                // (each view's sin and cos, worked out once: the same calls, the same values, as each frame made them)
                for (v = 0; v < FISHMOD_BEDCARD_VIEWS; v++) {
                    f32 t = v * (M_PI / FISHMOD_BEDCARD_VIEWS);

                    sBedViewSin[v] = sinf(t);
                    sBedViewCos[v] = cosf(t);
                }
                sBedViewTrigMade = true;
            }
            for (v = 0; v < FISHMOD_BEDCARD_VIEWS; v++) {
                f32 d = fabsf(dx * sBedViewSin[v] - dz * sBedViewCos[v]);

                if (d > best) {
                    best = d;
                    view = v;
                }
            }
            // the card's width along the view's right (cos t, sin t)
            rx = sBedViewCos[view] * bc->reach;
            rz = sBedViewSin[view] * bc->reach;
            mtx = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Mtx));
            vtx = GRAPH_ALLOC(play->state.gfxCtx, 4 * sizeof(Vtx));
            guTranslate(mtx, bc->x, bc->y, bc->z);
            {
                s16 ix = (s16)rx;
                s16 iz = (s16)rz;
                s16 h = bc->height;
                s32 sw = FISHMOD_BEDCARD_TEX_W << 5;
                s32 th = FISHMOD_BEDCARD_TEX_H << 5;

                // (bottom left, bottom right, top right, top left; the texture's first row is the top)
                vtx[0] = (Vtx){ { { -ix, 0, -iz }, 0, { 0, th }, { 255, 255, 255, 255 } } };
                vtx[1] = (Vtx){ { { ix, 0, iz }, 0, { sw, th }, { 255, 255, 255, 255 } } };
                vtx[2] = (Vtx){ { { ix, h, iz }, 0, { sw, 0 }, { 255, 255, 255, 255 } } };
                vtx[3] = (Vtx){ { { -ix, h, -iz }, 0, { 0, 0 }, { 255, 255, 255, 255 } } };
            }
            gDPLoadTLUT_pal16(opa++, 0, views + view * FISHMOD_BEDCARD_VIEW_BYTES);
            gDPLoadTextureBlock_4b(opa++, views + view * FISHMOD_BEDCARD_VIEW_BYTES + 32, G_IM_FMT_CI,
                                   FISHMOD_BEDCARD_TEX_W, FISHMOD_BEDCARD_TEX_H, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                   G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSPMatrix(opa++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPVertex(opa++, vtx, 4, 0);
            gSP2Triangles(opa++, 0, 1, 2, 0, 0, 2, 3, 0);
        }
        gDPPipeSync(opa++);
        gDPSetTextureLUT(opa++, G_TT_NONE);
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
        {
            // far from the dock (from its nearest point), its posts with four sides instead of six
            f32 dx = eye.x - CLAMP(eye.x, FISHMOD_DOCK_X - 45.0f, FISHMOD_DOCK_X + 45.0f);
            f32 dz = eye.z - CLAMP(eye.z, FISHMOD_DOCK_Z, FISHMOD_DOCK_Z + 420.0f);
            s32 far = (SQ(dx) + SQ(dz)) > SQ(PROP_FAR_DIST);

            for (k = FISHMOD_PROP_DOCK_DECK; k <= FISHMOD_PROP_DOCK_POSTS; k++) {
                opa = Scatter_Material(opa, &sKinds[k], k, TIER_NEAR);
                opa = Scatter_PropColor(opa, TIER_NEAR, light[0], light[1], light[2], 255);
                gSPMatrix(opa++, &sDockMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(opa++, far ? sKinds[k].modelFar : sKinds[k].model);
            }
        }
        gSPTexture(opa++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    }
    gDPPipeSync(opa++);
    gDPSetTextureLUT(opa++, G_TT_NONE); // (a 4-bit texture's palette mode off again for what's drawn next)
    if (SCATTER_ONE_CYCLE()) {
        // what's drawn next: the 2-cycle mode, no packed normals, and the scene's lights
        gDPSetCycleType(opa++, G_CYC_2CYCLE);
#ifdef G_PACKED_NORMALS // (F3DEX3 only; the emulator profile's microcode never takes this branch)
        gSPClearGeometryMode(opa++, G_PACKED_NORMALS);
#endif
        opa = Scatter_SceneLights(opa);
        xlu = Scatter_SceneLights(xlu);
    }
    gSPEndDisplayList(opa++);
    gSPEndDisplayList(xlu++);
#if FISHMOD_DEBUG
    gFishmodScatterCmdTime[0] += (u32)(osGetTime() - tStart);
    gFishmodScatterCmdTime[1]++;
    sT1 += osGetTime() - tStart;
    sT2++;
    if ((play->gameplayFrames & 15) == 0) { // (printing is slow: not every frame)
#if FISHMOD_PROFILE_OCCLUSION && defined(F3DEX3_OCCLUSION)
        Fishmod_DebugPrintf(10, "p%d b%d us %d+%d occ %d", set->count, sBuilt, (s32)OS_CYCLES_TO_USEC(sT0 / sT2),
                            (s32)OS_CYCLES_TO_USEC(sT1 / sT2), EnFishmod_OcclusionChosen());
#else
        Fishmod_DebugPrintf(10, "p%d b%d us %d+%d", set->count, sBuilt, (s32)OS_CYCLES_TO_USEC(sT0 / sT2),
                            (s32)OS_CYCLES_TO_USEC(sT1 / sT2));
#endif
        sT0 = sT1 = sT2 = 0;
        sBuilt = 0;
    }
#endif

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_scatter.c", __LINE__);
#if FISHMOD_PROFILE_OCCLUSION && defined(F3DEX3_OCCLUSION)
    {
        // the occlusion plane around the props only (the scene, the actors and the HUD as before)
        void* occ = EnFishmod_OcclusionPlane(play);

        gSPOcclusionPlane(POLY_OPA_DISP++, occ);
        gSPDisplayList(POLY_OPA_DISP++, frame->opa);
        gSPOcclusionPlane(POLY_OPA_DISP++, EnFishmod_OcclusionNone());
        gSPOcclusionPlane(POLY_XLU_DISP++, occ);
        gSPDisplayList(POLY_XLU_DISP++, frame->xlu);
        gSPOcclusionPlane(POLY_XLU_DISP++, EnFishmod_OcclusionNone());
    }
#else
    gSPDisplayList(POLY_OPA_DISP++, frame->opa);
    gSPDisplayList(POLY_XLU_DISP++, frame->xlu);
#endif
    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_scatter.c", __LINE__);
}


#if FISHMOD_DEBUG
/*
 * The plant stress bench (fishmod_debug_stress.inc.c): a kelp forest in the view ahead of Link, its first `count`
 * plants drawn as the scene's props are (full models, the same materials and lighting), nearest first. Placed once on
 * the bed in a wedge 60 to 900 units ahead, 70 degrees wide, of the tall underwater kinds in turn.
 */
#define STRESS_PLANTS_MAX 1600

typedef struct StressPlant {
    FishmodProp prop;
    Mtx mtx;
} StressPlant;

static StressPlant* sStressPlants;
static s32 sStressPlantCount;
// its own display lists, two (one built while the RSP draws the other): a plant takes 4 commands, a kind's material
// about 30 (the frame's own lists have no room for a forest)
#define STRESS_DL_SIZE (STRESS_PLANTS_MAX * 4 + 512)
static Gfx* sStressDL[2];

static void Stress_PlacePlants(PlayState* play) {
    static const u8 sKindsUsed[] = { FISHMOD_PROP_KELP_FROND, FISHMOD_PROP_RIBBON_WEED, FISHMOD_PROP_BIG_PONDWEED,
                                     FISHMOD_PROP_TALL_WEED, FISHMOD_PROP_FAN_WEED };
    Player* player = GET_PLAYER(play);
    u32 seed = 0x5EED1234;
    s32 i;

    sStressPlants = SystemArena_Malloc(STRESS_PLANTS_MAX * sizeof(StressPlant)); // (120 KB: the actor heap has no room)
    sStressDL[0] = SystemArena_Malloc(STRESS_DL_SIZE * sizeof(Gfx));
    sStressDL[1] = SystemArena_Malloc(STRESS_DL_SIZE * sizeof(Gfx));
    if ((sStressPlants == NULL) || (sStressDL[0] == NULL) || (sStressDL[1] == NULL)) {
        sStressPlants = NULL;
        return;
    }
    sStressPlantCount = 0;
    for (i = 0; i < STRESS_PLANTS_MAX; i++) {
        StressPlant* p = &sStressPlants[sStressPlantCount];
        // nearest first: the distance grows with the index (the area of the wedge out to d grows as d squared)
        f32 t = (i + 0.5f) / STRESS_PLANTS_MAX;
        f32 d = 60.0f + sqrtf(t) * 840.0f;
        s16 yaw;
        f32 surface;
        f32 floor;
        f32 x;
        f32 z;

        seed = seed * 1664525 + 1013904223;
        yaw = player->actor.shape.rot.y + (s16)((s32)((seed >> 16) & 0xFFFF) * 0x3200 / 0x10000 - 0x1900);
        x = player->actor.world.pos.x + Math_SinS(yaw) * d;
        z = player->actor.world.pos.z + Math_CosS(yaw) * d;
        {
            // the bed by a raycast from above the water (the fish's water map may not be built yet)
            CollisionPoly* poly;
            Vec3f from;

            from.x = x;
            from.y = player->actor.world.pos.y + 1500.0f;
            from.z = z;
            floor = BgCheck_EntityRaycastDown1(&play->colCtx, &poly, &from);
            if (floor <= BGCHECK_Y_MIN + 1.0f) {
                continue;
            }
            (void)surface;
        }
        p->prop.x = x;
        p->prop.y = floor;
        p->prop.z = z;
        p->prop.kind = sKindsUsed[i % ARRAY_COUNT(sKindsUsed)];
        p->prop.yaw = seed >> 8;
        p->prop.scale = 56 + ((seed >> 4) & 31);
        p->prop.tint = 128;
        p->prop.tiltX = p->prop.tiltZ = 0;
        {
            f32 s = p->prop.scale * (65536.0f / 64.0f);
            s16 a = p->prop.yaw << 8;
            s32 c = (s32)(Math_CosS(a) * s);
            s32 sn = (s32)(Math_SinS(a) * s);
            Mtx* mtx = &p->mtx;

            MTX_SET(mtx, 0, 0, c);
            MTX_SET(mtx, 0, 1, 0);
            MTX_SET(mtx, 0, 2, -sn);
            MTX_SET(mtx, 0, 3, 0);
            MTX_SET(mtx, 1, 0, 0);
            MTX_SET(mtx, 1, 1, (s32)s);
            MTX_SET(mtx, 1, 2, 0);
            MTX_SET(mtx, 1, 3, 0);
            MTX_SET(mtx, 2, 0, sn);
            MTX_SET(mtx, 2, 1, 0);
            MTX_SET(mtx, 2, 2, c);
            MTX_SET(mtx, 2, 3, 0);
            mtx->intPart[3][0] = p->prop.x;
            mtx->intPart[3][1] = p->prop.y;
            mtx->intPart[3][2] = p->prop.z;
            mtx->intPart[3][3] = 1;
            mtx->fracPart[3][0] = mtx->fracPart[3][1] = mtx->fracPart[3][2] = mtx->fracPart[3][3] = 0;
        }
        sStressPlantCount++;
    }
    osWritebackDCache(sStressPlants, sStressPlantCount * sizeof(StressPlant));
}

void EnFishmod_StressPlantsDraw(PlayState* play, s32 count) {
    u32 light[3];
    s32 lastKind = -1;
    s32 i;

    if ((sScatter == NULL) || (GET_PLAYER(play) == NULL) || (count <= 0)) {
        return; // (none until the bench starts: Link is in place by then, and the forest is laid out ahead of him)
    }
    if (sStressPlants == NULL) {
        Stress_PlacePlants(play);
        if (sStressPlants == NULL) {
            return;
        }
    }
    {
        EnvironmentContext* env = &play->envCtx;

        for (i = 0; i < 3; i++) {
            s32 l = play->lightCtx.ambientColor[i] + (env->dirLight1.params.dir.color[i] >> 1) +
                    (env->dirLight2.params.dir.color[i] >> 1);

            light[i] = CLAMP_MAX(l, 255);
        }
    }
#if ENABLE_F3DEX3
    sSceneLights = NULL;
    if (SCATTER_ONE_CYCLE()) {
        sSceneLights = LightContext_NewLights(&play->lightCtx, play->state.gfxCtx);
        Lights_BindAll(sSceneLights, play->lightCtx.listHead, NULL);
        sSceneLights->l.l[sSceneLights->numLights].l.col[0] = sSceneLights->l.l[sSceneLights->numLights].l.colc[0] =
            sSceneLights->l.a.l.col[0];
        sSceneLights->l.l[sSceneLights->numLights].l.col[1] = sSceneLights->l.l[sSceneLights->numLights].l.colc[1] =
            sSceneLights->l.a.l.col[1];
        sSceneLights->l.l[sSceneLights->numLights].l.col[2] = sSceneLights->l.l[sSceneLights->numLights].l.colc[2] =
            sSceneLights->l.a.l.col[2];
    }
#endif
    count = CLAMP(count, 0, sStressPlantCount);
    gFishmodScatterDrawn = count;
    Fishmod_DebugPrintf(3, "forest: %d of %d plants", count, sStressPlantCount);

    OPEN_DISPS(play->state.gfxCtx, "fishmod_scatter.c", __LINE__);
    {
        Gfx* start = sStressDL[play->gameplayFrames & 1];
        Gfx* opa = start;

        s32 kind;

        // kind by kind, as the scene's props are drawn (one material each), nearest first within a kind
        for (kind = 0; kind < FISHMOD_PROP_KIND_MAX; kind++) {
            FishmodPropKindInfo* info = &sKinds[kind];

            for (i = 0; i < count; i++) {
                StressPlant* p = &sStressPlants[i];

                if (p->prop.kind != kind) {
                    continue;
                }
                if (kind != lastKind) {
                    opa = Scatter_Material(opa, info, kind, TIER_NEAR);
                    lastKind = kind;
                }
                opa = Scatter_PropColor(opa, TIER_NEAR, light[0], light[1], light[2], 255);
                gSPMatrix(opa++, &p->mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(opa++, info->model);
            }
        }
        gDPPipeSync(opa++);
        gSPTexture(opa++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        if (SCATTER_ONE_CYCLE()) {
            gDPSetCycleType(opa++, G_CYC_2CYCLE);
            gSPClearGeometryMode(opa++, G_PACKED_NORMALS);
            opa = Scatter_SceneLights(opa);
        }
        gSPEndDisplayList(opa++);
        gSPDisplayList(POLY_OPA_DISP++, start);
    }
    CLOSE_DISPS(play->state.gfxCtx, "fishmod_scatter.c", __LINE__);
}
#endif
