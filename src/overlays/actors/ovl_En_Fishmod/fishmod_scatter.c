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


// the props' textures and display lists (fishmod/tools/gen_habitat.py -> fishmod/tools/assets.py, made at build time)
#include "src/fishmod_data/fishmod_props.inc.c"
#include "src/fishmod_data/fishmod_props_kinds.h"
#include "fishmod_lod.h"
#include "fishmod_lod_tune.h"

// Lake Hylia's bed shaded under the full lake (fishmod/tools/gen_bedshade.py, made at build time)
typedef struct FishmodBedShade {
    u32 offset; // of a vertex's normal in the room
    s8 normal[3];
} FishmodBedShade;
#include "src/fishmod_data/fishmod_bedshade.h"
#include "sys_math.h" // (Math_Atan2S: without its prototype its floats were passed as doubles)

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

#define DIST_SMALL 800.0f
#define DIST_MID 1200.0f
#define DIST_FAR 1600.0f
// Props drawn per frame at most. They are drawn nearest band first (whatever their kind), so the budget runs out on
// the far ones. Each is a matrix and three commands, in buffers of the props' own (allocated with the scene's props).
#define PROP_BUDGET 900
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
#define XLU_MAX 200
// Under water the haze hides everything past about this far, whatever the kind (the measured distances can be less)
#define DIST_UNDER_FOG 900.0f
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
static s32 sEdgeNear; // the props being drawn are within PROP_EDGE_RA_DIST
#if FISHMOD_DEBUG
// what the LOD calibration (fishmod_debug.c, FISHMOD_LOD_CALIBRATE) draws everything out to
#define DIST_CALIBRATE 3000.0f
#endif

static FishmodPropKindInfo sKinds[FISHMOD_PROP_KIND_MAX];
static f32 sDistFar[2]; // the furthest any kind is drawn: above water, under it (how far the chunks are looked at)
static f32 sExtentMax; // the furthest any prop reaches from where it stands (the chunks' view test)

static void Scatter_InitKinds(void) {
#define X(id, name)                                                       \
    sKinds[FISHMOD_PROP_##id].material = gFishmodProp_##name##DLMaterial; \
    sKinds[FISHMOD_PROP_##id].model = gFishmodProp_##name##DL;            \
    sKinds[FISHMOD_PROP_##id].modelFar = gFishmodProp_##name##DLFar;
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
    {
        s32 i;

        for (i = 0; i < FISHMOD_PROP_KIND_MAX; i++) {
            sKinds[i].modelAlt = sKinds[i].model;
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
#undef X
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
    // moss mats: low on the bed, faint shapes past this, and each costs about as much to issue as a few hundred
    // scanlines (hwbench-5h): near ones only
    sKinds[FISHMOD_PROP_MOSS_PATCH].drawDist = 600.0f;
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
    sKinds[FISHMOD_PROP_KELP_FROND].faceCam = true;
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
    {
        // the big plant cards and their heights (gen_scatter.py HEIGHT)
        static const struct {
            u8 kind;
            u16 height;
        } sCards[] = {
            { FISHMOD_PROP_GRASS_TUFT, 16 },  { FISHMOD_PROP_RIBBON_WEED, 80 },  { FISHMOD_PROP_TALL_WEED, 140 },
            { FISHMOD_PROP_BROADLEAF, 40 },   { FISHMOD_PROP_BUSH_WEED, 42 },    { FISHMOD_PROP_KELP_FROND, 180 },
            { FISHMOD_PROP_VINE_STRAND, 150 }, { FISHMOD_PROP_BIG_PONDWEED, 52 }, { FISHMOD_PROP_FAN_WEED, 44 },
        };
        s32 i;

        for (i = 0; i < ARRAY_COUNT(sCards); i++) {
            sKinds[sCards[i].kind].card = true;
            sKinds[sCards[i].kind].height = sCards[i].height;
        }
    }
    sKinds[FISHMOD_PROP_DOCK_DECK].plant = false;
    sKinds[FISHMOD_PROP_DOCK_POSTS].plant = false;
    sKinds[FISHMOD_PROP_ROCK].plant = false;
    sKinds[FISHMOD_PROP_PEBBLE_PATCH].plant = false;
    sKinds[FISHMOD_PROP_MOSS_PATCH].plant = false;
    sKinds[FISHMOD_PROP_SUNKEN_LOG].plant = false;
    sKinds[FISHMOD_PROP_BRANCH].plant = false;
    sKinds[FISHMOD_PROP_OLD_POSTS].plant = false;
    {
        // how far each reaches from where it stands (the view test's room for its size): the cards their height,
        // the rest a generous default (reeds, the pond's tall reed, rocks, logs, hanging vines)
        s32 i;

        for (i = 0; i < FISHMOD_PROP_KIND_MAX; i++) {
            sKinds[i].extent = (sKinds[i].height != 0) ? sKinds[i].height : 150;
        }
        sKinds[FISHMOD_PROP_POND_REED].extent = 240;
        sKinds[FISHMOD_PROP_CATTAILS].extent = 120;
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
            sKinds[i].drawDistUnder = MIN(sKinds[i].drawDist, DIST_UNDER_FOG);
        }
        // (moss: nearer still in the haze)
        sKinds[FISHMOD_PROP_MOSS_PATCH].drawDistUnder = 500.0f;
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
static u16* sChunkStart;
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

// The props' commands, two frames' worth (the RSP reads last frame's while this one is built), in buffers of their
// own rather than the frame's display buffer, which the scene and the fish need.
typedef struct ScatterFrame {
    Gfx opa[PROP_BUDGET * 4 + MAT_MAX * 16 + 96]; // (and the dock, and the far edge mode's material)
    Gfx xlu[XLU_MAX * 3 + MAT_MAX_XLU * 16 + 4];
    Mtx sway[SWAY_MAX];
} ScatterFrame;
// What's drawn and the props' matrices: made when the camera has moved, and kept while it holds still (fishing,
// mostly), so most frames only send the commands again. Two sets: a new one is made while the RSP may still be
// reading the last.
typedef struct ScatterSet {
    Mtx mtx[PROP_BUDGET];
    u16 prop[PROP_BUDGET]; // in drawing order: nearest band first, kind by kind within a band
    u8 tier[PROP_BUDGET];  // ScatterTier, as its matrix was made
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
    sDryMade = false;
    sBedShadedRoom = NULL;
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

static Mtx* Scatter_Matrix(FishmodProp* prop, s32 flatScale, s16 yaw) {
    Mtx* mtx = &sBufs->set[sSet].mtx[sMtxUsed++];
    f32 s = prop->scale * (65536.0f / 64.0f);
    s32 c = (s32)(Math_CosS(yaw) * s);
    s32 sn = (s32)(Math_SinS(yaw) * s);

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
        return mtx;
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
    return mtx;
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

        room[shade->offset + 0] = shade->normal[0];
        room[shade->offset + 1] = shade->normal[1];
        room[shade->offset + 2] = shade->normal[2];
        last = MAX(last, shade->offset + 3);
    }
    osWritebackDCache(room, last);
    sBedShadedRoom = room;
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
static Gfx* Scatter_Material(Gfx* gfx, FishmodPropKindInfo* info, s32 kind, s32 tier) {
    gfx = Gfx_SetupDL(gfx, SETUPDL_25);
    if (info->xlu || (kind == FISHMOD_PROP_LILY_PAD)) {
        // vanilla's own materials (lit by the scene's lights, like the Fishing Pond's props)
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
    if (kind == FISHMOD_PROP_ROCK) {
        gSPSetGeometryMode(gfx++, G_CULL_BACK); // (solid: their far sides cost nothing)
    }
    if (tier == TIER_IMP) {
        gSPDisplayList(gfx++, info->materialImp);
    } else if (tier == TIER_HALF) {
        gSPDisplayList(gfx++, info->materialHalf);
    } else {
        gSPDisplayList(gfx++, info->material);
    }
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

static void Scatter_Build(PlayState* play, Vec3f* eyeP, f32 lookX, f32 lookY, f32 lookZ) {
    static u16 sVisible[256];                       // chunks near the camera
    static f32 sChunkDistSq[ARRAY_COUNT(sVisible)]; // their centres' distance from the eye, squared
    static u16 sCand[VISIBLE_MAX];                  // props in view (kind by kind)
    static u8 sCandBand[VISIBLE_MAX];               // and their distance bands
    static u16 sOrdered[VISIBLE_MAX];               // nearest band first, kind by kind within a band
    static u16 sKindStart[ARRAY_COUNT(sVisible)][FISHMOD_PROP_KIND_MAX + 1]; // each kind's first prop in the chunk
    Vec3f eye = *eyeP;
    s32 bandStart[BANDS + 1];
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

    Scatter_TunedDistances(under);
#if FISHMOD_DEBUG
    // the rotation check (FISHMOD_LOD_ROTATE) draws a reference with a much wider cone and nothing cut by distance
    if (gFishmodLodTest.active && gFishmodLodTest.wide) {
        viewCos = 0.3f;
        viewRoom = 400.0f;
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
            front = mx * lookX + my * lookY + mz * lookZ + chunk * 0.71f + PROP_RADIUS + sExtentMax + viewRoom;
            if (((front < 0.0f) || (SQ(front) < SQ(viewCos) * (d + SQ(my)))) && (d > SQ(chunk * 1.5f))) {
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
                }
                if (candCount < VISIBLE_MAX) {
                    sCandBand[candCount] = CLAMP_MAX((s32)(sqrtf(dSq) * (1.0f / BAND_WIDTH)), BANDS - 1);
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
        sOrdered[bandStart[sCandBand[n]]++] = sCand[n];
    }
    // into the other set, as far as the budget (and the room for material changes) goes
    sSet ^= 1;
    sMtxUsed = 0;
    set = &sBufs->set[sSet];
    set->count = 0;
    // the single cards face the camera: they turn about their own up, by the look's direction across the ground
    sBillboardYaw = Math_Atan2S(-lookZ, -lookX);
    sLodScale = under ? UNDER_LOD_SCALE : 1.0f;
    for (n = 0; (n < candCount) && (budget > 0); n++) {
        FishmodProp* prop = &sProps[sOrdered[n]];
        s32 k = prop->kind;
        s32 tier = Scatter_Tier(prop, &sKinds[k], k, SQ(prop->x - eye.x) + SQ(prop->z - eye.z), eye.y);
        s32 key = k + tier * FISHMOD_PROP_KIND_MAX; // (a material per kind and tier)

        if (sKinds[k].xlu) {
            if ((xluLeft <= 0) || ((k != lastXlu) && (matsXlu >= MAT_MAX_XLU))) {
                continue;
            }
            xluLeft--;
            if (k != lastXlu) {
                matsXlu++;
                lastXlu = k;
            }
        } else {
            if ((key != lastOpa) && (mats >= MAT_MAX)) {
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
        {
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
            Scatter_Matrix(prop, k == FISHMOD_PROP_LILY_PAD, yaw);
        }
        set->tier[set->count] = tier;
        set->prop[set->count++] = sOrdered[n];
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
    s32 lastOpa = -1; // the material each list has now (the two are interleaved: nearest first, whatever the kind)
    s32 lastXlu = -1;
    s32 lastNear = -1; // the edge mode the opaque list has now
    s32 n;
    Gfx* opa;
    Gfx* xlu;
    u32 light[3];
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
#endif
        ((SQ(eye.x - sSetEye.x) + SQ(eye.y - sSetEye.y) + SQ(eye.z - sSetEye.z)) > SQ(REUSE_MOVE)) ||
        ((lookX * sSetLook.x + lookY * sSetLook.y + lookZ * sSetLook.z) < REUSE_TURN_COS)) {
        Scatter_Build(play, &eye, lookX, lookY, lookZ);
#if FISHMOD_DEBUG
        sSetBench = gFishmodBench;
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
    Scatter_Sway(play, frame);
#if FISHMOD_DEBUG
    gFishmodScatterDrawn = set->count;
#endif
    for (n = 0; n < set->count; n++) {
        FishmodProp* prop = &sProps[set->prop[n]];
        FishmodPropKindInfo* info;
        Gfx** disp;
        f32 dSq;
        s32 tier;
        s32 key;

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
        if (!info->xlu) {
            u32 t = (prop->tint > 255 / 2) ? 255 : prop->tint * 2;
            s32 a = 255;

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
            if (kind == FISHMOD_PROP_ROCK) {
                // each rock a slightly different stone: neutral, warmer or cooler grey (by where it stands)
                static const u8 sStone[4][3] = { { 255, 255, 255 }, { 255, 240, 222 }, { 228, 240, 255 }, { 240, 248, 232 } };
                const u8* c = sStone[((u32)(prop->x * 7 + prop->z * 13) >> 4) & 3];

                gDPSetPrimColor(opa++, 0, 0, (t * light[0] * c[0]) >> 16, (t * light[1] * c[1]) >> 16,
                                (t * light[2] * c[2]) >> 16, a);
            } else {
                gDPSetPrimColor(opa++, 0, 0, (t * light[0]) >> 8, (t * light[1]) >> 8, (t * light[2]) >> 8, a);
            }
        }
        gSPMatrix((*disp)++, &set->mtx[n], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        if (info->sway != 0) {
            // neighbours share a phase: a wave through the bed
            s32 ph = ((prop->x + prop->z) >> 7) & (SWAY_PHASES - 1);

            gSPMatrix((*disp)++, &frame->sway[kind * SWAY_PHASES + ph], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        }
        gSPDisplayList((*disp)++, (tier == TIER_IMP)    ? info->modelImp
                                  : (tier == TIER_HALF) ? info->modelHalf
                                  : (tier == TIER_FAR)  ? info->modelFar
                                                        : info->model);
    }
    if (lastOpa >= 0) {
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
            opa = Scatter_Material(opa, &sKinds[k], k, TIER_NEAR);
            gDPSetPrimColor(opa++, 0, 0, light[0], light[1], light[2], 255);
            gSPMatrix(opa++, &sDockMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(opa++, sKinds[k].model);
        }
        gSPTexture(opa++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    }
    gDPPipeSync(opa++);
    gDPSetTextureLUT(opa++, G_TT_NONE); // (a 4-bit texture's palette mode off again for what's drawn next)
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

