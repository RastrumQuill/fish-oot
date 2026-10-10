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
#include "player.h"
#include "save.h"
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

void Sram_InitDebugSave(void);

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

OSTime gFishmodCpuTime[FISHMOD_CPU_MAX];

/**
 * Averages over 60 frames, in tenths of a millisecond: RSP graphics and RDP time, the CPU time of Play_Update,
 * Play_Draw and the fishmod actors, and the whole frame.
 */
static void Fishmod_DebugTiming(void) {
    static u32 sSum[2 + FISHMOD_CPU_MAX + 1];
    static u32 sShown[2 + FISHMOD_CPU_MAX + 1];
    static s32 sCount;
    s32 i;

    sSum[0] += (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
    sSum[1] += (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
    for (i = 0; i < FISHMOD_CPU_MAX; i++) {
        sSum[2 + i] += (u32)OS_CYCLES_TO_USEC(gFishmodCpuTime[i]);
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
#endif
    }

#ifdef FISHMOD_VIEW
    if (sPlacedPlayer && (sPlayFrames > 40)) {
        Fishmod_ViewUpdate(play);
    }
#endif
#ifdef FISHMOD_PHOTO
    if (sPlacedPlayer && (sPlayFrames > 60)) {
        Fishmod_PhotoUpdate(play);
    }
#endif

    Fishmod_DebugPrintf(0, "F%d S%d/%d caught %d lost %d", sPlayFrames, sScriptStepIndex,
                        (s32)ARRAY_COUNT(sScriptSteps), gFishmodRodInfo.caught, gFishmodRodInfo.lost);
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
            case 2: // wind-up: hold the rod button until half power
                if (gFishmodRodInfo.castPower < 0.6f) {
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
            case 7: // landed: keep it if there's an empty bottle, else release
                if ((gFishmodRodInfo.choice != 0) && ((sAnglerTimer % 8) == 0)) {
                    button = (gFishmodRodInfo.choice == 1) ? BTN_A : BTN_B;
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

    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, *gfxP);
    GfxPrint_SetColor(&printer, 255, 255, 100, 255);

    for (i = 0; i < FISHMOD_DEBUG_LINES; i++) {
        if (sDebugLines[i][0] != '\0') {
            GfxPrint_SetPos(&printer, 2, 20 + i);
            GfxPrint_Printf(&printer, "%s", sDebugLines[i]);
        }
    }

    *gfxP = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);
}

#endif
