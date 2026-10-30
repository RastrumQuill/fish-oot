/*
 * File: fishmod_debug.c
 * Description: fishmod debug harness. Only compiled into FISHMOD_DEBUG builds.
 *  - Boots straight into a test scene with a debug save and the fishing rod on C-Left
 *  - Optionally plays a scripted input sequence (FISHMOD_SCRIPT), then hands control back to the controller
 *  - Draws debug text lines that any code can set with Fishmod_DebugPrintf
 */

#include "dma.h"
#include "fishmod.h"
#include "fishmod_sram.h"
#include "src/fishmod_data/fishmod_profile.h"

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

#include "fishmod_debug_photo.inc.c" // the photo booth for the wiki renders (FISHMOD_PHOTO)

#include "fishmod_debug_studio.inc.c" // the animation studio (FISHMOD_ANIM_STUDIO)

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

#include "fishmod_debug_lod.inc.c" // the LOD jobs: calibration, sweep, tuning, rotation check (FISHMOD_LOD_*)


#ifdef FISHMOD_HWBENCH
static u32 sLastCpu[FISHMOD_CPU_MAX];
#endif
#include "fishmod_debug_bench.inc.c" // the hardware benches: the play bench and the area bench (FISHMOD_HWBENCH)

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
    // the slingshot on C-Down (switching to it puts the rod away), bugs in the C-Right bottle
    gSaveContext.save.info.inventory.items[SLOT_SLINGSHOT] = ITEM_SLINGSHOT;
    gSaveContext.save.info.equips.buttonItems[2] = ITEM_SLINGSHOT;
    gSaveContext.save.info.equips.cButtonSlots[1] = SLOT_SLINGSHOT;
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
#ifdef FISHMOD_SCRIPT_OLD_SAVE
    // a file from an old build: the save block at the old place, without the mark; loading it moves it
    {
        FishmodSaveData* old = (FishmodSaveData*)&gSaveContext.save.info.sceneFlags[FISHMOD_SAVE_OLD_SCENE];

        bcopy(FISHMOD_SAVE, old, sizeof(FishmodSaveData));
        bzero(FISHMOD_SAVE, sizeof(FishmodSaveData));
        old->magic = 0;
        old->version = 0;
        Fishmod_InitSaveExtras();
    }
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
            SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_PROFILE), &sProf, sizeof(sProf), OS_WRITE);
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
                SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_STATS), &sStats, sizeof(sStats), OS_WRITE);
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

#include "fishmod_debug_playlog.inc.c" // the play log (FISHMOD_PLAYLOG)

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
    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_HASHES), &sHashes, sizeof(sHashes), OS_WRITE);
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

#include "fishmod_debug_angler.inc.c" // the auto-angler (FISHMOD_AUTO_ANGLER)

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
        SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_STALL), &sRec, sizeof(sRec), OS_WRITE);
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
    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_FAULT), &sRec, sizeof(sRec), OS_WRITE);
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
    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_COLLISION), sRec, sizeof(sRec), OS_WRITE);
}
#endif
