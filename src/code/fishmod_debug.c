/*
 * File: fishmod_debug.c
 * Description: fishmod debug harness. Only compiled into FISHMOD_DEBUG builds.
 *  - Boots straight into a test scene with a debug save and the fishing rod on C-Left
 *  - Optionally plays a scripted input sequence (FISHMOD_SCRIPT), then hands control back to the controller
 *  - Draws debug text lines that any code can set with Fishmod_DebugPrintf
 */

#include "dma.h"
#include "fishmod.h"

#if FISHMOD_DEBUG

#include "libc64/sprintf.h"
#include "array_count.h"
#include "scene.h"
#include "transition.h"
#include "ultra64/libc.h"
#include "libu64/gfxprint.h"
#include "stdarg.h"
#include "controller.h"
#include "gfx.h"
#include "item.h"
#include "play_state.h"
#include "pause.h"
#include "ss_sram.h"
#include "regs.h"
#include "player.h"
#include "save.h"
#include "environment.h"
#include "sequence.h"
#include "seqcmd.h"
#include "game.h"
#include "actor_profile.h"
#include "z_actor_dlftbls.h"
#include "camera.h"
#include "interface.h"
#include "z_lib.h"
#include "sys_math.h" // (Math_Atan2S: without its prototype its floats were passed as doubles)
#include "speed_meter.h"
#include "ultra64/convert.h"
#include "bgcheck.h"
#include "segmented_address.h"
#include "libc64/qrand.h"
#include "gfx_setupdl.h"
#include "sfx.h"
#include "libc/math.h"
#include "letterbox.h"
#include "actor.h"
#include "libc64/malloc.h"
#include "message.h"

void Sram_InitDebugSave(void);

// A stop on a camera tour (FISHMOD_TOUR in a script): where the camera is, what it looks at, and a caption
typedef struct FishmodTourStop {
    Vec3f eye;
    Vec3f at;
    const char* title;
    const char* lines[3];
} FishmodTourStop;

// A spot for the area bench (FISHMOD_BENCH_AREA): where Link walks, and from how high to look for the ground there
typedef struct FishmodAreaSpot {
    s16 x;
    s16 z;
    s16 probeY;
    const char* name;
} FishmodAreaSpot;

// A viewpoint for the LOD sweep (fishmod/tools/gen_lod_views.py)
typedef struct FishmodLodView {
    Vec3s eye;
    Vec3s at;
    u8 zone;
    u8 under;
} FishmodLodView;

#ifdef FISHMOD_SCRIPT
#include FISHMOD_SCRIPT
#else
// No script: just place Link at the Kokiri test spot and hand over control.
static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
#endif

#ifdef FISHMOD_PHOTO
/*
 * Photo booth for wiki renders: shows each species in turn, from the side and from a three-quarter view,
 * high above the test spot on a plain backdrop. fishmod/tools/photo_capture.py screenshots each photo
 * using the marker color drawn in the corner.
 */
static u8 sPhotoSpecies[] = {
    FISHMOD_SPECIES_GREENGILL,    FISHMOD_SPECIES_HYRULE_BASS, FISHMOD_SPECIES_ORDON_CATFISH,
    FISHMOD_SPECIES_HYLIAN_LOACH, FISHMOD_SPECIES_SANKE_CARP,  FISHMOD_SPECIES_HYRULE_MINNOW,
};
#define PHOTO_POSES 2
#define PHOTO_FRAMES 100

static s32 sPhotoIndex = -1;
static s32 sPhotoTimer;
static Actor* sPhotoFish;
// Size line-up after the portraits: every species at its typical size, smallest at the top.
static u8 sLineupOrder[] = {
    FISHMOD_SPECIES_HYRULE_MINNOW, FISHMOD_SPECIES_GREENGILL,  FISHMOD_SPECIES_HYLIAN_LOACH,  FISHMOD_SPECIES_HYRULE_BASS,
    FISHMOD_SPECIES_SANKE_CARP, FISHMOD_SPECIES_ORDON_CATFISH,
};
static Actor* sLineupFish[ARRAY_COUNT(sLineupOrder)];
static f32 sMeasuredExtent[FISHMOD_SPECIES_MAX];
static Vec3f sMeasuredOffset[FISHMOD_SPECIES_MAX]; // drawn center minus home position
static s16 sPhotoCamId = SUB_CAM_ID_DONE;
static Vec3f sPhotoEye;
static Vec3f sPhotoAt;

static void Fishmod_PhotoUpdate(PlayState* play) {
    const FishmodSpecies* species;
    s32 pose;
    f32 length;
    f32 dist;

    if (sPhotoCamId == SUB_CAM_ID_DONE) {
        sPhotoCamId = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, sPhotoCamId, CAM_STAT_ACTIVE);
        gFishmodPhoto.active = true;
    }

    play->envCtx.skyboxDisabled = true;
    play->envCtx.sunMoonDisabled = true;
    // studio lighting: lift the ambient and key light so colors read clearly
    play->envCtx.adjAmbientColor[0] = play->envCtx.adjAmbientColor[1] = play->envCtx.adjAmbientColor[2] = 70;
    play->envCtx.adjLight1Color[0] = play->envCtx.adjLight1Color[1] = play->envCtx.adjLight1Color[2] = 60;
    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING);

    if ((sPhotoIndex < 0) || (++sPhotoTimer >= PHOTO_FRAMES)) {
        s32 numPortraits = (s32)ARRAY_COUNT(sPhotoSpecies) * PHOTO_POSES;
        s32 i;

        // remember how big each fish drew, for the line-up
        if ((sPhotoIndex >= 0) && (sPhotoIndex < numPortraits) && (gFishmodPhoto.extent > 0.0f) &&
            (sPhotoFish != NULL)) {
            s32 sp = sPhotoSpecies[sPhotoIndex / PHOTO_POSES];

            sMeasuredExtent[sp] = gFishmodPhoto.extent;
            sMeasuredOffset[sp].x = gFishmodPhoto.center.x - sPhotoFish->world.pos.x;
            sMeasuredOffset[sp].y = gFishmodPhoto.center.y - sPhotoFish->world.pos.y;
            sMeasuredOffset[sp].z = gFishmodPhoto.center.z - sPhotoFish->world.pos.z;
        }

        sPhotoTimer = 0;
        sPhotoIndex++;
        if (sPhotoIndex > numPortraits) {
            sPhotoIndex = 0;
        }
        if (sPhotoFish != NULL) {
            Actor_Kill(sPhotoFish);
            sPhotoFish = NULL;
        }
        for (i = 0; i < ARRAY_COUNT(sLineupFish); i++) {
            if (sLineupFish[i] != NULL) {
                Actor_Kill(sLineupFish[i]);
                sLineupFish[i] = NULL;
            }
        }

        if (sPhotoIndex < numPortraits) {
            sPhotoFish =
                Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, sPhotoPos.x, sPhotoPos.y, sPhotoPos.z, 0, 0, 0,
                            FISHMOD_PARAMS(FISHMOD_TYPE_FISH, sPhotoSpecies[sPhotoIndex / PHOTO_POSES]));
            if (sPhotoFish != NULL) {
                sPhotoFish->room = -1;
            }
        } else {
            f32 rowY = sPhotoPos.y;

            for (i = 0; i < ARRAY_COUNT(sLineupOrder); i++) {
                s32 sp = sLineupOrder[i];
                f32 rowHeight = sMeasuredExtent[sp] * 0.25f + 2.0f;

                rowY -= rowHeight;
                sLineupFish[i] = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, sPhotoPos.x - sMeasuredOffset[sp].x,
                                             rowY - sMeasuredOffset[sp].y, sPhotoPos.z - sMeasuredOffset[sp].z, 0, 0, 0,
                                             FISHMOD_PARAMS(FISHMOD_TYPE_FISH, sp));
                if (sLineupFish[i] != NULL) {
                    sLineupFish[i]->room = -1;
                }
                rowY -= rowHeight;
            }
        }
        gFishmodPhoto.marker = sPhotoIndex + 1;
        gFishmodPhoto.extent = 0.0f;
    }

    if (sPhotoIndex >= (s32)ARRAY_COUNT(sPhotoSpecies) * PHOTO_POSES) {
        // line-up: frame the whole column
        f32 top = sPhotoPos.y;
        f32 bottom = sPhotoPos.y;
        f32 widest = 0.0f;
        s32 i;

        for (i = 0; i < ARRAY_COUNT(sLineupOrder); i++) {
            s32 sp = sLineupOrder[i];

            bottom -= 2.0f * (sMeasuredExtent[sp] * 0.25f + 2.0f);
            widest = CLAMP_MIN(widest, sMeasuredExtent[sp]);
        }
        gFishmodPhoto.yaw = sPhotoYaw;
        gFishmodPhoto.pitch = 0;
        sPhotoAt.x = sPhotoPos.x;
        sPhotoAt.y = (top + bottom) * 0.5f;
        sPhotoAt.z = sPhotoPos.z;
        dist = CLAMP_MIN(widest * 1.25f * 1.35f, (top - bottom) * 1.05f);
        sPhotoEye.x = sPhotoAt.x;
        sPhotoEye.y = sPhotoAt.y;
        sPhotoEye.z = sPhotoAt.z + dist;
        Play_SetCameraAtEye(play, sPhotoCamId, &sPhotoAt, &sPhotoEye);
        return;
    }

    species = &gFishmodSpecies[sPhotoSpecies[sPhotoIndex / PHOTO_POSES]];
    pose = sPhotoIndex % PHOTO_POSES;

    // frame the fish from the bounds it reported in its last draw (limb origins, so pad a little)
    length = gFishmodPhoto.extent * 1.25f;
    if (length < 5.0f) {
        length = 20.0f;
    }
    dist = length * 1.35f;

    gFishmodPhoto.yaw = sPhotoYaw;
    gFishmodPhoto.pitch = 0;

    if (gFishmodPhoto.extent > 0.0f) {
        sPhotoAt = gFishmodPhoto.center;
    } else {
        sPhotoAt = sPhotoPos;
    }
    if (pose == 0) {
        sPhotoEye.x = sPhotoAt.x;
        sPhotoEye.y = sPhotoAt.y + dist * 0.05f;
        sPhotoEye.z = sPhotoAt.z + dist;
    } else {
        sPhotoEye.x = sPhotoAt.x - dist * 0.35f;
        sPhotoEye.y = sPhotoAt.y + dist * 0.7f;
        sPhotoEye.z = sPhotoAt.z + dist * 0.62f;
    }
    Play_SetCameraAtEye(play, sPhotoCamId, &sPhotoAt, &sPhotoEye);
}
#endif

#ifdef FISHMOD_ANIM_STUDIO
/*
 * Animation studio: each species in turn as a row of three fish on the photo backdrop (full rig, reduced rig, and a
 * small one on the reduced rig), held in place while a fixed program puts them through cruising, fast swimming,
 * turns, startles, a hooked thrash and hovering, seen from above and then from a three-quarter view. The corner
 * marker (gFishmodPhoto.marker) tells fishmod/tools/anim_capture.py which species and view is on screen.
 */
#ifdef FISHMOD_STUDIO_ORBIT
// one species seen from all round (for finding angles it vanishes from): above, three-quarter, nose-on, from
// behind, from below, low from the side
static u8 sStudioSpecies[] = { FISHMOD_STUDIO_ORBIT };
#define STUDIO_VIEWS 6
#else
static u8 sStudioSpecies[] = {
    FISHMOD_SPECIES_GREENGILL,    FISHMOD_SPECIES_HYRULE_BASS, FISHMOD_SPECIES_ORDON_CATFISH,
    FISHMOD_SPECIES_HYLIAN_LOACH, FISHMOD_SPECIES_SANKE_CARP,  FISHMOD_SPECIES_HYRULE_MINNOW,
};
#define STUDIO_VIEWS 2
#endif
#define STUDIO_FISH 3

typedef struct StudioPhase {
    s16 frames;
    s16 swimMag;
    s16 phaseStep; // hundredths
    s16 yawSwing;  // amplitude of a yaw sweep
    s16 yawRate;
    s16 jaw;
    s16 roll;
    s8 startleEvery; // frames between startles, 0 = none
} StudioPhase;

static StudioPhase sStudioProgram[] = {
    { 70, 1500, 25, 0, 0, 0, 0, 0 },               // cruise
    { 50, 2200, 45, 0, 0, 0, 0, 0 },               // fast
    { 90, 1500, 30, 0x2800, 0x500, 0, 0, 0 },      // turning left and right
    { 60, 1500, 25, 0, 0, 0, 0, 30 },              // startles
    { 70, 2800, 60, 0x1000, 0x1800, 0x1800, 0x800, 0 }, // hooked thrash
    { 50, 350, 10, 0, 0, 0, 0, 0 },                // hovering
};

static Vec3f sStudioPos = { 1033.0f, 900.0f, 172.0f };
static s32 sStudioSegment = -1; // species * STUDIO_VIEWS + view
static s32 sStudioFrame;
static s16 sStudioCamId = SUB_CAM_ID_DONE;
static f32 sStudioSpacing;
static Vec3f sStudioAt;

static void Fishmod_StudioSpawn(PlayState* play, s32 species) {
    const FishmodSpecies* sp = &gFishmodSpecies[species];
    f32 typical = sp->lengthTypical;
    f32 small = CLAMP_MAX(typical * 0.6f, 7.0f);
    s32 i;

    for (i = 0; i < STUDIO_FISH; i++) {
        if (gFishmodStudio.fish[i] != NULL) {
            Actor_Kill(gFishmodStudio.fish[i]);
            gFishmodStudio.fish[i] = NULL;
        }
    }
    // side by side across X, all facing +Z (up the screen from above)
    sStudioSpacing = typical * 1.64f * 1.3f + 6.0f;
    for (i = 0; i < STUDIO_FISH; i++) {
        gFishmodStudio.spawnLength = (i == 2) ? small : typical;
#ifdef FISHMOD_STUDIO_ORBIT
        {
            // to compare: dark and cool; bright, warm and mirrored; plain colours (koi: three different patterns)
            static const u8 sLooks[] = { 0x00, 0x7F, 0xDB };

            gFishmodStudio.spawnLook = sLooks[i % 3];
        }
#else
        gFishmodStudio.spawnLook = 0x1B;
#endif
        gFishmodStudio.rig[i] = (i == 0) ? 1 : 2;
        gFishmodStudio.fish[i] =
            Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, sStudioPos.x + (i - 1) * sStudioSpacing, sStudioPos.y,
                        sStudioPos.z, 0, 0, 0, FISHMOD_PARAMS(FISHMOD_TYPE_FISH, species));
        if (gFishmodStudio.fish[i] != NULL) {
            gFishmodStudio.fish[i]->room = -1;
        }
    }
    gFishmodStudio.spawnLength = 0.0f;
}

static void Fishmod_StudioUpdate(PlayState* play) {
    s32 programFrames = 0;
    s32 t;
    s32 i;
    StudioPhase* phase = NULL;
    Vec3f at;
    Vec3f eye;
    f32 dist;

    if (sStudioCamId == SUB_CAM_ID_DONE) {
        sStudioCamId = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, sStudioCamId, CAM_STAT_ACTIVE);
        gFishmodPhoto.active = true;
        gFishmodStudio.active = true;
    }
    play->envCtx.skyboxDisabled = true;
    play->envCtx.sunMoonDisabled = true;
    play->envCtx.adjAmbientColor[0] = play->envCtx.adjAmbientColor[1] = play->envCtx.adjAmbientColor[2] = 70;
    play->envCtx.adjLight1Color[0] = play->envCtx.adjLight1Color[1] = play->envCtx.adjLight1Color[2] = 60;
    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING);

    for (i = 0; i < ARRAY_COUNT(sStudioProgram); i++) {
        programFrames += sStudioProgram[i].frames;
    }
    if ((sStudioSegment < 0) || (++sStudioFrame >= programFrames)) {
        sStudioFrame = 0;
        sStudioSegment++;
        if (sStudioSegment >= (s32)ARRAY_COUNT(sStudioSpecies) * STUDIO_VIEWS) {
            sStudioSegment = 0;
        }
        if ((sStudioSegment % STUDIO_VIEWS) == 0) {
            Fishmod_StudioSpawn(play, sStudioSpecies[sStudioSegment / STUDIO_VIEWS]);
        }
        gFishmodPhoto.marker = sStudioSegment + 1;
    }

    // where in the program
    t = sStudioFrame;
    for (i = 0; i < ARRAY_COUNT(sStudioProgram); i++) {
        if (t < sStudioProgram[i].frames) {
            phase = &sStudioProgram[i];
            break;
        }
        t -= sStudioProgram[i].frames;
    }
    gFishmodStudio.swimMag = phase->swimMag;
    gFishmodStudio.swimPhaseStep = phase->phaseStep * 0.01f;
    gFishmodStudio.yaw = (s16)(Math_SinS(t * phase->yawRate) * phase->yawSwing);
    gFishmodStudio.pitch = 0;
    gFishmodStudio.roll = (s16)(Math_SinS(t * 0x1300) * phase->roll);
    gFishmodStudio.jawRot = phase->jaw;
    gFishmodStudio.startle = 0;
    if ((phase->startleEvery != 0) && ((t % phase->startleEvery) == 5)) {
        gFishmodStudio.startle = ((t / phase->startleEvery) & 1) ? -8 : 8;
    }

    // camera: aimed at the middle fish's drawn center (the models' origins aren't at their middles), taken once it
    // has been drawn and held for the segment; from above (a touch behind, so the view has an up direction), then
    // three-quarter from the front
    if ((sStudioFrame == 3) || ((sStudioSegment % STUDIO_VIEWS) == 0) && (sStudioFrame == 0)) {
        sStudioAt = (sStudioFrame == 3) ? gFishmodPhoto.center : sStudioPos;
    }
    at = sStudioAt;
    // a narrow lens from farther away: nearly flat, so the fish at the sides are seen from straight above too
    dist = sStudioSpacing * 1.7f * 2.4f;
    if ((sStudioSegment % STUDIO_VIEWS) == 0) {
        eye.x = at.x;
        eye.y = at.y + dist;
        eye.z = at.z - dist * 0.08f;
    } else if ((sStudioSegment % STUDIO_VIEWS) == 1) {
        dist *= 1.3f;
        eye.x = at.x + dist * 0.45f;
        eye.y = at.y + dist * 0.45f;
        eye.z = at.z + dist * 0.75f;
    } else {
        // (FISHMOD_STUDIO_ORBIT) nose-on, from behind, from below, low from the side
        static const f32 sDir[4][3] = { { 0.05f, 0.1f, 1.0f }, { 0.05f, 0.1f, -1.0f }, { 0.08f, -1.0f, 0.1f },
                                        { 1.0f, -0.2f, 0.05f } };
        const f32* d = sDir[(sStudioSegment % STUDIO_VIEWS) - 2];

        // close up nose-on (into the mouth) and from the side
        dist *= (((sStudioSegment % STUDIO_VIEWS) == 2) || ((sStudioSegment % STUDIO_VIEWS) == 5)) ? 0.3f : 1.3f;
        eye.x = at.x + d[0] * dist;
        eye.y = at.y + d[1] * dist;
        eye.z = at.z + d[2] * dist;
        if (((sStudioSegment % STUDIO_VIEWS) == 5) && (gFishmodStudio.fish[0] != NULL)) { // the full-rig fish, from the side
            at = gFishmodStudio.fish[0]->world.pos;
            eye.x = at.x - dist;
            eye.y = at.y - 0.2f * dist;
            eye.z = at.z + 0.05f * dist;
        }
    }
    Play_SetCameraAtEye(play, sStudioCamId, &at, &eye);
    Play_SetCameraFov(play, sStudioCamId, 25.0f);
}
#endif

Vec3f gFishmodDebugFocus;
FishmodLodTest gFishmodLodTest;
#ifdef FISHMOD_VIEW_SCHOOL
s32 gFishmodDebugFocusSchool = true;
#else
s32 gFishmodDebugFocusSchool = false;
#endif
s16 gFishmodDebugFocusYaw;

#ifdef FISHMOD_VIEW
// Fixed camera for watching the pond (FISHMOD_VIEW in a script defines sViewEye / sViewAt)
static s16 sViewCamId = SUB_CAM_ID_DONE;

static void Fishmod_ViewUpdate(PlayState* play) {
    if (sViewCamId == SUB_CAM_ID_DONE) {
        sViewCamId = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, sViewCamId, CAM_STAT_ACTIVE);
    }
#ifdef FISHMOD_VIEW_FOLLOW
    // chase the nearest fish from behind and above
    {
        static Vec3f sEye;
        static Vec3f sAt;
        Vec3f eye;

        eye.x = gFishmodDebugFocus.x - Math_SinS(gFishmodDebugFocusYaw) * 45.0f;
        eye.y = gFishmodDebugFocus.y + 35.0f;
        eye.z = gFishmodDebugFocus.z - Math_CosS(gFishmodDebugFocusYaw) * 45.0f;
        Math_ApproachF(&sEye.x, eye.x, 0.1f, 10.0f);
        Math_ApproachF(&sEye.y, eye.y, 0.1f, 10.0f);
        Math_ApproachF(&sEye.z, eye.z, 0.1f, 10.0f);
        Math_ApproachF(&sAt.x, gFishmodDebugFocus.x, 0.2f, 10.0f);
        Math_ApproachF(&sAt.y, gFishmodDebugFocus.y, 0.2f, 10.0f);
        Math_ApproachF(&sAt.z, gFishmodDebugFocus.z, 0.2f, 10.0f);
        Play_SetCameraAtEye(play, sViewCamId, &sAt, &sEye);
        return;
    }
#endif
    Play_SetCameraAtEye(play, sViewCamId, &sViewAt, &sViewEye);
}
#endif

#ifdef FISHMOD_TOUR
/*
 * Camera tour: a free camera glides between the script's stops (sTourStops) and a Navi-style caption names each.
 * A: next stop, B: the one before, stick: turn and raise the camera about what it looks at, L: debug text.
 * The controller is kept from Link while the tour runs. FISHMOD_TOUR_AUTO: move on by itself (for screenshots).
 */
#define TOUR_GLIDE 110
#ifdef FISHMOD_EDGE_AB
#ifndef FISHMOD_EDGE_AB_FLAGS
#define FISHMOD_EDGE_AB_FLAGS FISHMOD_BENCH_ALTEDGE // what the A/B switches (bench flags)
#endif
static s32 sEdgeAbPhase = -1; // -1 not ready, 0 the default edge mode on screen, 1 the other
#endif
static s16 sTourCamId = SUB_CAM_ID_DONE;
static s32 sTourStop;
static s32 sTourFrame;
static Vec3f sTourFromEye;
static Vec3f sTourFromAt;
static Vec3f sTourEye;
static Vec3f sTourAt;
static f32 sTourYaw;
static f32 sTourRise;
static u16 sTourPress;
static s8 sTourStickX;
static s8 sTourStickY;
#if defined(FISHMOD_TOUR_START) || defined(FISHMOD_BENCH_SHOWTEXT)
static s32 sTourDebugText = true; // (held at one stop: for profiling, so the numbers show)
#else
static s32 sTourDebugText;
#endif

#ifdef FISHMOD_LOD_CALIBRATE
static void Fishmod_LodUpdate(PlayState* play);
#endif

static void Fishmod_TourGo(s32 stop) {
    s32 n = ARRAY_COUNT(sTourStops);

    sTourStop = ((stop % n) + n) % n;
    sTourFrame = 0;
    sTourFromEye = sTourEye;
    sTourFromAt = sTourAt;
    sTourYaw = 0.0f;
    sTourRise = 0.0f;
    Sfx_PlaySfxCentered(NA_SE_VO_NAVY_HELLO);
}

static void Fishmod_TourUpdate(PlayState* play) {
    FishmodTourStop* stop;
    f32 t;
    f32 s;
    f32 arc;
    f32 dx;
    f32 dz;
    f32 c;
    f32 sn;
    Vec3f eye;
    Vec3f at;

    if (sTourCamId == SUB_CAM_ID_DONE) {
        sTourCamId = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, sTourCamId, CAM_STAT_ACTIVE);
        sTourEye = sTourStops[0].eye;
        sTourAt = sTourStops[0].at;
#ifdef FISHMOD_TOUR_START
        Fishmod_TourGo(FISHMOD_TOUR_START); // (straight to one stop, for looking at it)
        sTourEye = sTourStops[FISHMOD_TOUR_START].eye;
        sTourAt = sTourStops[FISHMOD_TOUR_START].at;
#else
        Fishmod_TourGo(0);
#endif
    }
    // something else in the scene can hand the view back to the main camera (or clear ours): take it back
    if (play->cameraPtrs[sTourCamId] == NULL) {
        sTourCamId = Play_CreateSubCamera(play);
    }
    // the owl talks to Link by the bench (and takes the camera): not on the tour
    {
        Actor* actor = play->actorCtx.actorLists[ACTORCAT_NPC].head;

        for (; actor != NULL; actor = actor->next) {
            if (actor->id == ACTOR_EN_OWL) {
                Actor_Kill(actor);
            }
        }
    }
    Letterbox_SetSizeTarget(0);
    if (play->activeCamId != sTourCamId) {
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, sTourCamId, CAM_STAT_ACTIVE);
    }
    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING);

    if (sTourPress & BTN_A) {
        Fishmod_TourGo(sTourStop + 1);
    } else if (sTourPress & BTN_B) {
        Fishmod_TourGo(sTourStop - 1);
    }
    if (sTourPress & BTN_L) {
        sTourDebugText ^= 1;
    }
    sTourPress = 0;
#ifdef FISHMOD_TOUR_AUTO
    if (sTourFrame >= FISHMOD_TOUR_AUTO) {
        Fishmod_TourGo(sTourStop + 1);
    }
#endif
#ifdef FISHMOD_EDGE_AB
    {
        // An exact A/B of the props' two edge modes (fishmod/tools/edge_ab_capture.py): once the camera has
        // settled, everything that moves is frozen (sway, water, sky, actors) and the modes take turns
        static s32 sFrozenFrames;
        static s32 sFrozenTime;
        static s16 sFrozenSky;
        static s32 sFrozenState;
        s32 since = sTourFrame - (TOUR_GLIDE + 40);

        if (since == 0) {
            sFrozenFrames = play->gameplayFrames;
            sFrozenTime = gSaveContext.save.dayTime;
            sFrozenSky = play->skyboxCtx.rot.y;
            sFrozenState = play->state.frames;
        }
        if (since >= 0) {
            play->gameplayFrames = sFrozenFrames;
            gSaveContext.save.dayTime = sFrozenTime;
            play->skyboxCtx.rot.y = sFrozenSky;
            play->state.frames = sFrozenState;
            play->haltAllActors = true;
            gFishmodBench = ((since / 60) & 1) ? FISHMOD_EDGE_AB_FLAGS : 0;
            // the marker shows the mode once the new one has been on screen for a while
            sEdgeAbPhase = ((since % 60) >= 30) ? ((since / 60) & 1) : -1;
        } else {
            play->haltAllActors = false;
            gFishmodBench = 0;
            sEdgeAbPhase = -1;
        }
    }
#endif
    sTourYaw += sTourStickX * 0.0006f;
    sTourRise = CLAMP(sTourRise + sTourStickY * 0.08f, -150.0f, 400.0f);

    // glide (eased), arcing up over long hops so the camera doesn't cut through the land
    stop = &sTourStops[sTourStop];
#ifdef FISHMOD_LOD_SWEEP
    {
        // the sweep's view, without gliding there
        static FishmodTourStop sSweepStop;
        extern Vec3f gFishmodLodSweepEye;
        extern Vec3f gFishmodLodSweepAt;

        sSweepStop.eye = gFishmodLodSweepEye;
        sSweepStop.at = gFishmodLodSweepAt;
        stop = &sSweepStop;
        sTourFromEye = sTourEye = stop->eye;
        sTourFromAt = sTourAt = stop->at;
        sTourYaw = sTourRise = 0.0f;
    }
#endif
    t = CLAMP_MAX(sTourFrame / (f32)TOUR_GLIDE, 1.0f);
    s = t * t * (3.0f - 2.0f * t);
    sTourFrame++;
    arc = sqrtf(SQ(stop->eye.x - sTourFromEye.x) + SQ(stop->eye.z - sTourFromEye.z)) * 0.25f;
    arc = CLAMP_MAX(arc, 700.0f) * sinf(s * M_PI);
    sTourEye.x = sTourFromEye.x + (stop->eye.x - sTourFromEye.x) * s;
    sTourEye.y = sTourFromEye.y + (stop->eye.y - sTourFromEye.y) * s;
    sTourEye.z = sTourFromEye.z + (stop->eye.z - sTourFromEye.z) * s;
    sTourAt.x = sTourFromAt.x + (stop->at.x - sTourFromAt.x) * s;
    sTourAt.y = sTourFromAt.y + (stop->at.y - sTourFromAt.y) * s;
    sTourAt.z = sTourFromAt.z + (stop->at.z - sTourFromAt.z) * s;

    // looking around: turned about the point looked at, and raised
    at = sTourAt;
    dx = sTourEye.x - at.x;
    dz = sTourEye.z - at.z;
    c = cosf(sTourYaw);
    sn = sinf(sTourYaw);
    eye.x = at.x + dx * c + dz * sn;
    eye.z = at.z - dx * sn + dz * c;
    eye.y = sTourEye.y + arc + sTourRise;
    Play_SetCameraAtEye(play, sTourCamId, &at, &eye);

    // under the surface: the water's own lights (its colour and fog), as when the main camera dives
    {
        static s32 sUnder;
        WaterBox* box;
        f32 surface;
        s32 under = BgCheck_GetWaterSurface(play, &play->colCtx, eye.x, eye.z, &surface, &box) && (eye.y < surface);

        if (under && !sUnder) {
            Environment_EnableUnderwaterLights(play, WaterBox_GetLightIndex(&play->colCtx, box));
        } else if (!under && sUnder) {
            Environment_DisableUnderwaterLights(play);
        }
        sUnder = under;
    }
#ifdef FISHMOD_LOD_CALIBRATE
    Fishmod_LodUpdate(play);
#endif
}

#ifdef FISHMOD_LOD_CALIBRATE
/*
 * LOD calibration: how far each habitat prop kind has to be drawn, measured on the game's own frames at the N64's
 * resolution. At each tour stop, with everything that moves held still (the clock, the frame count that drives
 * sway and scrolling water; no fish, no particles), the frame with every prop drawn out to 3000 units is the
 * reference, and a few more like it mark the pixels that change by themselves. Then for each kind in view, a
 * binary search finds the shortest distance it can stop at with no more than LOD_PIXELS pixels (not counting the
 * restless ones) differing by more than LOD_THRESHOLD in any 5-bit channel. Pixel by pixel, not averaged: a frame
 * that is mostly fog can't hide a prop that shows. Each kind keeps the furthest any stop needs, above water and
 * under it, and each stop is checked again with every kind at those distances together. The results go to SRAM
 * (0x7A40, after the save slots) for fishmod/tools/lod_calibrate.py, which writes include/fishmod_lod.h.
 */
#define LOD_THRESHOLD 2 // a pixel differs if a channel moves by more than this (of 31; dither moves them by 1)
#define LOD_PIXELS 2    // pixels a kind may change by stopping short
#define LOD_STEP 100    // distances tried: steps of this...
#define LOD_STEPS 30    // ...out to LOD_STEP * LOD_STEPS (the reference)
#define LOD_HOLD 5      // frames each test is drawn before its frame is read (the display runs a frame or two behind)
#define LOD_SETTLE 40   // frames after the camera arrives before the reference
#define LOD_NOISE_REFS 3

typedef enum LodState {
    LOD_ARRIVE,
    LOD_REF,
    LOD_NOISE,
    LOD_NEXT_KIND,
    LOD_SEARCH,
    LOD_VERIFY,
    LOD_DONE
} LodState;

typedef struct LodResults {
    /* 0x000 */ u32 magic; // 'FLOD'
    /* 0x004 */ u8 kinds;
    /* 0x005 */ u8 stops;   // stops done so far
    /* 0x006 */ u8 threshold;
    /* 0x007 */ u8 pixels;
    /* 0x008 */ u16 need[2][FISHMOD_LOD_KINDS]; // [above, under] water: the furthest any stop needs, units (0: never seen)
    /* 0x0C8 */ u16 noise[32];  // per stop: pixels that changed by themselves
    /* 0x108 */ u16 verify[32]; // per stop: pixels changed with every kind at its distance
    /* 0x148 */ u8 under[32];   // per stop: the camera was under water
    /* 0x168 */ u16 logCount;
    /* 0x16A */ u16 log[64][3]; // the first tests: kind, distance, pixels changed (for checking the method)
} LodResults;

static LodResults sLod;
static u16* sLodRef;   // the reference frame
static u8* sLodNoise;  // pixels that change by themselves, a bit each
static u16 sLodDrawn[FISHMOD_LOD_KINDS]; // props of each kind in the reference
static s32 sLodState = LOD_ARRIVE;
static s32 sLodTimer;
static s32 sLodPass;
static s32 sLodKind;
static s32 sLodLo;
static s32 sLodHi;
static u32 sLodFrames;
static u16 sLodTime;
static f32 sLodSkyRot;
static u32 sLodStateFrames;

static u16* sLodCur;   // the frame being tested, copied off the screen

static GraphicsContext* sLodGfxCtx;

/**
 * Copies a finished frame to `dst`: the buffer this frame is about to be drawn into, which still holds the frame
 * from two updates ago, whole (its drawing ended before the last one began, and this one's starts after this
 * update). Not the buffer on the screen: the next frame can already be being drawn into that one.
 */
static s32 Lod_Grab(u16* dst) {
    u16* fb = (u16*)OS_PHYSICAL_TO_K0(OS_K0_TO_PHYSICAL(sLodGfxCtx->curFrameBuffer));

    // through the cache (lines at a time: several times faster than one read each), the stale lines dropped first
    osInvalDCache(fb, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
    bcopy(fb, dst, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
    return true;
}

/** Pixels differing from the reference by more than LOD_THRESHOLD; with `markNoise`, they're marked restless. */
static s32 Lod_Compare(s32 markNoise) {
    u16* fb = sLodCur;
    s32 count = 0;
    s32 i;

    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        u16 a = fb[i];
        u16 b = sLodRef[i];
        s32 dr;
        s32 dg;
        s32 db;

        if (a == b) {
            continue;
        }
        dr = ABS((s32)((a >> 11) & 31) - (s32)((b >> 11) & 31));
        dg = ABS((s32)((a >> 6) & 31) - (s32)((b >> 6) & 31));
        db = ABS((s32)((a >> 1) & 31) - (s32)((b >> 1) & 31));
        if ((dr > LOD_THRESHOLD) || (dg > LOD_THRESHOLD) || (db > LOD_THRESHOLD)) {
            if (markNoise == 2) {
                if (!(sLodNoise[i >> 3] & (1 << (i & 7)))) {
                    fb[i] = 0xF801;
                }
            } else if (markNoise) {
                sLodNoise[i >> 3] |= 1 << (i & 7);
                count++;
            } else if (!(sLodNoise[i >> 3] & (1 << (i & 7)))) {
                count++;
            }
        }
    }
    return count;
}

static u16 sLodPending[FISHMOD_LOD_KINDS]; // the cut to test once its fresh reference is taken
static s32 sLodFresh;                      // a fresh reference is being drawn

/**
 * Draws everything for LOD_HOLD frames and takes that as the reference, then the cut in sLodPending for LOD_HOLD
 * frames: each test is compared with a frame from just before it, so nothing that drifts slowly (light settling,
 * the sky) is counted.
 */
static void Lod_TestPending(void) {
    bzero(gFishmodLodTest.dist, sizeof(gFishmodLodTest.dist));
    bzero(gFishmodLodTest.imp, sizeof(gFishmodLodTest.imp));
    bzero(gFishmodLodTest.half, sizeof(gFishmodLodTest.half));
    sLodFresh = true;
    sLodTimer = LOD_HOLD;
}

static void Lod_Test(s32 kind, s32 dist) {
    bzero(sLodPending, sizeof(sLodPending));
    if (kind < 0) {
        bzero(gFishmodLodTest.dist, sizeof(gFishmodLodTest.dist));
        bzero(gFishmodLodTest.imp, sizeof(gFishmodLodTest.imp));
        bzero(gFishmodLodTest.half, sizeof(gFishmodLodTest.half));
        sLodTimer = LOD_HOLD;
        return;
    }
    sLodPending[kind] = dist;
    Lod_TestPending();
}

static void Lod_Save(void) {
    sLod.magic = 0x464C4F44; // 'FLOD'
    sLod.kinds = gFishmodLodTest.kindCount;
    sLod.threshold = LOD_THRESHOLD;
    sLod.pixels = LOD_PIXELS;
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7A40, &sLod, sizeof(sLod), OS_WRITE);
}

#ifdef FISHMOD_LOD_SWEEP
/*
 * The LOD sweep: the calibration over many viewpoints (fishmod/tools/gen_lod_views.py: the shores, the surface, the
 * bed, each looked from several ways), for draw distances per zone. Group testing keeps it quick: each view is first
 * tested once with every kind cut to its zone's distances so far; only if that changes the frame are the kinds
 * split in halves (each half cut alone) down to the ones that show, and only those searched for the distance they
 * need here. Most views, once the tables have grown, pass on the first test. Results to SRAM 0x0000 (over the save
 * slots: this build never saves) for fishmod/tools/lod_calibrate.py --sweep.
 */
#define SWEEP_ZONES 16
#ifdef FISHMOD_LOD_TUNE
/*
 * The LOD tuning (lod_tune.inc.c): the sweep, but over every view at each of TUNE_TIMES times of day, and for three
 * settings per kind, each against the same full-detail reference: how far it's drawn (the cut), from how far its
 * single card is used, from how far its half-size texture is. Each the shortest distance that changes no view (as
 * the sweep counts it), per time of day and above or under water, in TUNE_STEP units. The views are shared out
 * between parts run side by side; the part comes from SRAM 0x7C00 ('FMPT', part, parts: fishmod/pc/run_jobs.py
 * writes it into each job's save), so one ROM serves every part.
 */
#define TUNE_TIMES 4
#define TUNE_KNOBS 3 // the cut, the single card, the half-size texture
#define SW_STEP 50
#define SW_STEPS 60 // out to 3000 (DIST_CALIBRATE)
static const u16 sTuneTimes[TUNE_TIMES] = { CLOCK_TIME(6, 30), CLOCK_TIME(12, 0), CLOCK_TIME(18, 30), CLOCK_TIME(0, 0) };
static s32 sTuneKnob;
#define SWEEP_SLOTS ((s32)ARRAY_COUNT(sLodViews) * TUNE_TIMES)
#define SWEEP_SLOT_VIEW(slot) ((slot) / TUNE_TIMES)
#define SW_KNOB_DONE SW_NEXT_KNOB
#else
#define SW_STEP LOD_STEP
#define SW_STEPS LOD_STEPS
#define SWEEP_SLOTS ((s32)ARRAY_COUNT(sLodViews))
#define SWEEP_SLOT_VIEW(slot) (slot)
#define SW_KNOB_DONE SW_NEXT_VIEW
#endif
static s32 sSweepPart = FISHMOD_LOD_PART;
static s32 sSweepParts = FISHMOD_LOD_PARTS;
#define SWEEP_SETTLE 45
#define SWEEP_HOLD 3       // frames a test is drawn before it's read
#define SWEEP_GRAB_TRIES 30 // a view whose frame can't be copied whole this many times is skipped
#define SWEEP_RETRIES 3

Vec3f gFishmodLodSweepEye;
Vec3f gFishmodLodSweepAt;

typedef struct SweepResults {
    /* 0x000 */ u32 magic; // 'FLSW'
    /* 0x004 */ u16 views;
    /* 0x006 */ u16 done;   // views measured so far (in this part)
    /* 0x008 */ u16 noisy;  // views skipped: the frame changed by itself
    /* 0x00A */ u16 empty;  // views with no props in sight
    /* 0x00C */ u32 tests;
    /* 0x010 */ u8 kinds;
    /* 0x011 */ u8 zones;
    /* 0x012 */ u8 part;
    /* 0x013 */ u8 parts;
#ifdef FISHMOD_LOD_TUNE
    /* 0x014 */ u8 need[TUNE_TIMES][2][TUNE_KNOBS][FISHMOD_LOD_KINDS]; // in SW_STEP units; 0: never in sight
#else
    /* 0x014 */ u8 need[SWEEP_ZONES][2][FISHMOD_LOD_KINDS]; // in LOD_STEP units; 0: never in sight
#endif
    u16 passFirst; // views that passed on their first test
    u16 splits;    // views that needed the kinds split
} SweepResults;

typedef enum SweepState {
    SW_ARRIVE,
    SW_REF,
    SW_NOISE,
    SW_ALL,
    SW_SPLIT_NEXT,
    SW_SPLIT_CHECK,
    SW_SEARCH,
    SW_NEXT_VIEW,
    SW_NEXT_KNOB,
    SW_DONE
} SweepState;

static SweepResults sSweep;
static s32 sSweepState = SW_ARRIVE;
static s32 sSweepView = FISHMOD_LOD_PART; // (the slot: the view, and with the tuning the time of day)
static s32 sSweepRetry;
static s32 sSweepFirstTest;
static u64 sSweepStack[64];
static s32 sSweepTop;
static u64 sSweepMask; // being tested in a split
static u64 sSweepCulprits;
static u64 sSweepWhole; // every kind cut, if that changed the frame
static s32 sSweepSettle;
static s32 sSweepGrabFails;

// each part takes its own run of the views (one stretch of the lake each, its times of day together)
static s32 Sweep_PartStart(void) {
    return (s32)((s64)SWEEP_SLOTS * sSweepPart / sSweepParts);
}

static s32 Sweep_PartEnd(void) {
    return (s32)((s64)SWEEP_SLOTS * (sSweepPart + 1) / sSweepParts);
}

static u8* Sweep_Need(s32 kind) {
#ifdef FISHMOD_LOD_TUNE
    return &sSweep.need[sSweepView % TUNE_TIMES][gFishmodLodTest.under][sTuneKnob][kind];
#else
    FishmodLodView* view = &sLodViews[sSweepView];

    return &sSweep.need[view->zone % SWEEP_ZONES][gFishmodLodTest.under][kind];
#endif
}

/** The test's distances for the setting being measured (the cut, or with the tuning one of its others). */
static s16* Sweep_Knob(void) {
    bzero(gFishmodLodTest.dist, sizeof(gFishmodLodTest.dist));
    bzero(gFishmodLodTest.imp, sizeof(gFishmodLodTest.imp));
    bzero(gFishmodLodTest.half, sizeof(gFishmodLodTest.half));
#ifdef FISHMOD_LOD_TUNE
    if (sTuneKnob == 1) {
        return gFishmodLodTest.imp;
    }
    if (sTuneKnob == 2) {
        return gFishmodLodTest.half;
    }
#endif
    return gFishmodLodTest.dist;
}

/** The cut to test: the kinds in `mask` at their zone's distances, the rest drawn in full. */
static void Sweep_Test(u64 mask) {
    s32 k;

    s16* knob = Sweep_Knob();

    for (k = 0; k < gFishmodLodTest.kindCount; k++) {
        if (mask & (1ULL << k)) {
            knob[k] = MAX(*Sweep_Need(k), 1) * SW_STEP;
        }
    }
    sSweep.tests++;
    // one reference per view (taken after it settles, checked for anything moving): no fresh one per test
    sLodTimer = SWEEP_HOLD;
}

/** One kind cut at `dist`, the rest drawn in full. */
static void Sweep_TestKind(s32 kind, s32 dist) {
    Sweep_Knob()[kind] = dist;
    sLodTimer = SWEEP_HOLD;
}

static void Sweep_Save(void) {
    sSweep.magic = 0x464C5357; // 'FLSW'
    sSweep.views = SWEEP_SLOTS;
    sSweep.kinds = gFishmodLodTest.kindCount;
    sSweep.zones = SWEEP_ZONES;
    sSweep.part = sSweepPart;
    sSweep.parts = sSweepParts;
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000), &sSweep, sizeof(sSweep), OS_WRITE);
}

static void Sweep_Place(void) {
    FishmodLodView* view = &sLodViews[SWEEP_SLOT_VIEW(sSweepView)];

    gFishmodLodSweepEye.x = view->eye.x;
    gFishmodLodSweepEye.y = view->eye.y;
    gFishmodLodSweepEye.z = view->eye.z;
    gFishmodLodSweepAt.x = view->at.x;
    gFishmodLodSweepAt.y = view->at.y;
    gFishmodLodSweepAt.z = view->at.z;
#ifdef FISHMOD_LOD_TUNE
    sLodTime = sTuneTimes[sSweepView % TUNE_TIMES];
#endif
}

static void Fishmod_LodUpdate(PlayState* play) {
    sLodGfxCtx = play->state.gfxCtx;
    if (sLodRef == NULL) {
        sLodRef = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        sLodNoise = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT / 8, "../fishmod_debug.c", __LINE__);
        sLodCur = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        sLodFrames = play->gameplayFrames;
        sLodTime = gSaveContext.save.dayTime;
        sLodSkyRot = play->skyboxCtx.rot.y;
        sLodStateFrames = play->state.frames;
        gFishmodLodTest.active = true;
        sSweepSettle = SWEEP_SETTLE + 200; // the first: the scene's lights are still coming up
        // which part this is, if the runner said: in the ROM's header (the name field, which nothing checks), else
        // in the save (fishmod/pc/run_jobs.py writes both)
        {
            static u32 pt[4] ALIGNED(16);

            DmaMgr_RequestSync(pt, 0x20, sizeof(pt));
            if (!((pt[0] == 0x464D5054) && (pt[2] > 0) && (pt[1] < pt[2]))) { // 'FMPT'
                SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7C00, pt, 12, OS_READ);
            }
            if ((pt[0] == 0x464D5054) && (pt[2] > 0) && (pt[1] < pt[2])) {
                sSweepPart = pt[1];
                sSweepParts = pt[2];
            }
        }
        // carry on from an earlier run (the save file kept what it measured)
        SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000), &sSweep, sizeof(sSweep), OS_READ);
        if ((sSweep.magic != 0x464C5357) || (sSweep.part != sSweepPart) || (sSweep.parts != sSweepParts)) {
            bzero(&sSweep, sizeof(sSweep));
        }
        sSweepView = Sweep_PartStart() + sSweep.done;
        if (sSweepView >= Sweep_PartEnd()) {
            sSweepState = SW_DONE;
            gFishmodLodTest.active = false;
            return;
        }
        Sweep_Place();
        if ((sLodRef == NULL) || (sLodNoise == NULL) || (sLodCur == NULL)) {
            sSweepState = SW_DONE;
        }
    }
    play->gameplayFrames = sLodFrames;
    gSaveContext.save.dayTime = sLodTime;
    play->skyboxCtx.rot.y = sLodSkyRot;
    play->state.frames = sLodStateFrames;
    play->haltAllActors = gFishmodLodTest.active;
    if (sSweepState == SW_DONE) {
        Fishmod_DebugPrintf(2, "LOD sweep done: %d views", sSweep.done);
        return;
    }
    Fishmod_DebugPrintf(2, "LOD part %d/%d: %d of %d", sSweepPart + 1, sSweepParts, sSweepView - Sweep_PartStart() + 1,
                        Sweep_PartEnd() - Sweep_PartStart());
    if (sSweepState == SW_ARRIVE) {
        if (--sSweepSettle > 0) {
            return;
        }
        Lod_Test(-1, 0);
        sSweepState = SW_REF;
        return;
    }
    if (--sLodTimer > 0) {
        return;
    }
    if (sLodFresh) {
        if (!Lod_Grab(sLodRef)) {
            sLodTimer = 1;
            return;
        }
        bcopy(sLodPending, gFishmodLodTest.dist, sizeof(sLodPending));
        sLodFresh = false;
        sLodTimer = LOD_HOLD;
        return;
    }
    if (!Lod_Grab((sSweepState == SW_REF) ? sLodRef : sLodCur)) {
        sLodTimer = 1; // the screen swapped while it was copied: again next frame
        if (++sSweepGrabFails < SWEEP_GRAB_TRIES) {
            return;
        }
        sSweep.noisy++;
        sSweepState = SW_NEXT_VIEW;
    }
    sSweepGrabFails = 0;

    switch (sSweepState) {
        case SW_REF: {
            s32 k;
            s32 any = false;

            bcopy(gFishmodLodTest.drawn, sLodDrawn, sizeof(sLodDrawn));
            for (k = 0; k < gFishmodLodTest.kindCount; k++) {
                any |= (sLodDrawn[k] != 0);
            }
            if (!any) {
                sSweep.empty++;
                sSweepState = SW_NEXT_VIEW;
                sLodTimer = 1;
                break;
            }
            bzero(sLodNoise, SCREEN_WIDTH * SCREEN_HEIGHT / 8);
            Lod_Test(-1, 0);
            sSweepState = SW_NOISE;
            break;
        }

        case SW_NOISE:
            if (Lod_Compare(true) > 0) {
#ifdef FISHMOD_LOD_SHOW
                // diagnosis: the first noisy view's moving pixels (and its number) to SRAM 0x1000
                static s32 sShown;

                if (!sShown) {
                    u32 v = sSweepView;

                    sShown = true;
                    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x1000, &v, 4, OS_WRITE);
                    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x1010, sLodNoise,
                                     SCREEN_WIDTH * SCREEN_HEIGHT / 8, OS_WRITE);
                    // and row 200 of the reference and of this frame
                    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x3800, &sLodRef[200 * SCREEN_WIDTH],
                                     SCREEN_WIDTH * 2, OS_WRITE);
                    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x3800 + SCREEN_WIDTH * 2,
                                     &sLodCur[200 * SCREEN_WIDTH], SCREEN_WIDTH * 2, OS_WRITE);
                }
#endif
                sSweep.noisy++;
                sSweepState = SW_NEXT_VIEW;
                sLodTimer = 1;
                break;
            }
            sSweepRetry = 0;
            sSweepFirstTest = true;
            // fall through
        case SW_ALL: {
            u64 mask = 0;
            s32 k;

            for (k = 0; k < gFishmodLodTest.kindCount; k++) {
#ifdef FISHMOD_LOD_TUNE
                // (only the kinds that have the setting being measured)
                if (((sTuneKnob == 1) && !(gFishmodLodTest.hasImp & (1ULL << k))) ||
                    ((sTuneKnob == 2) && !(gFishmodLodTest.hasHalf & (1ULL << k)))) {
                    continue;
                }
#endif
                if (sLodDrawn[k] != 0) {
                    *Sweep_Need(k) = MAX(*Sweep_Need(k), 1); // seen in this zone (0: never seen, no measure)
                    if (*Sweep_Need(k) < SW_STEPS) {
                        mask |= 1ULL << k;
                    }
                }
            }
            if (mask == 0) {
                sSweepState = SW_KNOB_DONE;
                sLodTimer = 1;
                break;
            }
            sSweepTop = 0;
            sSweepStack[sSweepTop++] = mask;
            sSweepCulprits = 0;
            sSweepMask = mask;
            Sweep_Test(mask);
            sSweepState = SW_SPLIT_CHECK;
            break;
        }

        case SW_SPLIT_CHECK: {
            s32 changed = Lod_Compare(false);

            sSweepTop--; // the mask just tested was on top
            if (sSweepTop == 0) {
                sSweepWhole = (changed > LOD_PIXELS) ? sSweepMask : 0;
            }
            if (changed > LOD_PIXELS) {
                if (sSweepFirstTest) {
                    sSweep.splits++;
                }
                if ((sSweepMask & (sSweepMask - 1)) == 0) {
                    sSweepCulprits |= sSweepMask;
                } else {
                    // halves: alternate bits
                    u64 a = 0;
                    u64 b = 0;
                    u64 m = sSweepMask;
                    s32 odd = 0;

                    while (m != 0) {
                        u64 bit = m & (~m + 1);

                        if (odd) {
                            a |= bit;
                        } else {
                            b |= bit;
                        }
                        odd ^= 1;
                        m &= m - 1;
                    }
                    sSweepStack[sSweepTop++] = a;
                    sSweepStack[sSweepTop++] = b;
                }
            } else if (sSweepFirstTest) {
                sSweep.passFirst++;
            }
            sSweepFirstTest = false;
        }
            // fall through
        case SW_SPLIT_NEXT:
            if (sSweepTop > 0) {
                sSweepMask = sSweepStack[sSweepTop - 1];
                Sweep_Test(sSweepMask);
                sSweepState = SW_SPLIT_CHECK;
                break;
            }
            if (sSweepCulprits == 0) {
                if (sSweepWhole != 0) {
                    // the kinds together change the frame, but none alone: each a step further, and again
                    s32 k;

                    for (k = 0; k < gFishmodLodTest.kindCount; k++) {
                        if (sSweepWhole & (1ULL << k)) {
                            *Sweep_Need(k) = MIN(MAX(*Sweep_Need(k), 1) + 1, SW_STEPS);
                        }
                    }
                    if (++sSweepRetry < SWEEP_RETRIES) {
                        sSweepState = SW_ALL;
                        sLodTimer = 1;
                        break;
                    }
                }
                sSweepState = SW_KNOB_DONE;
                sLodTimer = 1;
                break;
            }
            sLodKind = -1;
            // fall through
        case SW_SEARCH:
            if (sLodKind >= 0) {
                s32 mid = (sLodLo + sLodHi) / 2;

                if (Lod_Compare(false) <= LOD_PIXELS) {
                    sLodHi = mid;
                } else {
                    sLodLo = mid + 1;
                }
                if (sLodLo < sLodHi) {
                    Sweep_TestKind(sLodKind, ((sLodLo + sLodHi) / 2) * SW_STEP);
                    sSweep.tests++;
                    sSweepState = SW_SEARCH;
                    break;
                }
                *Sweep_Need(sLodKind) = MAX(*Sweep_Need(sLodKind), sLodHi);
                sSweepCulprits &= ~(1ULL << sLodKind);
            }
            if (sSweepCulprits != 0) {
                s32 k = 0;

                while (!(sSweepCulprits & (1ULL << k))) {
                    k++;
                }
                sLodKind = k;
                sLodLo = MAX(*Sweep_Need(k), 1) + 1;
                sLodHi = SW_STEPS;
                if (sLodLo >= sLodHi) {
                    *Sweep_Need(k) = SW_STEPS;
                    sSweepCulprits &= ~(1ULL << k);
                    sLodKind = -1;
                    sLodTimer = 1;
                    sSweepState = SW_SEARCH;
                    break;
                }
                Sweep_TestKind(k, ((sLodLo + sLodHi) / 2) * SW_STEP);
                sSweep.tests++;
                sSweepState = SW_SEARCH;
                break;
            }
            // the culprits raised: the whole view again
            if (++sSweepRetry < SWEEP_RETRIES) {
                sSweepState = SW_ALL;
                sLodTimer = 1;
                break;
            }
            sSweepState = SW_KNOB_DONE;
            sLodTimer = 1;
            break;

        case SW_NEXT_KNOB:
#ifdef FISHMOD_LOD_TUNE
            // the next setting, against the same reference
            if (++sTuneKnob < TUNE_KNOBS) {
                sSweepRetry = 0;
                sSweepFirstTest = true;
                sSweepState = SW_ALL;
                sLodTimer = 1;
                break;
            }
#endif
            // fall through
        case SW_NEXT_VIEW:
#ifdef FISHMOD_LOD_TUNE
            sTuneKnob = 0;
#endif
            sSweep.done++;
            Sweep_Save();
            sSweepView++;
            if (sSweepView >= Sweep_PartEnd()) {
                Sweep_Save();
                sSweepState = SW_DONE;
                gFishmodLodTest.active = false;
                break;
            }
            Sweep_Place();
            Lod_Test(-1, 0);
            sSweepSettle = SWEEP_SETTLE;
            sSweepState = SW_ARRIVE;
            break;
    }
}
#elif defined(FISHMOD_LOD_ROTATE)
/*
 * The rotation check: at each tour stop the camera turns all the way round (16 directions, about the point it looks
 * at, as the stick turns it), and at each the game's frame is compared with two references: the same props with a
 * much wider view cone (what the cone wrongly leaves out), and everything out to DIST_CALIBRATE with that cone (what
 * any cut leaves out: the cone and the distances). Pixels differing, per stop and direction, to SRAM 0x7A40 for
 * fishmod/tools/lod_calibrate.py --rotate.
 */
#define ROT_YAWS 16
#define ROT_STOPS 22

typedef struct RotResults {
    u32 magic; // 'FROT'
    u16 stops;
    u16 yaws;
    u16 cone[ROT_STOPS][ROT_YAWS];
    u16 all[ROT_STOPS][ROT_YAWS];
} RotResults;

static RotResults sRot;
static s32 sRotState; // 0 arriving; then per direction: 1 the game's frame, 2 the wide cone, 3 everything
static s32 sRotYaw;

static void Rot_Config(s32 state) {
    gFishmodLodTest.asGame = (state != 3);
    gFishmodLodTest.wide = (state >= 2);
    bzero(gFishmodLodTest.dist, sizeof(gFishmodLodTest.dist));
    sLodTimer = LOD_HOLD;
}

static void Fishmod_LodUpdate(PlayState* play) {
    s32 stop = sTourStop;

    sLodGfxCtx = play->state.gfxCtx;
    if (sLodRef == NULL) {
        sLodRef = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        sLodNoise = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT / 8, "../fishmod_debug.c", __LINE__);
        sLodCur = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        if ((sLodRef == NULL) || (sLodNoise == NULL) || (sLodCur == NULL)) {
            return;
        }
        bzero(sLodNoise, SCREEN_WIDTH * SCREEN_HEIGHT / 8);
        sLodFrames = play->gameplayFrames;
        sLodTime = gSaveContext.save.dayTime;
        sLodSkyRot = play->skyboxCtx.rot.y;
        sLodStateFrames = play->state.frames;
        gFishmodLodTest.active = true;
        Rot_Config(1);
    }
    play->gameplayFrames = sLodFrames;
    gSaveContext.save.dayTime = sLodTime;
    play->skyboxCtx.rot.y = sLodSkyRot;
    play->state.frames = sLodStateFrames;
    play->haltAllActors = gFishmodLodTest.active;
    if (!gFishmodLodTest.active) {
        Fishmod_DebugPrintf(2, "rotation check done");
        return;
    }
    Fishmod_DebugPrintf(2, "rotation stop %d dir %d", stop + 1, sRotYaw);
    sTourYaw = sRotYaw * (2.0f * M_PI / ROT_YAWS);
    if (sRotState == 0) {
        if (sTourFrame >= TOUR_GLIDE + LOD_SETTLE + ((stop == 0) ? 200 : 0)) {
            sRotState = 1;
            sRotYaw = 0;
            Rot_Config(1);
        }
        return;
    }
    if (--sLodTimer > 0) {
        return;
    }
    {
        // a direction that takes the camera out of the water or into it: wait for the under-water look to finish
        // changing (it eases in and out), or the frames compared would differ by it
        static s32 sHazeWait;

        if ((gFishmodHaze > 0.001f) && (gFishmodHaze < 0.999f) && (sHazeWait < 600)) {
            sHazeWait++;
            return;
        }
        sHazeWait = 0;
    }
    switch (sRotState) {
        case 1:
            Lod_Grab(sLodRef); // the game's frame
            Rot_Config(2);
            sRotState = 2;
            break;
        case 2:
            Lod_Grab(sLodCur);
            sRot.cone[stop][sRotYaw] = MIN(Lod_Compare(false), 0xFFFF);
            Rot_Config(3);
            sRotState = 3;
            break;
        default:
            Lod_Grab(sLodCur);
            sRot.all[stop][sRotYaw] = MIN(Lod_Compare(false), 0xFFFF);
            if (++sRotYaw >= ROT_YAWS) {
                sRot.magic = 0x46524F54; // 'FROT'
                sRot.stops = stop + 1;
                sRot.yaws = ROT_YAWS;
                SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7A40 - 0x400, &sRot, sizeof(sRot), OS_WRITE);
                sRotYaw = 0;
                if (stop + 1 >= ROT_STOPS) {
                    gFishmodLodTest.active = false;
                    gFishmodLodTest.asGame = gFishmodLodTest.wide = false;
                    break;
                }
                Fishmod_TourGo(stop + 1);
                sRotState = 0;
                break;
            }
            Rot_Config(1);
            sRotState = 1;
            break;
    }
}
#else
static void Fishmod_LodUpdate(PlayState* play) {
    s32 under = gFishmodLodTest.under;
    s32 stop = sTourStop;

    sLodGfxCtx = play->state.gfxCtx;

    if (sLodRef == NULL) {
        sLodRef = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        sLodNoise = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT / 8, "../fishmod_debug.c", __LINE__);
        sLodCur = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        sLodSkyRot = play->skyboxCtx.rot.y;
        sLodStateFrames = play->state.frames;
        sLodFrames = play->gameplayFrames;
        sLodTime = gSaveContext.save.dayTime;
        gFishmodLodTest.active = true;
        if ((sLodRef == NULL) || (sLodNoise == NULL) || (sLodCur == NULL)) {
            sLodState = LOD_DONE;
        }
    }
    // everything that moves, held still
    play->gameplayFrames = sLodFrames;
    gSaveContext.save.dayTime = sLodTime;
    play->skyboxCtx.rot.y = sLodSkyRot; // the sky turns slowly (and shows through the water from below)
    play->state.frames = sLodStateFrames; // the lake's water scrolls by this count
    play->haltAllActors = gFishmodLodTest.active; // Link, the NPCs, birds: still (drawn, not updated)
    if (sLodState == LOD_DONE) {
        Fishmod_DebugPrintf(2, "LOD calibration done: %d stops", sLod.stops);
        return;
    }
    Fishmod_DebugPrintf(2, "LOD stop %d kind %d [%d-%d] %s", stop + 1, sLodKind, sLodLo * LOD_STEP, sLodHi * LOD_STEP,
                        under ? "under" : "above");
    if ((sLodState != LOD_ARRIVE) && (--sLodTimer > 0)) {
        return;
    }
    if (sLodFresh) {
        if (!Lod_Grab(sLodRef)) {
            sLodTimer = 1; // torn: again next frame
            return;
        }
        bcopy(sLodPending, gFishmodLodTest.dist, sizeof(sLodPending));
        sLodFresh = false;
        sLodTimer = LOD_HOLD;
        return;
    }

    // every state past this reads the frame (the noise passes, the tests, the check)
    if ((sLodState == LOD_NOISE) || (sLodState == LOD_SEARCH) || (sLodState == LOD_VERIFY)) {
        if (!Lod_Grab(sLodCur)) {
            sLodTimer = 1;
            return;
        }
    }

    switch (sLodState) {
        case LOD_ARRIVE:
            // the first stop waits longer: the scene's lights are still coming up after it loads
            if (sTourFrame >= TOUR_GLIDE + LOD_SETTLE + ((stop == 0) ? 200 : 0)) {
                Lod_Test(-1, 0);
                sLodState = LOD_REF;
            }
            break;

        case LOD_REF:
            if (!Lod_Grab(sLodRef)) {
                sLodTimer = 1;
                return;
            }
            bcopy(gFishmodLodTest.drawn, sLodDrawn, sizeof(sLodDrawn));
            bzero(sLodNoise, SCREEN_WIDTH * SCREEN_HEIGHT / 8);
            sLodPass = 0;
            sLod.noise[stop] = 0;
            Lod_Test(-1, 0);
            sLodState = LOD_NOISE;
            break;

        case LOD_NOISE:
            sLod.noise[stop] += Lod_Compare(true);
            if (++sLodPass < LOD_NOISE_REFS) {
                Lod_Test(-1, 0);
                break;
            }
            sLodKind = -1;
            sLodState = LOD_NEXT_KIND;
            // fall through
        case LOD_NEXT_KIND:
            do {
                sLodKind++;
            } while ((sLodKind < gFishmodLodTest.kindCount) && (sLodDrawn[sLodKind] == 0));
            if (sLodKind >= gFishmodLodTest.kindCount) {
                s32 k;

                // every kind at the distances found so far, together
                for (k = 0; k < gFishmodLodTest.kindCount; k++) {
                    sLodPending[k] = MAX(sLod.need[under][k], LOD_STEP);
                }
                Lod_TestPending();
                sLodState = LOD_VERIFY;
                break;
            }
            sLodLo = 1;
            sLodHi = LOD_STEPS;
            Lod_Test(sLodKind, ((sLodLo + sLodHi) / 2) * LOD_STEP);
            sLodState = LOD_SEARCH;
            break;

        case LOD_SEARCH: {
            s32 mid = (sLodLo + sLodHi) / 2;

            s32 changed = Lod_Compare(false);

#ifdef FISHMOD_LOD_SHOW
            // diagnosis: masks of what differs, to SRAM (over the save slots): the first small failure, the noise
            // mask, the first large failure
            {
                static s32 sDumped;
                s32 slot = (changed > 3000) ? 2 : (changed > LOD_PIXELS) ? 0 : -1;

                if ((slot >= 0) && !(sDumped & (1 << slot))) {
                    static u8 sMask[SCREEN_WIDTH * SCREEN_HEIGHT / 8];
                    u16* fb = sLodCur;
                    s32 i;

                    sDumped |= 1 << slot;
                    bzero(sMask, sizeof(sMask));
                    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
                        u16 x = fb[i];
                        u16 y = sLodRef[i];

                        if ((ABS((s32)((x >> 11) & 31) - (s32)((y >> 11) & 31)) > LOD_THRESHOLD) ||
                            (ABS((s32)((x >> 6) & 31) - (s32)((y >> 6) & 31)) > LOD_THRESHOLD) ||
                            (ABS((s32)((x >> 1) & 31) - (s32)((y >> 1) & 31)) > LOD_THRESHOLD)) {
                            sMask[i >> 3] |= 1 << (i & 7);
                        }
                    }
                    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + slot * sizeof(sMask), sMask, sizeof(sMask),
                                     OS_WRITE);
                    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + sizeof(sMask), sLodNoise, sizeof(sMask),
                                     OS_WRITE);
                }
            }
#endif

            if (sLod.logCount < ARRAY_COUNT(sLod.log)) {
                sLod.log[sLod.logCount][0] = sLodKind;
                sLod.log[sLod.logCount][1] = mid * LOD_STEP;
                sLod.log[sLod.logCount][2] = changed;
                sLod.logCount++;
            }
            if (changed <= LOD_PIXELS) {
                sLodHi = mid; // stopping there changes nothing that shows
            } else {
                sLodLo = mid + 1;
            }
            if (sLodLo < sLodHi) {
                Lod_Test(sLodKind, ((sLodLo + sLodHi) / 2) * LOD_STEP);
            } else {
                sLod.need[under][sLodKind] = MAX(sLod.need[under][sLodKind], sLodHi * LOD_STEP);
                sLodState = LOD_NEXT_KIND;
                sLodTimer = 1;
            }
            break;
        }

        case LOD_VERIFY:
            sLod.verify[stop] = Lod_Compare(false);
            sLod.under[stop] = under;
            sLod.stops = stop + 1;
            Lod_Save();
            if (stop + 1 >= (s32)ARRAY_COUNT(sTourStops)) {
                sLodState = LOD_DONE;
                gFishmodLodTest.active = false;
            } else {
                Fishmod_TourGo(stop + 1);
                Lod_Test(-1, 0);
                sLodState = LOD_ARRIVE;
            }
            break;
    }
}
#endif // FISHMOD_LOD_SWEEP
#endif


#ifdef FISHMOD_HWBENCH
static u32 sLastCpu[FISHMOD_CPU_MAX];
#endif
#ifdef FISHMOD_HWBENCH
/*
 * The hardware benches' shared core (the play bench, FISHMOD_BENCH_PLAY; the area bench has its own records): blocks
 * of frames in each mode, timed, with the RDP's own counters (only the console has them: how much of its time goes on
 * fetching commands and waiting on memory, drawing, loading textures), and a few memory tests, with the RDP busy and
 * idle. The sums go to SRAM as they're made (the SummerCart writes them to the .sav): hwbench_report.py reads them.
 */
#define BENCH_STOPS 22  // the tour's stops: the legs between them
#define BENCH_SKIP 10   // frames after changing mode, not counted (the frame being timed runs behind)
#if defined(FISHMOD_BENCH_FRAMES)
#define BENCH_FRAMES FISHMOD_BENCH_FRAMES // (the short test)
#else
#define BENCH_FRAMES 240 // frames counted per block: Link on his way along the leg
#endif
#define BENCH_ROUNDS 1
#define BENCH_MICRO_FRAMES 30
#define BENCH_VERSION 8
#define BENCH_CONDS 8 // the times of day (4) x clear or rain
#ifndef FISHMOD_BENCH_LAPS
#define FISHMOD_BENCH_LAPS 15 // then it stops and says so
#endif

// the play bench's modes (Bench_SetMode has what each switches off)
#define BENCH_BASE 0
#define BENCH_MODES 8
#define BENCH_MICRO BENCH_MODES

typedef enum BenchMetric {
    BM_PERIOD,     // frame period, us (the time between graph updates)
    BM_CPU_UPDATE, // Play_Update, us (less the cache filling in the cold mode)
    BM_CPU_DRAW,   // Play_Draw (display lists), us
    BM_RSP_GFX,    // RSP graphics task, us
    BM_RDP,        // RDP (scheduler: from its task's start to its end), us
    BM_RSP_AUDIO,  // RSP audio, us
    BM_DP_CLOCK,   // RDP counters, in 64-cycle units: all cycles
    BM_DP_BUF,     //   command buffer busy (fetching, or held up)
    BM_DP_PIPE,    //   pipeline busy (drawing)
    BM_DP_TMEM,    //   loading TMEM
    BM_SCATTER,    // the props' display lists (CPU), us
    BM_FISH_UPD,   // the fishes' updates (CPU), us
    BM_FISH_DRAW,  // the fishes' draws (CPU), us
    BM_PROPS,      // props drawn
    BENCH_METRICS
} BenchMetric;

typedef enum BenchTest {
    BT_CHASE,         // dependent loads, each to a line not in the cache: latency (per load)
    BT_CHASE_UNCACHED,// the same through KSEG1 (no cache at all)
    BT_READ,          // reading 64 KB through the cache, from RDRAM (per KB)
    BT_READ_UNCACHED, // reading 16 KB uncached (per KB)
    BT_WRITE,         // writing 64 KB through the cache and writing it back (per KB)
    BT_COPY,          // bcopy 64 KB, from RDRAM to RDRAM through the cache (per KB)
    BENCH_TESTS
} BenchTest;

typedef struct BenchCell {
    u32 frames;
    u32 sum[BENCH_METRICS];
    u32 max[BENCH_METRICS];
} BenchCell;

typedef struct BenchMicro {
    u32 count;
    u32 sum; // CPU count cycles (46.875 MHz)
    u32 min;
} BenchMicro;

typedef struct BenchResults {
    u32 magic; // 'FBEN'
    u8 version;
    u8 stops;
    u8 modes;
    u8 metrics;
    u8 tests;
    u8 hw;    // the original-hardware build
    u8 pad[2];
    u32 laps; // tours completed
    u32 visits;
    BenchCell cell[BENCH_STOPS][BENCH_MODES];
    BenchMicro micro[BENCH_TESTS][2]; // [test][RDP idle, busy]
#ifdef BENCH_CONDS
    BenchCell cond[BENCH_CONDS][BENCH_MODES]; // all the legs, per time of day and weather
#endif
} BenchResults;

static BenchResults sBench;
#ifdef BENCH_CONDS
static s32 sPbCond; // this lap's time of day and weather (the play bench)
#endif
static s32 sBenchState; // 0: arriving, 1: measuring, 2: micro
static s32 sBenchFrame;
static s32 sBenchBlock; // block in this visit
static u32* sBenchBuf;  // 128 KB for the memory tests
static u32 sBenchRand = 0x1234567;

#define BENCH_BUF_SIZE 0x20000

static u32 Bench_Rand(void) {
    sBenchRand ^= sBenchRand << 13;
    sBenchRand ^= sBenchRand >> 17;
    sBenchRand ^= sBenchRand << 5;
    return sBenchRand;
}

static s32 Bench_RdpBusy(void) {
    return (IO_READ(DPC_STATUS_REG) & (DPC_STATUS_PIPE_BUSY | DPC_STATUS_CMD_BUSY)) != 0;
}

static void Bench_Micro(void) {
    s32 test = (s32)(Bench_Rand() % BENCH_TESTS);
    u32* buf = sBenchBuf;
    s32 busy0 = Bench_RdpBusy();
    u32 t0;
    u32 t1;
    u32 per = 1;
    s32 i;
    u32 sink = 0;

    osWritebackDCacheAll();
    osInvalDCache(buf, BENCH_BUF_SIZE);
    switch (test) {
        case BT_CHASE:
        case BT_CHASE_UNCACHED: {
            // a random cycle through 2048 lines of the first 64 KB (one word each, 32 bytes apart): each load needs
            // the last
            u32* base = (test == BT_CHASE) ? buf : (u32*)OS_PHYSICAL_TO_K1(OS_K0_TO_PHYSICAL(buf));
            u32 idx = 0;

            t0 = osGetCount();
            for (i = 0; i < 2048; i++) {
                idx = base[idx];
            }
            t1 = osGetCount();
            sink = idx;
            per = 2048;
            break;
        }
        case BT_READ:
            t0 = osGetCount();
            for (i = 0; i < 0x10000 / 4; i += 4) {
                sink += buf[i] + buf[i + 1] + buf[i + 2] + buf[i + 3];
            }
            t1 = osGetCount();
            per = 64;
            break;
        case BT_READ_UNCACHED: {
            u32* k1 = (u32*)OS_PHYSICAL_TO_K1(OS_K0_TO_PHYSICAL(buf));

            t0 = osGetCount();
            for (i = 0; i < 0x4000 / 4; i += 4) {
                sink += k1[i] + k1[i + 1] + k1[i + 2] + k1[i + 3];
            }
            t1 = osGetCount();
            per = 16;
            break;
        }
        case BT_WRITE:
            t0 = osGetCount();
            for (i = 0x10000 / 4; i < 0x20000 / 4; i += 4) {
                buf[i] = i;
                buf[i + 1] = i;
                buf[i + 2] = i;
                buf[i + 3] = i;
            }
            osWritebackDCache(&buf[0x10000 / 4], 0x10000);
            t1 = osGetCount();
            per = 64;
            break;
        default: // BT_COPY
            t0 = osGetCount();
            bcopy(buf, &buf[0x10000 / 4], 0x10000);
            osWritebackDCache(&buf[0x10000 / 4], 0x10000);
            t1 = osGetCount();
            per = 64;
            break;
    }
    // (the writes and the copy only go to the second 64 KB: the chase's cycle in the first is left as it was)
    if (sink == 0xDEADBEEF) {
        sBench.pad[0]++; // (keeps the reads from being optimised away)
    }
    if (busy0 == Bench_RdpBusy()) {
        BenchMicro* m = &sBench.micro[test][busy0];
        u32 c = (t1 - t0) / per;

        m->count++;
        m->sum += c;
        if ((m->min == 0) || (c < m->min)) {
            m->min = c;
        }
    }
}

static void Bench_Record(s32 stop, s32 mode) {
    BenchCell* cell = &sBench.cell[stop][mode];
    u32 v[BENCH_METRICS];
    s32 i;

    v[BM_PERIOD] = (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    v[BM_CPU_UPDATE] = sLastCpu[FISHMOD_CPU_UPDATE];
    v[BM_CPU_DRAW] = sLastCpu[FISHMOD_CPU_DRAW];
    v[BM_RSP_GFX] = (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
    v[BM_RDP] = (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
    v[BM_RSP_AUDIO] = (u32)OS_CYCLES_TO_USEC(gRSPAudioTimeTotal);
    v[BM_DP_CLOCK] = gFishmodRdp.clock >> 6;
    v[BM_DP_BUF] = gFishmodRdp.buf >> 6;
    v[BM_DP_PIPE] = gFishmodRdp.pipe >> 6;
    v[BM_DP_TMEM] = gFishmodRdp.tmem >> 6;
    v[BM_SCATTER] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_SCATTER]);
    v[BM_FISH_UPD] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_U_ENGINE]);
    v[BM_FISH_DRAW] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_ENGINE]);
    v[BM_PROPS] = gFishmodScatterDrawn;
    cell->frames++;
    for (i = 0; i < BENCH_METRICS; i++) {
        cell->sum[i] += v[i];
        cell->max[i] = MAX(cell->max[i], v[i]);
    }
#ifdef BENCH_CONDS
    cell = &sBench.cond[sPbCond][mode];
    cell->frames++;
    for (i = 0; i < BENCH_METRICS; i++) {
        cell->sum[i] += v[i];
        cell->max[i] = MAX(cell->max[i], v[i]);
    }
#endif
}

/** Where the bench is, written to SRAM 0x7000 as each mode starts (0x7E00 is the profile): if the console crashes, the save says where. */
static void Bench_Breadcrumb(s32 stop, s32 mode, s32 block) {
    static struct {
        u32 magic; // 'FBCR'
        u32 laps;
        u8 stop;
        u8 mode;
        u8 block;
        u8 pad;
        u32 count; // breadcrumbs written so far
    } sCrumb;

    sCrumb.magic = 0x46424352;
    sCrumb.laps = sBench.laps;
    sCrumb.stop = stop;
    sCrumb.mode = mode;
    sCrumb.block = block;
    sCrumb.count++;
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7000, &sCrumb, sizeof(sCrumb), OS_WRITE);
}

static void Bench_SetMode(s32 mode) {
    static const s32 sFlags[BENCH_MODES] = {
        0, // the first pass along the leg (fish waking as Link arrives)
        0,
        FISHMOD_BENCH_NOFISHAI,
        FISHMOD_BENCH_NOFISH,
        FISHMOD_BENCH_NOFISH | FISHMOD_BENCH_NOFISHAI,
        FISHMOD_BENCH_NOPROPS,
        FISHMOD_BENCH_NOROOMXLU,
        FISHMOD_BENCH_NOPROPS | FISHMOD_BENCH_NOFISH | FISHMOD_BENCH_NOFISHAI | FISHMOD_BENCH_NOROOMXLU |
            FISHMOD_BENCH_NOSKY,
    };
    gFishmodBench = sFlags[mode];
}

static void Bench_Save(void) {
    sBench.magic = 0x4642454E; // 'FBEN'
    sBench.version = BENCH_VERSION;
    sBench.stops = BENCH_STOPS;
    sBench.modes = BENCH_MODES;
    sBench.metrics = BENCH_METRICS;
    sBench.tests = BENCH_TESTS;
    sBench.hw = FISHMOD_HW;
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000), &sBench, sizeof(sBench), OS_WRITE);
}

/** The memory tests' buffer, and the sums so far from the save. False if there's no memory for it. */
static s32 Bench_Init(void) {
    if (sBenchBuf == NULL) {
        s32 i;

        sBenchBuf = SYSTEM_ARENA_MALLOC(BENCH_BUF_SIZE, "../fishmod_debug.c", __LINE__);
        if (sBenchBuf == NULL) {
            return false;
        }
        // the chase's cycle: each of the first 2048 lines (of 16 bytes) points to the next in a shuffled order
        {
            static u16 sOrder[2048];

            for (i = 0; i < 2048; i++) {
                sOrder[i] = i;
            }
            for (i = 2047; i > 0; i--) {
                s32 j = (s32)(Bench_Rand() % (u32)(i + 1));
                u16 t = sOrder[i];

                sOrder[i] = sOrder[j];
                sOrder[j] = t;
            }
            for (i = 0; i < 2048; i++) {
                // spread over the first 64 KB: line k at 32 * k bytes (8 words apart; the cache's lines are 16)
                sBenchBuf[sOrder[i] * 8] = sOrder[(i + 1) % 2048] * 8;
            }
        }
        // carry on from an earlier night (the save file keeps the sums)
        SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000), &sBench, sizeof(sBench), OS_READ);
        if ((sBench.magic != 0x4642454E) || (sBench.version != BENCH_VERSION) || (sBench.hw != FISHMOD_HW)) {
            bzero(&sBench, sizeof(sBench));
        }
    }
    return true;
}

#if defined(FISHMOD_BENCH_PLAY) || defined(FISHMOD_BENCH_AREA)
// (Link's placing and steering, for the play bench and the area bench)
static s32 sPbStuckFrame;
static Vec3f sPbStuckPos;
static s8 sPbStickX;
static s8 sPbStickY;

/** Puts Link at (x, z) on the ground or the water there, facing yaw. */
static void PlayBench_Place(PlayState* play, Player* player, f32 x, f32 probeY, f32 z, s16 yaw) {
    CollisionPoly* poly;
    WaterBox* box;
    Vec3f probe;
    f32 floor;
    f32 surface;

    probe.x = x;
    probe.y = probeY + 200.0f;
    probe.z = z;
    floor = BgCheck_EntityRaycastDown1(&play->colCtx, &poly, &probe);
    if (floor < -30000.0f) {
        floor = player->actor.world.pos.y;
    }
    if (BgCheck_GetWaterSurface(play, &play->colCtx, x, z, &surface, &box) && (surface > floor)) {
        floor = surface;
    }
    player->actor.world.pos.x = player->actor.prevPos.x = x;
    player->actor.world.pos.y = player->actor.prevPos.y = floor + 2.0f;
    player->actor.world.pos.z = player->actor.prevPos.z = z;
    player->actor.shape.rot.y = player->actor.world.rot.y = player->yaw = yaw;
    player->actor.home.pos = player->actor.world.pos;
    player->actor.speed = 0.0f;
    player->actor.velocity.y = 0.0f;
    player->actor.floorBgId = BGCHECK_SCENE; // (not carried back by whatever he stood on)
    sPbStuckFrame = 0;
    sPbStuckPos = player->actor.world.pos;
}

/** The stick for heading toward a world yaw (the stick is read relative to the camera). */
static void PlayBench_Steer(PlayState* play, s16 yaw, s32 mag) {
    s16 rel = yaw - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));

    sPbStickX = (s8)(-Math_SinS(rel) * mag);
    sPbStickY = (s8)(Math_CosS(rel) * mag);
}
#endif

#ifdef FISHMOD_BENCH_PLAY
/*
 * The play bench (fishmod/scripts/hwbench_play.inc.c): Link plays the lake instead of a free camera flying it. He
 * runs, swims and dives from each tour stop to the next with the game's own camera behind him, and every third leg he
 * fishes from where he stands if he's on land (casting, waiting on the float, striking and reeling in what bites).
 * Each leg is timed once per mode, Link put back at its start each time: the first pass (the fish ahead waking as he
 * comes) and then the repeats. Each lap is at one time of day and weather, the clock held there: dawn, midday, dusk
 * and night, clear and in rain, in turn.
 */
#define PB_SKIP 30   // frames after putting Link back, not counted (the camera catching up)
#define PB_CLEANUP 400 // at most, after a fishing block: reeling in and through any catch (not counted)
#define PB_ARRIVED 90.0f

static s32 sPbState; // 0: new lap (reload the lake), 1: loading, 2: a block, 3: after a block, 4: memory tests
static s32 sPbLeg;
static s32 sPbBlock;
static s32 sPbFrame;
static s32 sPbFishing;
static u16 sPbButton;
static s32 sPbActive;
static s32 sPbTimer;

static const u16 sPbTimes[4] = { CLOCK_TIME(6, 30), CLOCK_TIME(12, 0), CLOCK_TIME(17, 30), CLOCK_TIME(23, 0) };
// the laps' conditions in turn (time + 4 * rain): clear dawn, rain midday, clear dusk, rain night, rain dawn,
// clear midday, rain dusk, clear night
static const u8 sPbOrder[BENCH_CONDS] = { 0, 5, 2, 7, 4, 1, 6, 3 };


/** Rain on or off without loading the scene again (the day's own weather is clear: FISHMOD_SCRIPT_DAY). */
static void PlayBench_SetRain(PlayState* play, s32 rain) {
    EnvironmentContext* envCtx = &play->envCtx;

    envCtx->precipitation[PRECIP_RAIN_MAX] = rain ? 20 : 0;
    envCtx->precipitation[PRECIP_RAIN_CUR] = rain ? 20 : 0;
    envCtx->stormState = rain ? STORM_STATE_ON : STORM_STATE_OFF;
    envCtx->skyboxConfig = envCtx->changeSkyboxNextConfig = rain ? 1 : 0;
    envCtx->lightConfig = envCtx->changeLightNextConfig = rain ? 2 : 0;
}

/** True for a stop under the water (its title ends "(under)"). */
static s32 PlayBench_Under(const char* title) {
    s32 len = 0;

    while (title[len] != '\0') {
        len++;
    }
    return (len >= 7) && (title[len - 7] == '(') && (title[len - 2] == 'r') && (title[len - 1] == ')');
}


/** The auto-angler's buttons for the rod's state (as FISHMOD_AUTO_ANGLER). */
static u16 PlayBench_Angle(void) {
    switch (gFishmodRodInfo.state) {
        case 0: // rod away
        case 1: // held: cast
            return ((sPbTimer % 40) == 0) ? BTN_CLEFT : 0;
        case 2: // wind-up
            return (gFishmodRodInfo.castPower < 0.5f) ? BTN_CLEFT : 0;
        case 4: // float: strike on a bite
            return (gFishmodRodInfo.biting && ((sPbTimer % 4) == 0)) ? BTN_B : 0;
        case 5: // on the ground: reel back
            return BTN_A;
        case 6: // fight: reel while the tension is comfortable
            return (gFishmodRodInfo.tension < 60.0f) ? BTN_A : 0;
        case 7: // landed: through the catch textbox
            return ((gFishmodRodInfo.choice != 0) && ((sPbTimer % 8) == 0)) ? BTN_A : 0;
    }
    return 0;
}

static void Fishmod_PlayBenchUpdate(PlayState* play) {
    Player* player = GET_PLAYER(play);
#ifdef FISHMOD_BENCH_LEGS
    s32 n = FISHMOD_BENCH_LEGS; // (the short test: a lap of the first few legs)
#else
    s32 n = ARRAY_COUNT(sTourStops);
#endif
    FishmodTourStop* from;
    FishmodTourStop* to;
    s32 mode;
    s32 i;

    if (!Bench_Init() || (player == NULL)) {
        return;
    }
#ifdef FISHMOD_STALL_TEST
    if (sPbTimer == FISHMOD_STALL_TEST) {
        volatile s32 forever = 1;

        while (forever) {} // (checking the watchdog)
    }
#endif
#ifdef FISHMOD_FAULT_TEST
    if (sPbTimer == FISHMOD_FAULT_TEST) {
        *(volatile u32*)0x10 = 0; // (checking the crash record)
    }
#endif
    Fishmod_SampleRdp();
    sPbTimer++;
    sPbStickX = sPbStickY = 0;
    sPbButton = 0;
    sPbActive = true;

    if (sBench.laps >= FISHMOD_BENCH_LAPS) {
        Bench_SetMode(BENCH_BASE);
        sPbActive = false;
        sTourDebugText = true;
        Fishmod_DebugPrintf(2, "BENCH DONE: %d laps saved", sBench.laps);
        Fishmod_DebugPrintf(3, "You can turn the console off");
        Fishmod_DebugPrintf(4, "and send the .sav file");
        return;
    }
    sPbCond = sPbOrder[sBench.laps % BENCH_CONDS];
    // the lake's enemies still bite, but he never runs out of hearts (the bench must outlast the night)
    gSaveContext.save.info.playerData.health = gSaveContext.save.info.playerData.healthCapacity;
    if (sPbState >= 2) {
        gSaveContext.save.dayTime = sPbTimes[sPbCond % 4]; // the clock held
    }
    // no one stops to talk: the owl goes, and any text is passed through
    {
        Actor* actor = play->actorCtx.actorLists[ACTORCAT_NPC].head;

        for (; actor != NULL; actor = actor->next) {
            if (actor->id == ACTOR_EN_OWL) {
                Actor_Kill(actor);
            }
        }
    }
    if ((Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) && ((sPbTimer % 8) == 0)) {
        sPbButton = BTN_A;
    }

    from = &sTourStops[sPbLeg];
    to = &sTourStops[(sPbLeg + 1) % n];
    mode = sPbBlock;
    switch (sPbState) {
        case 0: // a new lap: this lap's time and weather, set in place (as the Song of Storms brings rain)
            Bench_SetMode(BENCH_BASE);
            gSaveContext.save.dayTime = sPbTimes[sPbCond % 4];
            PlayBench_SetRain(play, sPbCond >= 4);
            sPbState = 1;
            sPbFrame = 0;
            sPbLeg = 0;
            sPbBlock = 0;
            break;

        case 1: // the light and the rain settling
            if (++sPbFrame > 100) {
                sPbState = 2;
                sPbFrame = 0;
            }
            break;

        case 2: { // a block: Link on his way along the leg
            f32 dx;
            f32 dz;
            f32 dist;

            if (sPbFrame == 0) {
                Bench_SetMode(mode);
                Bench_Breadcrumb(sPbLeg, mode, sPbBlock);
                // every other leg on the lake bed in the iron boots; the rest in his own boots (swimming, running,
                // fishing)
                Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, (((sPbLeg % 2) == 0) && ((sPbLeg % 3) != 1))
                                                                ? EQUIP_VALUE_BOOTS_IRON
                                                                : EQUIP_VALUE_BOOTS_KOKIRI);
                Player_SetEquipmentData(play, player);
                PlayBench_Place(play, player, from->eye.x, from->eye.y, from->eye.z,
                                Math_Vec3f_Yaw(&from->eye, &to->eye));
                sPbFishing = false;
            }
            if (sPbFrame == 10) {
                // every third leg he fishes, if he's standing on land
                sPbFishing = ((sPbLeg % 3) == 1) && !(player->stateFlags1 & PLAYER_STATE1_27) &&
                             (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND);
            }
            if (sPbFrame >= PB_SKIP) {
                Bench_Record(sPbLeg, mode);
            }
            dx = to->eye.x - player->actor.world.pos.x;
            dz = to->eye.z - player->actor.world.pos.z;
            dist = sqrtf(SQ(dx) + SQ(dz));
            if (sPbFishing && (sPbFrame > 10)) {
                if (sPbFrame < 16) {
                    PlayBench_Steer(play, Math_Atan2S(dz, dx), 20); // face the water first
                } else {
                    sPbButton |= PlayBench_Angle();
                }
            } else if (sPbFrame > 2) {
                if (dist > PB_ARRIVED) {
                    PlayBench_Steer(play, Math_Atan2S(dz, dx), 80);
                }
                // diving where the leg's end is under the water
                if ((player->stateFlags1 & PLAYER_STATE1_27) && PlayBench_Under(to->title) &&
                    ((sPbTimer % 90) == 0)) {
                    sPbButton |= BTN_A;
                }
                // stuck on a wall or a ledge: on along the way
                if (++sPbStuckFrame >= 40) {
                    f32 moved = Math_Vec3f_DistXZ(&player->actor.world.pos, &sPbStuckPos);

                    if ((moved < 25.0f) && (dist > PB_ARRIVED * 2)) {
                        f32 step = 200.0f / dist;

                        PlayBench_Place(play, player, player->actor.world.pos.x + dx * step,
                                        player->actor.world.pos.y, player->actor.world.pos.z + dz * step,
                                        Math_Atan2S(dz, dx));
                    }
                    sPbStuckFrame = 0;
                    sPbStuckPos = player->actor.world.pos;
                }
            }
            if (++sPbFrame >= PB_SKIP + BENCH_FRAMES) {
                sPbState = 3;
                sPbFrame = 0;
            }
            break;
        }

        case 3: // after a block: the line reeled in and any catch put away, then the next mode, or the next leg
            Bench_SetMode(BENCH_BASE);
            if ((gFishmodRodInfo.state > 1) && (sPbFrame++ < PB_CLEANUP)) {
                if (gFishmodRodInfo.state == 7) {
                    sPbButton |= ((sPbTimer % 8) == 0) ? BTN_A : 0;
                } else if (gFishmodRodInfo.state != 2) {
                    sPbButton |= BTN_A;
                }
                break;
            }
            sPbFrame = 0;
            if (++sPbBlock < BENCH_MODES) {
                sPbState = 2;
                Bench_Save(); // after every block: a run cut short keeps what it timed
            } else {
                sPbState = 4;
            }
            break;

        default: // the memory tests, then the next leg
            Bench_Micro();
            if (++sPbFrame >= BENCH_MICRO_FRAMES) {
                sBench.visits++;
                sPbBlock = 0;
                sPbFrame = 0;
                if (++sPbLeg >= n) {
                    sBench.laps++;
                    sPbState = 0;
                } else {
                    sPbState = 2;
                }
                Bench_Save();
            }
            break;
    }
    {
        static const char* sModeNames[] = { "fresh", "base", "fishfrozen", "nofishdraw", "nofish", "noprops",
                                            "nowater", "bare" };

#ifdef FISHMOD_BENCH_SHOWTEXT
        Fishmod_DebugPrintf(3, "pb s%d f%d P %d %d %d st%08X", sPbState, sPbFrame, (s32)player->actor.world.pos.x,
                            (s32)player->actor.world.pos.y, (s32)player->actor.world.pos.z, player->stateFlags1);
#endif
        Fishmod_DebugPrintf(2, "bench lap %d leg %d %s c%d", sBench.laps + 1, sPbLeg + 1,
                            ((sPbState == 2) && (sPbBlock < BENCH_MODES)) ? sModeNames[sPbBlock] : "-", sPbCond);
    }
}

/** The bench's controller: what Fishmod_PlayBenchUpdate decided last frame. */
static s32 PlayBench_Input(Input* input) {
    u16 buttonDiff;

    if (!sPbActive) {
        return false;
    }
    input->prev = input->cur;
    input->cur.button = sPbButton;
    input->cur.stick_x = sPbStickX;
    input->cur.stick_y = sPbStickY;
    buttonDiff = input->prev.button ^ input->cur.button;
    input->press.button = input->cur.button & buttonDiff;
    input->rel.button = input->prev.button & buttonDiff;
    PadUtils_UpdateRelXY(input);
    input->press.stick_x = 0;
    input->press.stick_y = 0;
    return true;
}
#endif

#ifdef FISHMOD_BENCH_AREA
/*
 * The area bench (fishmod/scripts/hwbench_area.inc.c): at each spot Link walks a slow circle, four ways in turn (the
 * iron boots or his own; everything as shipped, or the mod's fish, props, water surface and sky off), and every step
 * of the update (FishmodSect) and every kind of actor is timed. The sums go to SRAM after every spot.
 */
#define AREA_VARIANTS 4
#define AREA_SETTLE 40
#define AREA_FRAMES 200
#define AREA_TOP 16 // actor kinds kept per cell, the costliest
#define AREA_SPOTS ARRAY_COUNT(sAreaSpots)
#define AREA_BARE (FISHMOD_BENCH_NOPROPS | FISHMOD_BENCH_NOFISH | FISHMOD_BENCH_NOFISHAI | FISHMOD_BENCH_NOROOMXLU | \
                   FISHMOD_BENCH_NOSKY)

typedef enum AreaMetric {
    AM_PERIOD,
    AM_CPU_UPDATE,
    AM_CPU_DRAW,
    AM_RSP,
    AM_RDP,
    AM_FISH_UPD,  // the fish's own updates (FISHMOD_PROF_U_ENGINE)
    AM_FISH_DRAW,
    AM_SCATTER,
    AM_ACTORS_N,  // actors updated (count)
    AM_SECT0,     // then FISHMOD_SECT_MAX steps
    AM_CAT0 = AM_SECT0 + FISHMOD_SECT_MAX, // then the actor categories
    AREA_METRICS = AM_CAT0 + FISHMOD_ACTOR_CATS
} AreaMetric;

typedef struct AreaActor {
    u16 id;
    u8 most; // most of it updating in one frame
    u8 pad;
    u32 usec; // summed over the cell's frames
} AreaActor;

typedef struct AreaCell {
    u32 frames;
    u32 sum[AREA_METRICS]; // microseconds (counts for AM_ACTORS_N)
    u32 maxPeriod;
    AreaActor top[AREA_TOP];
} AreaCell;

typedef struct AreaResults {
    u32 magic; // 'FARE'
    u8 version;
    u8 spots;
    u8 variants;
    u8 sects;
    u8 cats;
    u8 top;
    u8 hw;
    u8 pad;
    u32 laps;
    u32 spot; // where it got to (for a run cut short)
    u32 variant;
    AreaCell cell[ARRAY_COUNT(sAreaSpots)][AREA_VARIANTS];
} AreaResults;

static AreaResults sArea;
static u32* sAreaActorUsec; // [spot][variant][FISHMOD_ACTOR_IDS], microseconds
static u8* sAreaActorMost;  // [spot][variant][FISHMOD_ACTOR_IDS]
static s32 sAreaSpot;
static s32 sAreaVariant;
static s32 sAreaFrame;
static s32 sAreaActive;
static s32 sAreaTimer;
static u16 sAreaButton;

/** Called every frame from Fishmod_DebugTiming, before the step timers are cleared: one frame into the cell. */
static void AreaBench_Sample(void) {
    AreaCell* cell;
    u32 v[AREA_METRICS];
    s32 i;
    s32 base;

    if (!sAreaActive || (sAreaFrame <= AREA_SETTLE) || (sAreaActorUsec == NULL)) {
        return;
    }
    cell = &sArea.cell[sAreaSpot][sAreaVariant];
    v[AM_PERIOD] = (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    v[AM_CPU_UPDATE] = sLastCpu[FISHMOD_CPU_UPDATE];
    v[AM_CPU_DRAW] = sLastCpu[FISHMOD_CPU_DRAW];
    v[AM_RSP] = (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
    v[AM_RDP] = (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
    v[AM_FISH_UPD] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_U_ENGINE]);
    v[AM_FISH_DRAW] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_ENGINE]);
    v[AM_SCATTER] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_SCATTER]);
    v[AM_ACTORS_N] = 0;
    for (i = 0; i < FISHMOD_SECT_MAX; i++) {
        v[AM_SECT0 + i] = (u32)OS_CYCLES_TO_USEC(gFishmodSect[i]);
    }
    for (i = 0; i < FISHMOD_ACTOR_CATS; i++) {
        v[AM_CAT0 + i] = (u32)OS_CYCLES_TO_USEC((OSTime)gFishmodCatCycles[i]);
    }
    base = (sAreaSpot * AREA_VARIANTS + sAreaVariant) * FISHMOD_ACTOR_IDS;
    for (i = 0; i < FISHMOD_ACTOR_IDS; i++) {
        if (gFishmodActorCount[i] != 0) {
            v[AM_ACTORS_N] += gFishmodActorCount[i];
            sAreaActorUsec[base + i] += (u32)OS_CYCLES_TO_USEC((OSTime)gFishmodActorCycles[i]);
            sAreaActorMost[base + i] = MAX(sAreaActorMost[base + i], gFishmodActorCount[i]);
        }
    }
    if (v[AM_PERIOD] > 500000) {
        return; // a load or a stall elsewhere: not a frame
    }
    cell->frames++;
    for (i = 0; i < AREA_METRICS; i++) {
        cell->sum[i] += v[i];
    }
    cell->maxPeriod = MAX(cell->maxPeriod, v[AM_PERIOD]);
}

static void AreaBench_Save(void) {
    s32 spot;
    s32 var;
    s32 i;
    s32 k;

    // each cell's costliest kinds of actor, from the full tallies
    for (spot = 0; spot < (s32)AREA_SPOTS; spot++) {
        for (var = 0; var < AREA_VARIANTS; var++) {
            AreaCell* cell = &sArea.cell[spot][var];
            s32 base = (spot * AREA_VARIANTS + var) * FISHMOD_ACTOR_IDS;

            bzero(cell->top, sizeof(cell->top));
            for (i = 0; i < FISHMOD_ACTOR_IDS; i++) {
                u32 us = sAreaActorUsec[base + i];

                if (us == 0) {
                    continue;
                }
                for (k = 0; k < AREA_TOP; k++) {
                    if (us > cell->top[k].usec) {
                        s32 m;

                        for (m = AREA_TOP - 1; m > k; m--) {
                            cell->top[m] = cell->top[m - 1];
                        }
                        cell->top[k].id = i;
                        cell->top[k].most = sAreaActorMost[base + i];
                        cell->top[k].usec = us;
                        break;
                    }
                }
            }
        }
    }
    sArea.magic = 0x46415245; // 'FARE'
    sArea.version = 1;
    sArea.spots = AREA_SPOTS;
    sArea.variants = AREA_VARIANTS;
    sArea.sects = FISHMOD_SECT_MAX;
    sArea.cats = FISHMOD_ACTOR_CATS;
    sArea.top = AREA_TOP;
    sArea.hw = FISHMOD_HW;
    sArea.spot = sAreaSpot;
    sArea.variant = sAreaVariant;
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000), &sArea, sizeof(sArea), OS_WRITE);
}

static void Fishmod_AreaBenchUpdate(PlayState* play) {
    Player* player = GET_PLAYER(play);
    const FishmodAreaSpot* spot;

    if (player == NULL) {
        return;
    }
    if (sAreaActorUsec == NULL) {
        u32 n = AREA_SPOTS * AREA_VARIANTS * FISHMOD_ACTOR_IDS;

        sAreaActorUsec = SYSTEM_ARENA_MALLOC(n * sizeof(u32), "../fishmod_debug.c", __LINE__);
        sAreaActorMost = SYSTEM_ARENA_MALLOC(n, "../fishmod_debug.c", __LINE__);
        if ((sAreaActorUsec == NULL) || (sAreaActorMost == NULL)) {
            Fishmod_DebugPrintf(2, "area bench: no memory");
            sAreaActorUsec = NULL;
            return;
        }
        bzero(sAreaActorUsec, n * sizeof(u32));
        bzero(sAreaActorMost, n);
        // the actor tallies start over with each run: a save from before is left as it was until the first spot
        bzero(&sArea, sizeof(sArea));
    }
    sAreaTimer++;
    sPbStickX = sPbStickY = 0;
    sAreaButton = 0;
    sAreaActive = true;
    gSaveContext.save.info.playerData.health = gSaveContext.save.info.playerData.healthCapacity;
    gSaveContext.save.dayTime = FISHMOD_SCRIPT_TIME;
    {
        Actor* actor = play->actorCtx.actorLists[ACTORCAT_NPC].head;

        for (; actor != NULL; actor = actor->next) {
            if (actor->id == ACTOR_EN_OWL) {
                Actor_Kill(actor);
            }
        }
    }
    if ((Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) && ((sAreaTimer % 8) == 0)) {
        sAreaButton = BTN_A;
    }
    if (sArea.laps >= FISHMOD_BENCH_LAPS) {
        gFishmodBench = 0;
        sAreaActive = false;
        sTourDebugText = true;
        Fishmod_DebugPrintf(2, "BENCH DONE: %d laps saved", sArea.laps);
        Fishmod_DebugPrintf(3, "You can turn the console off");
        Fishmod_DebugPrintf(4, "and send the .sav file");
        return;
    }

    spot = &sAreaSpots[sAreaSpot];
    if (sAreaFrame == 0) {
        // the iron boots on the bed (variants 0, 1) or his own (2, 3); as shipped (0, 2) or the mod's work off (1, 3)
        Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, (sAreaVariant < 2) ? EQUIP_VALUE_BOOTS_IRON
                                                                       : EQUIP_VALUE_BOOTS_KOKIRI);
        Player_SetEquipmentData(play, player);
        PlayBench_Place(play, player, spot->x, spot->probeY, spot->z, (s16)(sAreaTimer * 0x1000));
        gFishmodBench = (sAreaVariant & 1) ? AREA_BARE : 0;
    }
    if (sAreaFrame > 4) {
        // a slow circle: the heading turns once round over the spot's frames
        s16 yaw = (s16)(sAreaFrame * (0x10000 / (AREA_SETTLE + AREA_FRAMES)));

        PlayBench_Steer(play, yaw, 45);
    }
    if (++sAreaFrame > AREA_SETTLE + AREA_FRAMES) {
        sAreaFrame = 0;
        if (++sAreaVariant >= AREA_VARIANTS) {
            sAreaVariant = 0;
            if (++sAreaSpot >= (s32)AREA_SPOTS) {
                sAreaSpot = 0;
                sArea.laps++;
            }
            AreaBench_Save();
        }
    }
    Fishmod_DebugPrintf(2, "area lap %d spot %d %s v%d", sArea.laps + 1, sAreaSpot + 1, spot->name, sAreaVariant);
}

static s32 AreaBench_Input(Input* input) {
    u16 buttonDiff;

    if (!sAreaActive) {
        return false;
    }
    input->prev = input->cur;
    input->cur.button = sAreaButton;
    input->cur.stick_x = sPbStickX;
    input->cur.stick_y = sPbStickY;
    buttonDiff = input->prev.button ^ input->cur.button;
    input->press.button = input->cur.button & buttonDiff;
    input->rel.button = input->prev.button & buttonDiff;
    PadUtils_UpdateRelXY(input);
    input->press.stick_x = 0;
    input->press.stick_y = 0;
    return true;
}
#endif
#endif

static void Fishmod_TourDraw(Gfx** gfxP) {
    FishmodTourStop* stop = &sTourStops[sTourStop];
    GfxPrint printer;
    Gfx* gfx = *gfxP;
    s32 i;

#if defined(FISHMOD_LOD_CALIBRATE) || defined(FISHMOD_HWBENCH)
    return; // nothing over the frames being compared or timed
#endif

#ifdef FISHMOD_EDGE_AB
    // for fishmod/tools/edge_ab_capture.py: red = 8 * stop number, green 0 (default edge mode) or 248 (the other),
    // blue 248 while ready; nothing else drawn over the picture
    gfx = Gfx_SetupDL_57(gfx);
    gDPSetPrimColor(gfx++, 0, 0, (sTourStop + 1) * 8, (sEdgeAbPhase == 1) ? 248 : 0, (sEdgeAbPhase >= 0) ? 248 : 0,
                    255);
    gDPFillRectangle(gfx++, 0, 0, 16, 16);
    gDPPipeSync(gfx++);
    *gfxP = gfx;
    return;
#endif

    // a dark box at the bottom, like a textbox
    gfx = Gfx_SetupDL_57(gfx);
    gDPSetPrimColor(gfx++, 0, 0, 0, 10, 40, 170);
    gDPSetAlphaDither(gfx++, G_AD_DISABLE);
    gDPSetColorDither(gfx++, G_CD_DISABLE);
    gDPFillRectangle(gfx++, 18, 166, SCREEN_WIDTH - 18, 226);
    gDPPipeSync(gfx++);
#ifdef FISHMOD_TOUR_AUTO
    // for fishmod/tools/tour_capture.py: which stop has settled on screen (red = 10 * stop number), black while gliding
    gDPSetPrimColor(gfx++, 0, 0, (sTourFrame > TOUR_GLIDE + 10) ? (sTourStop + 1) * 8 : 0, 0, 0, 255);
    gDPFillRectangle(gfx++, 0, 0, 16, 16);
    gDPPipeSync(gfx++);
#endif

    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, gfx);
    GfxPrint_SetColor(&printer, 140, 200, 255, 255);
    GfxPrint_SetPos(&printer, 3, 21);
    GfxPrint_Printf(&printer, "%d/%d %s", sTourStop + 1, (s32)ARRAY_COUNT(sTourStops), stop->title);
    GfxPrint_SetColor(&printer, 255, 255, 255, 255);
    for (i = 0; i < 3; i++) {
        if (stop->lines[i] != NULL) {
            GfxPrint_SetPos(&printer, 3, 23 + i);
            GfxPrint_Printf(&printer, "%s", stop->lines[i]);
        }
    }
    GfxPrint_SetColor(&printer, 255, 255, 120, 255);
    GfxPrint_SetPos(&printer, 18, 27); // (ends at column 36: inside the box, which ends at x 302)
    GfxPrint_Printf(&printer, "A>  B<  stick:look");
    {
        // the frame rate, averaged over 20 frames (the time between graph updates)
        static u32 sSumUsec;
        static s32 sCount;
        static s32 sFpsTenths;

        sSumUsec += (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
        if (++sCount >= 20) {
            sFpsTenths = (sSumUsec != 0) ? (s32)(20u * 10000000u / sSumUsec) : 0;
            sSumUsec = 0;
            sCount = 0;
        }
        GfxPrint_SetColor(&printer, 255, 255, 255, 220);
        GfxPrint_SetPos(&printer, 31, 2);
        GfxPrint_Printf(&printer, "%2d.%dfps", sFpsTenths / 10, sFpsTenths % 10);
    }
    *gfxP = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);
}
#endif

static s32 sScriptStepIndex;
static s32 sScriptStepFrame;
static s32 sScriptActive;
static s32 sPlayFrames;
static s32 sPlacedPlayer;
static char sDebugLines[FISHMOD_DEBUG_LINES][FISHMOD_DEBUG_LINE_LEN];

s32 gFishmodNoPropCol;

void Fishmod_DebugBoot(void) {
#ifdef FISHMOD_SCRIPT_NO_PROPCOL
    gFishmodNoPropCol = true;
#endif
    gSaveContext.save.linkAge = LINK_AGE_CHILD;
    gSaveContext.fileNum = 0xFF;
    Sram_InitDebugSave();
    SET_EVENTCHKINF(EVENTCHKINF_B1); // Lake Hylia's intro cutscene seen: it would take the camera from the tour

    gSaveContext.save.dayTime = CLOCK_TIME(10, 0);
#ifdef FISHMOD_SCRIPT_TIME
    gSaveContext.save.dayTime = FISHMOD_SCRIPT_TIME;
#endif
    // day 1: a clear day at Kokiri Forest and Lake Hylia (the weather is rolled per spot per day)
    gSaveContext.save.totalDays = 1;
#ifdef FISHMOD_SCRIPT_DAY
    gSaveContext.save.totalDays = FISHMOD_SCRIPT_DAY;
#endif
    gSaveContext.save.cutsceneIndex = 0;

    // Rod on C-Left, two empty bottles on C-Down / C-Right slots; the lures, and bottles 5 and 6 (one with milk).
    gSaveContext.save.info.inventory.items[SLOT_BOTTLE_1] = ITEM_BOTTLE_EMPTY;
    gSaveContext.save.info.inventory.items[SLOT_BOTTLE_2] = ITEM_BOTTLE_EMPTY;
    INV_SLOT(SLOT_FISHING_ROD) = ITEM_FISHING_POLE;
    INV_SLOT(SLOT_LURE_1) = ITEM_LURE_CRANKBAIT;
    INV_SLOT(SLOT_LURE_2) = ITEM_LURE_FROG;
    INV_SLOT(SLOT_LURE_3) = ITEM_LURE_SINKING;
    INV_SLOT(SLOT_BOTTLE_5) = ITEM_BOTTLE_EMPTY;
    INV_SLOT(SLOT_BOTTLE_6) = ITEM_BOTTLE_MILK_FULL;
    gSaveContext.save.info.equips.buttonItems[1] = ITEM_FISHING_POLE;
    gSaveContext.save.info.equips.cButtonSlots[0] = SLOT_FISHING_ROD;
    gSaveContext.save.info.equips.buttonItems[2] = ITEM_BOTTLE_EMPTY;
    gSaveContext.save.info.equips.cButtonSlots[1] = SLOT_BOTTLE_1;
    gSaveContext.save.info.equips.buttonItems[3] = ITEM_BOTTLE_EMPTY;
    gSaveContext.save.info.equips.cButtonSlots[2] = SLOT_BOTTLE_2;
#ifdef FISHMOD_SCRIPT_BUGS
    // bugs in both C bottles
    gSaveContext.save.info.inventory.items[SLOT_BOTTLE_1] = ITEM_BOTTLE_BUG;
    gSaveContext.save.info.equips.buttonItems[2] = ITEM_BOTTLE_BUG;
    gSaveContext.save.info.inventory.items[SLOT_BOTTLE_2] = ITEM_BOTTLE_BUG;
    gSaveContext.save.info.equips.buttonItems[3] = ITEM_BOTTLE_BUG;
#endif

    gSaveContext.buttonStatus[0] = gSaveContext.buttonStatus[1] = gSaveContext.buttonStatus[2] =
        gSaveContext.buttonStatus[3] = gSaveContext.buttonStatus[4] = BTN_ENABLED;
#ifdef FISHMOD_SCRIPT_ENTRANCE
    gSaveContext.save.entranceIndex = FISHMOD_SCRIPT_ENTRANCE;
#else
    gSaveContext.save.entranceIndex = ENTR_KOKIRI_FOREST_0;
#endif
#ifdef FISHMOD_JOURNAL_TEST
    // a few catches for the journal pages
    FISHMOD_SAVE->records[FISHMOD_SPECIES_GREENGILL].count = 7;
    FISHMOD_SAVE->records[FISHMOD_SPECIES_GREENGILL].bestTenths = 94;
    FISHMOD_SAVE->records[FISHMOD_SPECIES_HYRULE_BASS].count = 2;
    FISHMOD_SAVE->records[FISHMOD_SPECIES_HYRULE_BASS].bestTenths = 168;
    FISHMOD_SAVE->records[FISHMOD_SPECIES_HYRULE_MINNOW].count = 12;
    FISHMOD_SAVE->records[FISHMOD_SPECIES_HYRULE_MINNOW].bestTenths = 41;
#endif
#ifdef FISHMOD_SCRIPT_ADULT
    // adult Link with the Iron Boots and Zora Tunic on (the debug save owns them), for walking the bottom of deep water
    gSaveContext.save.linkAge = LINK_AGE_ADULT;
    gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_MASTER;
    Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
    Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_IRON);
    Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_ZORA); // no drowning
    Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_HYLIAN);
    SET_EVENTCHKINF(EVENTCHKINF_RESTORED_LAKE_HYLIA); // the lake full, as the test build has it
#endif
#ifdef FISHMOD_SCRIPT_EMPTY_ITEMS
    // nothing on the item page (the pause-menu mockups draw their own icons on it)
    memset(gSaveContext.save.info.inventory.items, ITEM_NONE, sizeof(gSaveContext.save.info.inventory.items));
    memset(FISHMOD_SAVE->items, ITEM_NONE, sizeof(FISHMOD_SAVE->items));
    gSaveContext.save.info.equips.buttonItems[1] = ITEM_NONE;
    gSaveContext.save.info.equips.cButtonSlots[0] = SLOT_NONE;
    gSaveContext.save.info.equips.buttonItems[2] = gSaveContext.save.info.equips.buttonItems[3] = ITEM_NONE;
    gSaveContext.save.info.equips.cButtonSlots[1] = gSaveContext.save.info.equips.cButtonSlots[2] = SLOT_NONE;
#endif
    gSaveContext.respawnFlag = 0;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = ENTR_LOAD_OPENING;
    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
    gSaveContext.natureAmbienceId = 0xFF;
    gSaveContext.showTitleCard = false;
    gSaveContext.gameMode = GAMEMODE_NORMAL;

#ifdef FISHMOD_SCRIPT_ALTEDGE
    gFishmodBench |= FISHMOD_BENCH_ALTEDGE; // the props' other edge mode from the start
#endif
    sScriptStepIndex = 0;
    sScriptStepFrame = 0;
    sScriptActive = false;
    sPlayFrames = 0;
    sPlacedPlayer = false;
}

/** True if the script spawns its own fish, so the fish object is needed even in scenes without fishing. */
s32 Fishmod_DebugWantsFish(void) {
    return false;
}

s32 Fishmod_DebugNormalBoot(void) {
#ifdef FISHMOD_SCRIPT_NORMAL_BOOT
    // the script plays from power-on: through the title and file select, and Link is never moved
    sScriptStepIndex = 0;
    sScriptStepFrame = 0;
    sScriptActive = true;
    sPlacedPlayer = true;
    return true;
#else
    return false;
#endif
}

OSTime gFishmodCpuTime[FISHMOD_CPU_MAX];
OSTime gFishmodSect[FISHMOD_SECT_MAX];
u32 gFishmodActorCycles[FISHMOD_ACTOR_IDS];
u8 gFishmodActorCount[FISHMOD_ACTOR_IDS];
u32 gFishmodCatCycles[FISHMOD_ACTOR_CATS];
#ifndef FISHMOD_HWBENCH
static u32 sLastCpu[FISHMOD_CPU_MAX];
#endif
OSTime gFishmodLastProf[FISHMOD_PROF_MAX]; // the profile timers over the last frame (before they're cleared)
s32 gFishmodBench;                         // FISHMOD_BENCH_* flags (the hardware bench switches things off)
s32 gFishmodScatterDrawn;                  // props drawn in the last frame
OSTime gFishmodProf[FISHMOD_PROF_MAX];
u32 gFishmodProfCount[FISHMOD_PROF_MAX];

/**
 * Averages over 60 frames, in tenths of a millisecond: RSP graphics and RDP time, the CPU time of Play_Update,
 * Play_Draw and the fishmod actors, and the whole frame.
 */
#ifndef FISHMOD_STATS_SKIP
#define FISHMOD_STATS_SKIP 600 // frames before the whole-run stats start (scene load, settling)
#endif
static s32 sStatsFrames;

/**
 * The fish profile timers (FISHMOD_PROF): whole-run totals per slot, written to SRAM at 0x7E00 next to the timing
 * stats, for fishmod/tools/read_stats.py --prof.
 */
static void Fishmod_DebugProfile(s32 record) {
    static struct {
        u32 magic; // 'FMPR'
        u32 frames;
        u32 usec[FISHMOD_PROF_MAX];  // microseconds over the run
        u32 calls[FISHMOD_PROF_MAX]; // timed blocks over the run
    } sProf;
    static OSTime sTotal[FISHMOD_PROF_MAX];
    s32 i;

    // what a timer costs on its own, to subtract from slots that are timed many times a frame
    for (i = 0; i < 8; i++) {
        FISHMOD_PROF(FISHMOD_PROF_CALIBRATE, (void)0);
    }
    if (record) {
        sProf.magic = 0x464D5052;
        sProf.frames++;
        for (i = 0; i < FISHMOD_PROF_MAX; i++) {
            sTotal[i] += gFishmodProf[i];
            sProf.usec[i] = (u32)OS_CYCLES_TO_USEC(sTotal[i]);
            sProf.calls[i] += gFishmodProfCount[i];
        }
        if ((sProf.frames % 60) == 0) {
#ifndef FISHMOD_PLAYLOG // (the play log has the end of SRAM)
            SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7E00, &sProf, sizeof(sProf), OS_WRITE);
#endif
        }
    }
    bcopy(gFishmodProf, gFishmodLastProf, sizeof(gFishmodLastProf));
    for (i = 0; i < FISHMOD_PROF_MAX; i++) {
        gFishmodProf[i] = 0;
        gFishmodProfCount[i] = 0;
    }
}

static void Fishmod_DebugTiming(void) {
    static u32 sSum[2 + FISHMOD_CPU_MAX + 1];
    static u32 sShown[2 + FISHMOD_CPU_MAX + 1];
    static s32 sCount;
    s32 i;

    sSum[0] += (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
    sSum[1] += (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
    for (i = 0; i < FISHMOD_CPU_MAX; i++) {
        sLastCpu[i] = (u32)OS_CYCLES_TO_USEC(gFishmodCpuTime[i]);
        sSum[2 + i] += sLastCpu[i];
        gFishmodCpuTime[i] = 0;
    }
    sSum[2 + FISHMOD_CPU_MAX] += (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    if (++sCount >= 60) {
        for (i = 0; i < ARRAY_COUNT(sSum); i++) {
            sShown[i] = sSum[i] / (sCount * 100);
            sSum[i] = 0;
        }
        sCount = 0;
    }
    Fishmod_DebugPrintf(9, "r%d up%d dr%d f%d/%d r%d/%d F%d", sShown[1], sShown[2], sShown[3], sShown[4],
                        sShown[5], sShown[6], sShown[7], sShown[8]);

    // Whole-run averages, written to the end of SRAM every second so the emulator's save file can be read
    // afterwards (fishmod/tools/read_stats.py): exact numbers to compare builds with.
    {
        static struct {
            u32 magic; // 'FMST'
            u32 frames;
            u32 sum[2 + FISHMOD_CPU_MAX + 2]; // microseconds, summed over `frames`
            u32 max[2 + FISHMOD_CPU_MAX + 2]; // worst single frame
        } sStats;
        u32 now[2 + FISHMOD_CPU_MAX + 2];

        now[0] = (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
        now[1] = (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
        for (i = 0; i < FISHMOD_CPU_MAX; i++) {
            now[2 + i] = sLastCpu[i];
        }
        now[2 + FISHMOD_CPU_MAX] = (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
        now[3 + FISHMOD_CPU_MAX] = (u32)OS_CYCLES_TO_USEC(gRSPAudioTimeTotal);
        if (sStatsFrames++ >= FISHMOD_STATS_SKIP) {
            sStats.magic = 0x464D5354;
            sStats.frames++;
            for (i = 0; i < ARRAY_COUNT(now); i++) {
                sStats.sum[i] += now[i];
                if (now[i] > sStats.max[i]) {
                    sStats.max[i] = now[i];
                }
            }
            if ((sStats.frames % 60) == 0) {
#ifndef FISHMOD_PLAYLOG // (the play log has the end of SRAM)
                SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7F00, &sStats, sizeof(sStats), OS_WRITE);
#endif
            }
        }
    }
    Fishmod_DebugProfile(sStatsFrames > FISHMOD_STATS_SKIP);
#ifdef FISHMOD_BENCH_AREA
    AreaBench_Sample();
#endif
    bzero(gFishmodSect, sizeof(gFishmodSect));
    bzero(gFishmodActorCycles, sizeof(gFishmodActorCycles));
    bzero(gFishmodActorCount, sizeof(gFishmodActorCount));
    bzero(gFishmodCatCycles, sizeof(gFishmodCatCycles));
}

s32 Fishmod_DebugRosterCount(void) {
#ifdef FISHMOD_SCRIPT_ROSTER
    return FISHMOD_SCRIPT_ROSTER;
#else
    return 0;
#endif
}

#ifdef FISHMOD_PLAYLOG
/**
 * The play log (fishmod/scripts/playlog.inc.c): while the game is played normally at Lake Hylia, where and why frames
 * run slow. Per situation (camera under water or not, the weather, the rod out or not): frames and their summed times.
 * And the slowest seconds, at most one per spot, each with where Link and the camera were and what was going on.
 * Written to the end of SRAM, after the six save slots (0x7A30), so the saves are untouched; every 30 seconds, and the
 * frames after a write aren't counted. Read: fishmod/tools/playlog_report.py save.sav
 */
#define PLOG_ADDR 0x7A40
#define PLOG_WINDOW 20  // frames per sample ("second")
#define PLOG_WORST 24
#define PLOG_SITS 12    // under (2) x weather (3) x rod (2)
#define PLOG_SPOT 400.0f // two samples closer than this (Link's position) are the same spot

typedef struct PlogSit {
    u32 frames;
    u32 slow; // frames over 55 ms (under 18 fps)
    u32 sum[5]; // period, CPU update, CPU draw, RSP, RDP: 10 us units
} PlogSit; // size = 0x1C

typedef struct PlogMoment {
    u16 period; // average over the sample, 0.1 ms
    u16 periodMax;
    u16 upd;
    u16 draw;
    u16 rsp;
    u16 rdp;
    u16 fishUpd;
    u16 fishDraw;
    u16 scatter;
    u16 props;
    s16 link[3];
    s16 eye[3];
    s16 camYaw;   // the camera's facing
    s16 camPitch;
    u16 dayTime;
    u16 second;   // when (seconds of logging)
    u8 weather;
    u8 flags;     // 1: camera under water, 2: rod out, 4: Link swimming, 8: fish on the line
    u8 fish;      // fishmod actors (fish, rod, manager)
    u8 actors;    // all actors
} PlogMoment; // size = 0x2C

typedef struct PlogLog {
    u32 magic; // 'FPLG'
    u16 version;
    u16 worstCount;
    u32 frames;
    u32 samples;
    PlogSit sit[PLOG_SITS];
    PlogMoment worst[PLOG_WORST];
} PlogLog;

static PlogLog sPlog;
static u32 sPlogSum[11]; // period, upd, draw, rsp, rdp, fishUpd, fishDraw, scatter, props (us/count) + max period
static s32 sPlogFrames;
static s32 sPlogSkip;
static s32 sPlogSinceSave;

static void Fishmod_PlayLog(PlayState* play) {
    Player* player = GET_PLAYER(play);
    u32 v[9];
    s32 under;
    s32 weather;
    s32 rod;
    s32 i;

    if ((play->sceneId != SCENE_LAKE_HYLIA) || (player == NULL) || (play->pauseCtx.state != PAUSE_STATE_OFF) ||
        (play->transitionMode != TRANS_MODE_OFF)) {
        sPlogFrames = 0;
        bzero(sPlogSum, sizeof(sPlogSum));
        return;
    }
    if (sPlogSkip > 0) {
        sPlogSkip--;
        return;
    }
    v[0] = (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    v[1] = sLastCpu[FISHMOD_CPU_UPDATE];
    v[2] = sLastCpu[FISHMOD_CPU_DRAW];
    v[3] = (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
    v[4] = (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
    v[5] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_U_ENGINE]);
    v[6] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_ENGINE]);
    v[7] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_SCATTER]);
    v[8] = gFishmodScatterDrawn;
    if (v[0] > 500000) { // a hitch from something else (a load): not a frame
        return;
    }

    under = gFishmodHaze >= 0.5f;
    weather = CLAMP(Fishmod_Weather(play), 0, 2);
    rod = gFishmodRodInfo.state != 0;
    {
        PlogSit* sit = &sPlog.sit[(under * 3 + weather) * 2 + rod];

        sit->frames++;
        sit->slow += v[0] > 55000;
        for (i = 0; i < 5; i++) {
            sit->sum[i] += v[i] / 10;
        }
    }
    sPlog.frames++;
    for (i = 0; i < 9; i++) {
        sPlogSum[i] += v[i];
    }
    sPlogSum[9] = MAX(sPlogSum[9], v[0]);

    if (++sPlogFrames >= PLOG_WINDOW) {
        PlogMoment m;
        s32 slot = -1;
        Camera* cam = GET_ACTIVE_CAM(play);
        Actor* actor;

        m.period = sPlogSum[0] / (PLOG_WINDOW * 100);
        m.periodMax = sPlogSum[9] / 100;
        m.upd = sPlogSum[1] / (PLOG_WINDOW * 100);
        m.draw = sPlogSum[2] / (PLOG_WINDOW * 100);
        m.rsp = sPlogSum[3] / (PLOG_WINDOW * 100);
        m.rdp = sPlogSum[4] / (PLOG_WINDOW * 100);
        m.fishUpd = sPlogSum[5] / (PLOG_WINDOW * 100);
        m.fishDraw = sPlogSum[6] / (PLOG_WINDOW * 100);
        m.scatter = sPlogSum[7] / (PLOG_WINDOW * 100);
        m.props = sPlogSum[8] / PLOG_WINDOW;
        m.link[0] = player->actor.world.pos.x;
        m.link[1] = player->actor.world.pos.y;
        m.link[2] = player->actor.world.pos.z;
        m.eye[0] = play->view.eye.x;
        m.eye[1] = play->view.eye.y;
        m.eye[2] = play->view.eye.z;
        m.camYaw = Math_Vec3f_Yaw(&play->view.eye, &play->view.at);
        m.camPitch = Math_Vec3f_Pitch(&play->view.eye, &play->view.at);
        m.dayTime = gSaveContext.save.dayTime;
        m.second = sPlog.samples;
        m.weather = weather;
        m.flags = under | (rod << 1) | (((player->stateFlags1 & PLAYER_STATE1_27) != 0) << 2) |
                  ((gFishmodRodInfo.tension > 0.0f) << 3);
        m.fish = 0;
        for (actor = play->actorCtx.actorLists[ACTORCAT_NPC].head; actor != NULL; actor = actor->next) {
            m.fish += (actor->id == ACTOR_EN_FISHMOD);
        }
        m.actors = MIN(play->actorCtx.total, 255);
        (void)cam;
        sPlog.samples++;

        // one per spot: a slower sample replaces the one there; else the fastest of the list, if this is slower
        for (i = 0; i < sPlog.worstCount; i++) {
            PlogMoment* w = &sPlog.worst[i];
            f32 dx = w->link[0] - m.link[0];
            f32 dz = w->link[2] - m.link[2];

            if ((SQ(dx) + SQ(dz) < SQ(PLOG_SPOT)) && ((w->flags & 1) == (m.flags & 1))) {
                slot = (m.period > w->period) ? i : -2;
                break;
            }
        }
        if (slot == -1) {
            if (sPlog.worstCount < PLOG_WORST) {
                slot = sPlog.worstCount++;
            } else {
                s32 best = 0;

                for (i = 1; i < PLOG_WORST; i++) {
                    if (sPlog.worst[i].period < sPlog.worst[best].period) {
                        best = i;
                    }
                }
                if (m.period > sPlog.worst[best].period) {
                    slot = best;
                }
            }
        }
        if (slot >= 0) {
            sPlog.worst[slot] = m;
        }
        sPlogFrames = 0;
        bzero(sPlogSum, sizeof(sPlogSum));

        if (++sPlogSinceSave >= 30) {
            sPlogSinceSave = 0;
            sPlog.magic = 0x46504C47;
            sPlog.version = 1;
            SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + PLOG_ADDR, &sPlog, sizeof(sPlog), OS_WRITE);
            sPlogSkip = 3; // the write's own hitch isn't the game's
        }
    }
}
#endif

#ifdef FISHMOD_SCRIPT_WALK_TO
// the script's walk: Link is steered (by the camera, as the stick works) toward a point, from frame 60 until there
static s8 sWalkStickX;
static s8 sWalkStickY;
static s32 sWalking;
#endif
#ifdef FISHMOD_HWBENCH
static void Watchdog_Start(void);
static volatile u32 sWatchdogBeat;
#endif

#ifdef FISHMOD_SCRIPT
/*
 * State fingerprints for refactoring: every 200 frames after the script starts, a hash of every actor (id, params,
 * position, rotation) to SRAM 0x7A00 ('FHSH', count, 14 hashes). A script run is deterministic in ares (fixed seed,
 * scripted input), so two builds that should behave the same must write the same hashes
 * (fishmod/tools/state_hash.py compares them).
 */
typedef struct FishmodStateHashes {
    u32 magic;
    u32 count;
    u32 hash[14];
} FishmodStateHashes;

static void StateHash_Update(PlayState* play) {
    static FishmodStateHashes sHashes;
    static s32 sFrames;
    u32 h = 2166136261u;
    s32 cat;

    if (!sScriptActive || (sHashes.count >= ARRAY_COUNT(sHashes.hash))) {
        return;
    }
    if ((++sFrames % 200) != 0) {
        return;
    }
    for (cat = 0; cat < ACTORCAT_MAX; cat++) {
        Actor* actor;

        for (actor = play->actorCtx.actorLists[cat].head; actor != NULL; actor = actor->next) {
            u32 w[6];
            s32 k;

            w[0] = ((u32)actor->id << 16) | (u16)actor->params;
            bcopy(&actor->world.pos.x, &w[1], sizeof(f32));
            bcopy(&actor->world.pos.y, &w[2], sizeof(f32));
            bcopy(&actor->world.pos.z, &w[3], sizeof(f32));
            w[4] = ((u32)(u16)actor->shape.rot.x << 16) | (u16)actor->shape.rot.y;
            w[5] = (u32)(u16)actor->shape.rot.z;
            for (k = 0; k < 6; k++) {
                h = (h ^ w[k]) * 16777619u;
            }
        }
    }
    sHashes.magic = 0x46485348; // 'FHSH'
    sHashes.hash[sHashes.count++] = h;
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7A00, &sHashes, sizeof(sHashes), OS_WRITE);
}
#endif

void Fishmod_DebugPlayUpdate(PlayState* play) {
    Player* player = GET_PLAYER(play);

    sPlayFrames++;
#ifdef FISHMOD_SCRIPT
    StateHash_Update(play);
#endif
#ifdef FISHMOD_SCRIPT_WALK_TO
    if ((player != NULL) && sPlacedPlayer && (sPlayFrames > 60)) {
        static const Vec3f sTo = FISHMOD_SCRIPT_WALK_TO;
        f32 dx = sTo.x - player->actor.world.pos.x;
        f32 dz = sTo.z - player->actor.world.pos.z;

        sWalking = (SQ(dx) + SQ(dz)) > SQ(30.0f);
#ifdef FISHMOD_SCRIPT_WALK_PROBE
        {
            // his highest point while near the probe (a rock's middle): does he step up onto it?
            static const Vec3f sProbe = FISHMOD_SCRIPT_WALK_PROBE;
            static f32 sTop = -32000.0f;
            static f32 sLow = 32000.0f;

            if (SQ(player->actor.world.pos.x - sProbe.x) + SQ(player->actor.world.pos.z - sProbe.z) < SQ(30.0f)) {
                sTop = MAX(sTop, player->actor.world.pos.y);
            } else {
                sLow = MIN(sLow, player->actor.world.pos.y);
            }
            Fishmod_DebugPrintf(4, "walk probe: highest near %d, lowest off %d", (s32)sTop, (s32)sLow);
        }
#endif
        if (sWalking) {
            s16 rel = Math_Atan2S(dz, dx) - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));

            sWalkStickX = (s8)(-Math_SinS(rel) * 60);
            sWalkStickY = (s8)(Math_CosS(rel) * 60);
        }
    }
#endif
#ifdef FISHMOD_HWBENCH
    sWatchdogBeat = sPlayFrames;
    Watchdog_Start();
#endif
    Fishmod_DebugTiming();
#ifdef FISHMOD_PLAYLOG
    Fishmod_PlayLog(play);
#endif

    // Wait for the scene transition to finish, then place Link at the test spot and start the script.
    if (!sPlacedPlayer && (player != NULL) && (sPlayFrames > 20) && (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
        (play->transitionMode == TRANS_MODE_OFF)) {
        player->actor.world.pos = player->actor.prevPos = player->actor.home.pos = sScriptStartPos;
        player->actor.shape.rot.y = player->actor.world.rot.y = player->yaw = sScriptStartYaw;
        sPlacedPlayer = true;
#ifdef FISHMOD_SCRIPT
        sScriptActive = true;
        // the same random numbers every run, so benchmark runs of different builds start alike
        Rand_Seed(0x46495348);
#endif
#ifdef FISHMOD_SCRIPT_TEST_RELEASE
        {
            // a fish with known looks in the first bottle, poured into the water ahead (the fish that appears
            // reports its looks: they must match)
            Vec3f at = FISHMOD_SCRIPT_TEST_RELEASE;

            FISHMOD_SAVE->bottleFish[0].species = FISHMOD_SPECIES_SANKE_CARP + 1;
            FISHMOD_SAVE->bottleFish[0].lengthTenths = 180;
            FISHMOD_SAVE->bottleFish[0].look = 0xAB;
            gSaveContext.save.info.inventory.items[SLOT_BOTTLE_1] = ITEM_BOTTLE_FISH;
            Fishmod_ReleaseBottleFish(play, SLOT_BOTTLE_1, &at, player->actor.shape.rot.y);
        }
#endif
#ifdef FISHMOD_SCRIPT_TEST_HIVE
        // a beehive just in front of Link, low enough for the sword
        Actor_Spawn(&play->actorCtx, play, ACTOR_OBJ_COMB,
                    player->actor.world.pos.x + Math_SinS(player->actor.shape.rot.y) * 28.0f,
                    player->actor.world.pos.y + 18.0f,
                    player->actor.world.pos.z + Math_CosS(player->actor.shape.rot.y) * 28.0f, 0, 0, 0, 0xFF02);
#endif
    }

#ifdef FISHMOD_VIEW
#ifndef FISHMOD_VIEW_START
#define FISHMOD_VIEW_START 40 // frames before the fixed camera takes over (Link can't act while it's on)
#endif
    if (sPlacedPlayer && (sPlayFrames > FISHMOD_VIEW_START)) {
        Fishmod_ViewUpdate(play);
    }
#endif
#if defined(FISHMOD_BENCH_AREA)
    if (sPlacedPlayer && (sPlayFrames > 40)) {
        Fishmod_AreaBenchUpdate(play);
    }
#elif defined(FISHMOD_BENCH_PLAY)
    if (sPlacedPlayer && (sPlayFrames > 40)) {
        Fishmod_PlayBenchUpdate(play);
    }
#elif defined(FISHMOD_TOUR)
    if (sPlacedPlayer && (sPlayFrames > 40)) {
        Fishmod_TourUpdate(play);
    }
#endif
#ifdef FISHMOD_PHOTO
    if (sPlacedPlayer && (sPlayFrames > 60)) {
        Fishmod_PhotoUpdate(play);
    }
#endif
#ifdef FISHMOD_ANIM_STUDIO
    if (sPlacedPlayer && (sPlayFrames > 60)) {
        Fishmod_StudioUpdate(play);
    }
#endif

    Fishmod_DebugPrintf(0, "F%d S%d/%d c%d l%d t%02d:%02d s%d", sPlayFrames, sScriptStepIndex,
                        (s32)ARRAY_COUNT(sScriptSteps), gFishmodRodInfo.caught, gFishmodRodInfo.lost,
                        (s32)gSaveContext.save.dayTime * 24 / 0x10000,
                        (s32)gSaveContext.save.dayTime * 24 * 60 / 0x10000 % 60, gTimeSpeed);
    if (player != NULL) {
        Fishmod_DebugPrintf(1, "P %d %d %d y%04X ovl%08X", (s32)player->actor.world.pos.x,
                            (s32)player->actor.world.pos.y, (s32)player->actor.world.pos.z,
                            (u16)player->actor.shape.rot.y, (u32)gActorOverlayTable[ACTOR_EN_FISHMOD].loadedRamAddr);
    }
    if (0) {
        Fishmod_DebugPrintf(1, "P %d %d %d y%04X", (s32)player->actor.world.pos.x, (s32)player->actor.world.pos.y,
                            (s32)player->actor.world.pos.z, (u16)player->actor.shape.rot.y);
    }
}

void Fishmod_AutoplayInput(GameState* gameState, Input* inputs) {
    Input* input = &inputs[0];
    FishmodAutoplayStep* step;
    u16 buttonDiff;

#ifdef FISHMOD_SCRIPT_WALK_TO
    if (sWalking) {
        input->prev = input->cur;
        input->cur.button = 0;
        input->cur.stick_x = sWalkStickX;
        input->cur.stick_y = sWalkStickY;
        input->press.button = input->rel.button = 0;
        PadUtils_UpdateRelXY(input);
        input->press.stick_x = input->press.stick_y = 0;
        return;
    }
#endif
#ifdef FISHMOD_BENCH_PLAY
    if (PlayBench_Input(input)) {
        return;
    }
#endif
#ifdef FISHMOD_BENCH_AREA
    if (AreaBench_Input(input)) {
        return;
    }
#endif
#ifdef FISHMOD_TOUR
    // the tour has the controller: Link stands still
    if (sTourCamId != SUB_CAM_ID_DONE) {
        sTourPress |= input->press.button;
        sTourStickX = input->cur.stick_x;
        sTourStickY = input->cur.stick_y;
        input->cur.button = input->press.button = input->rel.button = 0;
        input->cur.stick_x = input->cur.stick_y = 0;
        input->rel.stick_x = input->rel.stick_y = 0;
        input->press.stick_x = input->press.stick_y = 0;
        return;
    }
#endif

#ifdef FISHMOD_AUTO_ANGLER
    // after the script: play the fishing game from the rod's state (for testing the whole loop)
    if (!sScriptActive && sPlacedPlayer && (sScriptStepIndex >= (s32)ARRAY_COUNT(sScriptSteps))) {
        static s32 sAnglerTimer;
        static s32 sAnglerLastState;
        static s32 sAnglerLastBiting;
        static s32 sAnglerCasts;
        static s32 sAnglerBites;
        static s32 sAnglerLanded;
        static s32 sAnglerFloatTimer;
        u16 button = 0;
        s8 stickY = 0;

        sAnglerTimer++;
        // tallies, to compare builds by how the fishing goes
        if ((gFishmodRodInfo.state == 4) && (sAnglerLastState != 4)) {
            sAnglerCasts++;
        }
        if (gFishmodRodInfo.biting && !sAnglerLastBiting) {
            sAnglerBites++;
        }
        if ((gFishmodRodInfo.state == 7) && (sAnglerLastState != 7)) {
            sAnglerLanded++;
        }
        sAnglerFloatTimer = (gFishmodRodInfo.state == 4) ? sAnglerFloatTimer + 1 : 0;
        sAnglerLastState = gFishmodRodInfo.state;
        sAnglerLastBiting = gFishmodRodInfo.biting;
        Fishmod_DebugPrintf(11, "angler t%d casts %d bites %d landed %d", sAnglerTimer / 20, sAnglerCasts,
                            sAnglerBites, sAnglerLanded);
        switch (gFishmodRodInfo.state) {
            case 0: // rod away
                if ((sAnglerTimer % 40) == 0) {
                    button = BTN_CLEFT;
                }
                break;
            case 1: // held: cast every so often
                if ((sAnglerTimer % 60) == 0) {
                    button = BTN_CLEFT;
                }
                break;
            case 2: // wind-up: hold the rod button to about 37% power (the old 60% cast, before casts got longer)
                if (gFishmodRodInfo.castPower < 0.33f) {
                    button = BTN_CLEFT;
                }
                break;
            case 4: // float: strike on a real bite; nothing for 25 seconds, reel in and cast again
                if (gFishmodRodInfo.biting && ((sAnglerTimer % 4) == 0)) {
                    button = BTN_B;
                } else if (sAnglerFloatTimer > 500) {
                    button = BTN_A;
                }
                break;
            case 5: // on the ground: reel back
                button = BTN_A;
                break;
            case 6: // fight: reel while the tension is comfortable
                if (gFishmodRodInfo.tension < 60.0f) {
                    button = BTN_A;
                }
                break;
            case 7: // landed: A through the catch textbox (its cursor starts on keeping it)
                if ((gFishmodRodInfo.choice != 0) && ((sAnglerTimer % 8) == 0)) {
                    button = BTN_A;
                }
                break;
        }
        input->prev = input->cur;
        input->cur.button = button;
        input->cur.stick_x = 0;
        input->cur.stick_y = stickY;
        buttonDiff = input->prev.button ^ input->cur.button;
        input->press.button = input->cur.button & buttonDiff;
        input->rel.button = input->prev.button & buttonDiff;
        PadUtils_UpdateRelXY(input);
        input->press.stick_x = 0;
        input->press.stick_y = 0;
        return;
    }
#endif

    if (!sScriptActive) {
        return;
    }

    if (sScriptStepIndex >= (s32)ARRAY_COUNT(sScriptSteps)) {
        sScriptActive = false;
        return;
    }

    step = &sScriptSteps[sScriptStepIndex];
    if (step->untilPause != 0) {
        if ((gameState->main == Play_Main) && (((PlayState*)gameState)->pauseCtx.state == step->untilPause - 1)) {
            sScriptStepFrame = 0;
            sScriptStepIndex++;
            return;
        }
        input->prev = input->cur;
        input->cur.button = ((sScriptStepFrame++ % step->frames) == 0) ? step->button : 0;
        buttonDiff = input->prev.button ^ input->cur.button;
        input->press.button = input->cur.button & buttonDiff;
        input->rel.button = input->prev.button & buttonDiff;
        return;
    }
#ifdef FISHMOD_SCRIPT_NORMAL_BOOT
    {
        // what the last scripted button press met: which step, and the pause menu's state then
        static u16 sLastButton;
        static s32 sLastStep;
        static s32 sLastPause;

        if (step->button != 0) {
            sLastButton = step->button;
            sLastStep = sScriptStepIndex;
            sLastPause = (gameState->main == Play_Main) ? ((PlayState*)gameState)->pauseCtx.state * 100 +
                                                              ((PlayState*)gameState)->pauseCtx.mainState
                                                        : -1;
        }
        Fishmod_DebugPrintf(3, "step %d/%d btn %04X@%d pause %d now %d", sScriptStepIndex,
                            (s32)ARRAY_COUNT(sScriptSteps), sLastButton, sLastStep, sLastPause,
                            (gameState->main == Play_Main) ? ((PlayState*)gameState)->pauseCtx.state * 100 +
                                                                 ((PlayState*)gameState)->pauseCtx.mainState
                                                           : -1);
    }
#endif

    input->prev = input->cur;
    input->cur.button = step->button;
    input->cur.stick_x = step->stickX;
    input->cur.stick_y = step->stickY;
    buttonDiff = input->prev.button ^ input->cur.button;
    input->press.button = input->cur.button & buttonDiff;
    input->rel.button = input->prev.button & buttonDiff;
    PadUtils_UpdateRelXY(input);
    input->press.stick_x = input->cur.stick_x - input->prev.stick_x;
    input->press.stick_y = input->cur.stick_y - input->prev.stick_y;

    if (step->inPlay) {
        // only frames where Play ran since the last call count
        static s32 sLastPlayFrames;

        if (sPlayFrames == sLastPlayFrames) {
            return;
        }
        sLastPlayFrames = sPlayFrames;
    }
    if (++sScriptStepFrame >= step->frames) {
        sScriptStepFrame = 0;
        sScriptStepIndex++;
    }
}


#ifdef FISHMOD_HWBENCH
/**
 * The watchdog: a thread of its own that wakes every 2 seconds and checks the game's frames are still going. If none
 * has passed for 10 seconds (a hang, an endless loop, the graphics waiting forever) it writes to SRAM 0x7B00 ('FSTL')
 * where the graphics thread is (its pc, ra, sp and the top of its stack) and what the bench was doing, once. A crash
 * is caught by the fault handler instead (Fishmod_DebugFault), but this also catches what never faults.
 */
extern OSThread sGraphThread;

static OSThread sWatchdogThread;
static u64 sWatchdogStack[0x400 / sizeof(u64)];
static OSMesgQueue sWatchdogQueue;
static OSMesg sWatchdogMsg;
static OSTimer sWatchdogTimer;
static volatile u32 sWatchdogBeat; // play frames, counted by Fishmod_DebugPlayUpdate

static void Watchdog_Entry(void* arg) {
    static struct {
        u32 magic; // 'FSTL'
        u32 beat;  // play frames when it stopped
        u32 pc;
        u32 ra;
        u32 sp;
        u32 sr;
        u32 bench[6]; // play bench: lap, leg, block, state, frame, play frames
        u32 stack[48];
    } sRec;
    u32 last = 0;
    s32 still = 0;
    s32 done = false;
    OSMesg msg;
    s32 i;

    while (true) {
        osRecvMesg(&sWatchdogQueue, &msg, OS_MESG_BLOCK);
        if (done) {
            continue;
        }
        if (sWatchdogBeat != last) {
            last = sWatchdogBeat;
            still = 0;
            continue;
        }
        if (++still < 5) {
            continue;
        }
        sRec.magic = 0x4653544C;
        sRec.beat = last;
        sRec.pc = sGraphThread.context.pc;
        sRec.ra = (u32)sGraphThread.context.ra;
        sRec.sp = (u32)sGraphThread.context.sp;
        sRec.sr = sGraphThread.context.sr;
#ifdef FISHMOD_BENCH_PLAY
        sRec.bench[0] = sBench.laps;
        sRec.bench[1] = sPbLeg;
        sRec.bench[2] = sPbBlock;
        sRec.bench[3] = sPbState;
        sRec.bench[4] = sPbFrame;
#elif defined(FISHMOD_BENCH_AREA)
        sRec.bench[0] = sArea.laps;
        sRec.bench[1] = sAreaSpot;
        sRec.bench[2] = sAreaVariant;
        sRec.bench[4] = sAreaFrame;
#endif
        sRec.bench[5] = last;
        for (i = 0; i < ARRAY_COUNT(sRec.stack); i++) {
            u32 addr = sRec.sp + i * 4;

            // the graphics thread's stack (it's in RDRAM; anything else is left out)
            sRec.stack[i] = ((addr >= 0x80000000) && (addr < 0x80800000)) ? *(u32*)addr : 0;
        }
        SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7B00, &sRec, sizeof(sRec), OS_WRITE);
        done = true;
    }
}

static void Watchdog_Start(void) {
    static s32 sStarted;

    if (sStarted) {
        return;
    }
    sStarted = true;
    osCreateMesgQueue(&sWatchdogQueue, &sWatchdogMsg, 1);
    osCreateThread(&sWatchdogThread, 99, Watchdog_Entry, NULL, (u8*)sWatchdogStack + sizeof(sWatchdogStack), 120);
    osStartThread(&sWatchdogThread);
    osSetTimer(&sWatchdogTimer, OS_USEC_TO_CYCLES(2000000), OS_USEC_TO_CYCLES(2000000), &sWatchdogQueue, NULL);
}
#endif

/**
 * A crash or a hang, written to SRAM 0x7C00 for hwbench_report.py: the thread and where it stopped, what the bench
 * was doing, and a hang's message. The console shows the crash screen only after a button combination, so without
 * this a crash on the hardware is just a frozen picture.
 */
void Fishmod_DebugFault(OSThread* thread, const char* exp1, const char* exp2) {
    static struct {
        u32 magic; // 'FFLT'
        u32 kind;  // 1: crash, 2: hang
        u32 thread;
        u32 pc;
        u32 cause;
        u32 badvaddr;
        u32 ra;
        u32 sp;
        u32 bench[6]; // play bench: lap, leg, block, state, frame, play frames
        char text[64]; // a hang's two messages
    } sRec;
    s32 i = 0;

    sRec.magic = 0x46464C54;
    sRec.kind = (thread != NULL) ? 1 : 2;
    sRec.thread = (thread != NULL) ? thread->id : osGetThreadId(NULL);
    if (thread != NULL) {
        sRec.pc = thread->context.pc;
        sRec.cause = thread->context.cause;
        sRec.badvaddr = thread->context.badvaddr;
        sRec.ra = (u32)thread->context.ra;
        sRec.sp = (u32)thread->context.sp;
    }
#ifdef FISHMOD_BENCH_PLAY
    sRec.bench[0] = sBench.laps;
    sRec.bench[1] = sPbLeg;
    sRec.bench[2] = sPbBlock;
    sRec.bench[3] = sPbState;
    sRec.bench[4] = sPbFrame;
#elif defined(FISHMOD_BENCH_AREA)
    sRec.bench[0] = sArea.laps;
    sRec.bench[1] = sAreaSpot;
    sRec.bench[2] = sAreaVariant;
    sRec.bench[4] = sAreaFrame;
#endif
    sRec.bench[5] = sPlayFrames;
    {
        const char* parts[2];
        s32 p;

        parts[0] = exp1;
        parts[1] = exp2;
        for (p = 0; p < 2; p++) {
            const char* c = parts[p];

            while ((c != NULL) && (*c != '\0') && (i < (s32)sizeof(sRec.text) - 2)) {
                sRec.text[i++] = *c++;
            }
            sRec.text[i++] = '|';
        }
        sRec.text[MIN(i, (s32)sizeof(sRec.text) - 1)] = '\0';
    }
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7C00, &sRec, sizeof(sRec), OS_WRITE);
}

void Fishmod_DebugPrintf(s32 line, const char* fmt, ...) {
    va_list args;
    char buf[128];

    if ((line < 0) || (line >= FISHMOD_DEBUG_LINES)) {
        return;
    }

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    buf[FISHMOD_DEBUG_LINE_LEN - 1] = '\0';
    bcopy(buf, sDebugLines[line], FISHMOD_DEBUG_LINE_LEN);
}

void Fishmod_DebugDrawText(GameState* gameState, Gfx** gfxP) {
    GfxPrint printer;
    s32 i;

    if (gFishmodPhoto.active) {
        return;
    }
#ifdef FISHMOD_PLAYLOG
    return; // played as the test build: the frame counter only
#endif
#ifdef FISHMOD_SCRIPT_NOTEXT
    return; // clean screenshots (the pause-menu mockups)
#endif
#if defined(FISHMOD_BENCH_PLAY) || defined(FISHMOD_BENCH_AREA)
    if (!sTourDebugText) {
        return; // the bench as played: nothing over the frames being timed until it's done
    }
#endif
#ifdef FISHMOD_TOUR
    if (sTourCamId != SUB_CAM_ID_DONE) {
        Fishmod_TourDraw(gfxP);
        if (!sTourDebugText) {
            return;
        }
    }
#endif

    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, *gfxP);
    GfxPrint_SetColor(&printer, 255, 255, 100, 255);

    for (i = 0; i < FISHMOD_DEBUG_LINES; i++) {
        if (sDebugLines[i][0] != '\0') {
            GfxPrint_SetPos(&printer, 2, 18 + i);
            GfxPrint_Printf(&printer, "%s", sDebugLines[i]);
        }
    }

    *gfxP = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);
}

#endif

#if FISHMOD_DEBUG
/**
 * How many of the static collision lookup's nodes a scene used, of how many it had (BgCheck_Allocate), written to
 * SRAM 0x7D00 ('FMCO', then scene, used, max) as each scene loads: the extra rock collision must leave room, since
 * running out has no check outside debug builds.
 */
void Fishmod_DebugColNodes(s32 sceneId, s32 used, s32 max) {
    static u32 sRec[4];

    sRec[0] = 0x464D434F;
    sRec[1] = sceneId;
    sRec[2] = used;
    sRec[3] = max;
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7D00, sRec, sizeof(sRec), OS_WRITE);
}
#endif
