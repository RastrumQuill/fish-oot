/*
 * File: fishmod_debug.c
 * Description: fishmod debug harness. Only compiled into FISHMOD_DEBUG builds.
 *  - Boots straight into a test scene with a debug save and the fishing rod on C-Left
 *  - Optionally plays a scripted input sequence (FISHMOD_SCRIPT), then hands control back to the controller
 *  - Draws debug text lines that any code can set with Fishmod_DebugPrintf
 */

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

void Sram_InitDebugSave(void);

// A stop on a camera tour (FISHMOD_TOUR in a script): where the camera is, what it looks at, and a caption
typedef struct FishmodTourStop {
    Vec3f eye;
    Vec3f at;
    const char* title;
    const char* lines[3];
} FishmodTourStop;

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
static s32 sTourDebugText;

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
        Fishmod_TourGo(0);
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
    sTourYaw += sTourStickX * 0.0006f;
    sTourRise = CLAMP(sTourRise + sTourStickY * 0.08f, -150.0f, 400.0f);

    // glide (eased), arcing up over long hops so the camera doesn't cut through the land
    stop = &sTourStops[sTourStop];
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
}

static void Fishmod_TourDraw(Gfx** gfxP) {
    FishmodTourStop* stop = &sTourStops[sTourStop];
    GfxPrint printer;
    Gfx* gfx = *gfxP;
    s32 i;

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
    GfxPrint_SetPos(&printer, 21, 27);
    GfxPrint_Printf(&printer, "A>  B<  stick:look");
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

void Fishmod_DebugBoot(void) {
    gSaveContext.save.linkAge = LINK_AGE_CHILD;
    gSaveContext.fileNum = 0xFF;
    Sram_InitDebugSave();

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

    // Rod on C-Left, two empty bottles on C-Down / C-Right slots.
    gSaveContext.save.info.inventory.items[SLOT_BOTTLE_1] = ITEM_BOTTLE_EMPTY;
    gSaveContext.save.info.inventory.items[SLOT_BOTTLE_2] = ITEM_BOTTLE_EMPTY;
    gSaveContext.save.info.equips.buttonItems[1] = ITEM_FISHING_POLE;
    gSaveContext.save.info.equips.cButtonSlots[0] = SLOT_NONE;
    gSaveContext.save.info.equips.buttonItems[2] = ITEM_BOTTLE_EMPTY;
    gSaveContext.save.info.equips.cButtonSlots[1] = SLOT_BOTTLE_1;
    gSaveContext.save.info.equips.buttonItems[3] = ITEM_BOTTLE_EMPTY;
    gSaveContext.save.info.equips.cButtonSlots[2] = SLOT_BOTTLE_2;

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
#endif
    gSaveContext.respawnFlag = 0;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = ENTR_LOAD_OPENING;
    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
    gSaveContext.natureAmbienceId = 0xFF;
    gSaveContext.showTitleCard = false;
    gSaveContext.gameMode = GAMEMODE_NORMAL;

    sScriptStepIndex = 0;
    sScriptStepFrame = 0;
    sScriptActive = false;
    sPlayFrames = 0;
    sPlacedPlayer = false;
}

/** True if the script spawns its own fish, so the fish object is needed even in scenes without fishing. */
s32 Fishmod_DebugWantsFish(void) {
#if defined(FISHMOD_SCRIPT_FISH_TANK) || defined(FISHMOD_SCRIPT_EXTRA_FISH)
    return true;
#else
    return false;
#endif
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
OSTime gFishmodProf[FISHMOD_PROF_MAX];
u32 gFishmodProfCount[FISHMOD_PROF_MAX];

/**
 * Averages over 60 frames, in tenths of a millisecond: RSP graphics and RDP time, the CPU time of Play_Update,
 * Play_Draw and the fishmod actors, and the whole frame.
 */
#ifndef FISHMOD_STATS_SKIP
#define FISHMOD_STATS_SKIP 600 // frames before the whole-run stats start (scene load, settling)
#endif
static u32 sLastCpu[FISHMOD_CPU_MAX];
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
            SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7E00, &sProf, sizeof(sProf), OS_WRITE);
        }
    }
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
                SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + 0x7F00, &sStats, sizeof(sStats), OS_WRITE);
            }
        }
    }
    Fishmod_DebugProfile(sStatsFrames > FISHMOD_STATS_SKIP);
}

s32 Fishmod_DebugRosterCount(void) {
#ifdef FISHMOD_SCRIPT_ROSTER
    return FISHMOD_SCRIPT_ROSTER;
#else
    return 0;
#endif
}

void Fishmod_DebugPlayUpdate(PlayState* play) {
    Player* player = GET_PLAYER(play);

    sPlayFrames++;
    Fishmod_DebugTiming();

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
#ifdef FISHMOD_SCRIPT_EXTRA_FISH
        {
            // a crowd of fish around Link for stress tests: the Kokiri species in turn, in a ring
            static u8 sKinds[] = { FISHMOD_SPECIES_GREENGILL, FISHMOD_SPECIES_HYRULE_BASS, FISHMOD_SPECIES_SANKE_CARP,
                                   FISHMOD_SPECIES_ORDON_CATFISH, FISHMOD_SPECIES_HYRULE_MINNOW,
                                   FISHMOD_SPECIES_HYLIAN_LOACH };
            s32 n;

            for (n = 0; n < FISHMOD_SCRIPT_EXTRA_FISH; n++) {
                s16 yaw = n * (0x10000 / FISHMOD_SCRIPT_EXTRA_FISH);
                f32 r = 40.0f + (n % 4) * 30.0f;

                Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, sScriptStartPos.x + Math_SinS(yaw) * r,
                            sScriptStartPos.y + 30.0f + (n % 3) * 15.0f, sScriptStartPos.z + Math_CosS(yaw) * r, 0,
                            yaw, 0, FISHMOD_PARAMS(FISHMOD_TYPE_FISH, sKinds[n % ARRAY_COUNT(sKinds)]));
            }
        }
#endif
#ifdef FISHMOD_SCRIPT_FISH_TANK
        {
            // fill the scene's first water box with fish, spread on a grid across it at all depths (stress tests)
            static u8 sKinds[] = { FISHMOD_SPECIES_GREENGILL, FISHMOD_SPECIES_HYRULE_BASS, FISHMOD_SPECIES_SANKE_CARP,
                                   FISHMOD_SPECIES_ORDON_CATFISH, FISHMOD_SPECIES_HYRULE_MINNOW,
                                   FISHMOD_SPECIES_HYLIAN_LOACH };
            WaterBox* box = SEGMENTED_TO_VIRTUAL(play->colCtx.colHeader->waterBoxes);
            s32 cols = 6;
            s32 rows = (FISHMOD_SCRIPT_FISH_TANK + cols - 1) / cols;
            s32 n;

            for (n = 0; n < FISHMOD_SCRIPT_FISH_TANK; n++) {
                f32 x = box->xMin + box->xLength * ((n % cols) + 0.5f) / cols;
                f32 z = box->zMin + box->zLength * ((n / cols) + 0.5f) / rows;
                f32 y = box->ySurface - 20.0f - (n * 37 % 300);

                Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, x, y, z, 0, (s16)(n * 0x2800), 0,
                            FISHMOD_PARAMS(FISHMOD_TYPE_FISH, sKinds[n % ARRAY_COUNT(sKinds)]));
            }
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
#ifdef FISHMOD_TOUR
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
        u16 button = 0;
        s8 stickY = 0;

        sAnglerTimer++;
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
            case 4: // float: strike on a real bite
                if (gFishmodRodInfo.biting && ((sAnglerTimer % 4) == 0)) {
                    button = BTN_B;
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
            GfxPrint_SetPos(&printer, 2, 19 + i);
            GfxPrint_Printf(&printer, "%s", sDebugLines[i]);
        }
    }

    *gfxP = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);
}

#endif
