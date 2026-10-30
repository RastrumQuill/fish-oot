/*
 * File: fishmod_rod.c
 * Overlay: ovl_En_Fishmod
 * Description: the fishing controller (EnFishmod with type FISHMOD_TYPE_ROD, one per fishing scene).
 *
 * Twilight Princess-style bobber fishing with a Sega Bass Fishing-style fight:
 *  - press the rod's C button to cast; during the wind-up the stick sets power (up = far) and aim
 *  - the bobber floats; small stick wiggles twitch it, which draws fish in; A reels in, R+A reels fast
 *  - fish nibble (small dips) before the real bite (bobber pulled deep); press B during the real bite to
 *    set the hook. Too early spooks the fish, too late and it steals the bait
 *  - while fighting, keep the line tension in the middle: too high and the line snaps, too slack and the
 *    hook falls out. Hold the stick against the fish's run to tire it faster
 *  - reel a tired fish in to land it
 * It also draws the rod, line, bobber and hook, runs the fishing camera and spawns the fish.
 */

#include "z_en_fishmod.h"

#include "assets/objects/object_fish/object_fish.h"
#include "libc64/math64.h"
#include "libc64/sprintf.h"
#include "libu64/gfxprint.h"
#include "array_count.h"
#include "camera.h"
#include "controller.h"
#include "sys_math.h"
#include "effect.h"
#include "gfx.h"
#include "gfx_setupdl.h"
#include "rand.h"
#include "rumble.h"
#include "segmented_address.h"
#include "sfx.h"
#include "sys_matrix.h"
#include "z_lib.h"
#include "play_state.h"
#include "player.h"
#include "interface.h"
#include "item.h"
#include "save.h"
#include "actor_profile.h"
#include "ultra64/libc.h"
#include "audio.h"
#include "message.h"
#include "sequence.h"
#include "regs.h"

FishmodRod gFishmodRodState;

#define ROD (&gFishmodRodState)

#define WINDUP_FRAMES 18
#define LINE_MIN 28.0f
#define BAIT_DEPTH_START 6.0f   // the bait as it lands, just under the bobber
#define BAIT_ABOVE_BOTTOM 6.0f  // where the sink stops by itself
// The most line under a bobber (about the rod's length, some 5 m): a float rig fishes the top of the water, not the
// deep basin or the Water Temple's gate (that's for sinkers and lures)
#define BAIT_DEPTH_MAX 180.0f
#define LINE_MAX_BASE 460.0f
// Strength upgrades (Goron's Bracelet, Silver and Golden Gauntlets) let Link cast further and play out more line
#define STRENGTH_CAST_SCALE() (1.0f + 0.2f * CUR_UPG_VALUE(UPG_STRENGTH))
#define LINE_MAX (LINE_MAX_BASE * STRENGTH_CAST_SCALE())
#define LINE_SEGMENTS 16

static void Rod_UpdateLineFloor(PlayState* play);

#if FISHMOD_DEBUG
static s32 sLineDraped; // line points lifted out of the ground or water this frame
#endif

/* ---------------------------------------------------------------------------------------------------------------
 * Bobber model: a small red-over-white float built from vertex colors (no texture).
 */

#define BOB_R 100
#define BOB_H 120
static Vtx sBobberVtx[] = {
    // top cap (red)
    VTX(0, BOB_H * 2, 0, 0, 0, 230, 30, 30, 255),
    VTX(BOB_R, 0, 0, 0, 0, 230, 30, 30, 255),
    VTX(71, 0, 71, 0, 0, 230, 30, 30, 255),
    VTX(0, 0, BOB_R, 0, 0, 230, 30, 30, 255),
    VTX(-71, 0, 71, 0, 0, 230, 30, 30, 255),
    VTX(-BOB_R, 0, 0, 0, 0, 230, 30, 30, 255),
    VTX(-71, 0, -71, 0, 0, 230, 30, 30, 255),
    VTX(0, 0, -BOB_R, 0, 0, 230, 30, 30, 255),
    VTX(71, 0, -71, 0, 0, 230, 30, 30, 255),
    // bottom cap (white)
    VTX(0, -BOB_H, 0, 0, 0, 245, 245, 235, 255),
    VTX(BOB_R, 0, 0, 0, 0, 245, 245, 235, 255),
    VTX(71, 0, 71, 0, 0, 245, 245, 235, 255),
    VTX(0, 0, BOB_R, 0, 0, 245, 245, 235, 255),
    VTX(-71, 0, 71, 0, 0, 245, 245, 235, 255),
    VTX(-BOB_R, 0, 0, 0, 0, 245, 245, 235, 255),
    VTX(-71, 0, -71, 0, 0, 245, 245, 235, 255),
    VTX(0, 0, -BOB_R, 0, 0, 245, 245, 235, 255),
    VTX(71, 0, -71, 0, 0, 245, 245, 235, 255),
};

static Gfx sBobberDL[] = {
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPClearGeometryMode(G_LIGHTING | G_CULL_BACK),
    gsSPVertex(sBobberVtx, 18, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(0, 3, 4, 0, 0, 4, 5, 0),
    gsSP2Triangles(0, 5, 6, 0, 0, 6, 7, 0),
    gsSP2Triangles(0, 7, 8, 0, 0, 8, 1, 0),
    gsSP2Triangles(9, 11, 10, 0, 9, 12, 11, 0),
    gsSP2Triangles(9, 13, 12, 0, 9, 14, 13, 0),
    gsSP2Triangles(9, 15, 14, 0, 9, 16, 15, 0),
    gsSP2Triangles(9, 17, 16, 0, 9, 10, 17, 0),
    gsSPSetGeometryMode(G_LIGHTING),
    gsSPEndDisplayList(),
};

/* ---------------------------------------------------------------------------------------------------------------
 * Helpers
 */

void EnFishmod_RodMessage(s32 color, const char* line1, const char* line2) {
    bzero(ROD->message, sizeof(ROD->message));
    if (line1 != NULL) {
        bcopy(line1, ROD->message[0], CLAMP_MAX(strlen(line1), 39));
    }
    if (line2 != NULL) {
        bcopy(line2, ROD->message[1], CLAMP_MAX(strlen(line2), 39));
    }
    ROD->messageColor = color;
    ROD->messageTimer = 90;
}

s32 EnFishmod_RodBobberInWater(void) {
    return (ROD->state == ROD_STATE_FLOAT);
}

void EnFishmod_RodNibble(EnFishmod* fish) {
    if (ROD->state != ROD_STATE_FLOAT) {
        return;
    }
    ROD->fish = fish;
    ROD->dip = 4.0f; // quick tick down
    ROD->dipTarget = 0.0f;
    Sfx_PlaySfxAtPos(&gFishmodRod->projectedPos, NA_SE_EV_LURE_MOVE_W);
}

void EnFishmod_RodBite(EnFishmod* fish) {
    if (ROD->state != ROD_STATE_FLOAT) {
        return;
    }
    ROD->fish = fish;
    ROD->biting = true;
    ROD->dipTarget = 14.0f;
    ROD->biteTimer = fish->species->behavior.biteWindow + 4; // + the frames it takes to pull the bobber down
    Sfx_PlaySfxAtPos(&gFishmodRod->projectedPos, NA_SE_EV_DIVE_WATER);
    Rumble_Override(0.0f, 100, 6, 20);
}

void EnFishmod_RodRelease(EnFishmod* fish) {
    if (ROD->fish == fish) {
        ROD->fish = NULL;
        ROD->biting = false;
        ROD->dipTarget = 0.0f;
    }
}

static f32 Rod_GroundY(PlayState* play, Vec3f* pos) {
    CollisionPoly* poly;
    Vec3f checkPos = *pos;

    checkPos.y += 40.0f;
    return BgCheck_EntityRaycastDown1(&play->colCtx, &poly, &checkPos);
}

static s32 Rod_WaterY(PlayState* play, Vec3f* pos, f32* waterY) {
    WaterBox* waterBox;

    return BgCheck_GetWaterSurface(play, &play->colCtx, pos->x, pos->z, waterY, &waterBox);
}

static void Rod_Splash(PlayState* play, Vec3f* pos, s32 big) {
    Vec3f splashPos = *pos;

    EffectSsGRipple_Spawn(play, &splashPos, big ? 100 : 60, big ? 500 : 300, 0);
    if (big) {
        EffectSsGSplash_Spawn(play, &splashPos, NULL, NULL, 0, 300);
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Camera: TP-style view from behind Link that looks at the bobber (or the fish while fighting).
 * Z cycles zoom levels.
 */

static void Rod_CameraStart(PlayState* play) {
    Camera* mainCam;

    if (ROD->camId != SUB_CAM_ID_DONE) {
        return;
    }
    ROD->camId = Play_CreateSubCamera(play);
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
    Play_ChangeCameraStatus(play, ROD->camId, CAM_STAT_ACTIVE);
    mainCam = Play_GetCamera(play, CAM_ID_MAIN);
    ROD->camEye = mainCam->eye;
    ROD->camAt = mainCam->at;
    ROD->camBlend = 0.0f;
}

static void Rod_CameraStop(PlayState* play) {
    Camera* mainCam;

    if (ROD->camId == SUB_CAM_ID_DONE) {
        return;
    }
    mainCam = Play_GetCamera(play, CAM_ID_MAIN);
    mainCam->eye = ROD->camEye;
    mainCam->eyeNext = ROD->camEye;
    mainCam->at = ROD->camAt;
    Play_ReturnToMainCam(play, ROD->camId, 0);
    ROD->camId = SUB_CAM_ID_DONE;
}

// what the catch textbox says, filled in when the fish is landed
static char sCatchSize[32];
static s32 sCatchRecord;

static void Rod_CameraUpdate(PlayState* play, Player* player, Vec3f* focus) {
    static f32 sZoomFactors[] = { 0.0f, 0.35f, 0.6f, 0.8f }; // the last one is half the distance of the one before
    Vec3f eyeTarget;
    Vec3f toFocus;
    f32 dist;
    f32 zoom;
    s16 yaw;

    if (ROD->camId == SUB_CAM_ID_DONE) {
        return;
    }

    toFocus.x = focus->x - player->actor.world.pos.x;
    toFocus.z = focus->z - player->actor.world.pos.z;
    dist = sqrtf(SQ(toFocus.x) + SQ(toFocus.z));
    yaw = Math_Atan2S(toFocus.z, toFocus.x);

    // over Link's shoulder, a bit behind and above; further back while fighting so Link and the fish both fit
    {
        f32 back = (ROD->state == ROD_STATE_FIGHT) ? 120.0f : 75.0f;
        f32 side = (ROD->state == ROD_STATE_FIGHT) ? 45.0f : 28.0f;
        f32 up = (ROD->state == ROD_STATE_FIGHT) ? 85.0f : 60.0f;

        eyeTarget.x = player->actor.world.pos.x - Math_SinS(yaw) * back + Math_CosS(yaw) * side;
        eyeTarget.y = player->actor.world.pos.y + up;
        eyeTarget.z = player->actor.world.pos.z - Math_CosS(yaw) * back - Math_SinS(yaw) * side;
    }

    if (ROD->state == ROD_STATE_LANDING) {
        // swing round to the front of Link to see the fish he's holding up, going round him, not through him
        s16 eyeYaw = Math_Vec3f_Yaw(&player->actor.world.pos, &ROD->camEye);
        f32 height = LINK_IS_CHILD ? 30.0f : 45.0f;
        Vec3f at;

        Math_SmoothStepToS(&eyeYaw, player->actor.shape.rot.y, 4, 0x900, 0x100);
        ROD->camEye.x = player->actor.world.pos.x + Math_SinS(eyeYaw) * 85.0f;
        ROD->camEye.z = player->actor.world.pos.z + Math_CosS(eyeYaw) * 85.0f;
        Math_ApproachF(&ROD->camEye.y, player->actor.world.pos.y + height + 10.0f, 0.2f, 10.0f);
        at = player->actor.world.pos;
        at.y += height;
        Math_ApproachF(&ROD->camAt.x, at.x, 0.25f, 20.0f);
        Math_ApproachF(&ROD->camAt.y, at.y, 0.25f, 20.0f);
        Math_ApproachF(&ROD->camAt.z, at.z, 0.25f, 20.0f);
        Play_SetCameraAtEye(play, ROD->camId, &ROD->camAt, &ROD->camEye);
        return;
    }

    // zooming moves the eye along the line toward the focus
    zoom = (ROD->state == ROD_STATE_FIGHT) ? 0.0f : sZoomFactors[ROD->zoom];
    if ((ROD->state == ROD_STATE_FLOAT) && ROD->biting) {
        zoom += 0.1f; // lean in on a bite
    }
    if (zoom > 0.0f) {
        // each zoom level frames the bobber from a fixed distance (not a fraction of the cast), so a long cast
        // doesn't leave it tiny and a short one still zooms
        static f32 sZoomDist[] = { 0.0f, 330.0f, 200.0f, 120.0f };
        Vec3f look;
        f32 far;
        f32 want = sZoomDist[ROD->zoom] * ((ROD->biting && (ROD->state == ROD_STATE_FLOAT)) ? 0.85f : 1.0f);

        look.x = focus->x - eyeTarget.x;
        look.y = focus->y + 25.0f - eyeTarget.y;
        look.z = focus->z - eyeTarget.z;
        far = sqrtf(SQ(look.x) + SQ(look.y) + SQ(look.z));
        if (far > want) {
            f32 t = (far - want) / far;

            eyeTarget.x += look.x * t;
            eyeTarget.y += look.y * t;
            eyeTarget.z += look.z * t;
        }
    }

    // never under the surface: look down into the water from above it (the bobber stays in view)
    if ((ROD->state == ROD_STATE_FLOAT) && (eyeTarget.y < ROD->waterY + 20.0f)) {
        eyeTarget.y = ROD->waterY + 20.0f;
    }

    Math_ApproachF(&ROD->camBlend, 1.0f, 1.0f, 0.04f);
    Math_ApproachF(&ROD->camAt.x, focus->x, 0.25f, 40.0f * ROD->camBlend);
    Math_ApproachF(&ROD->camAt.y, focus->y, 0.25f, 40.0f * ROD->camBlend);
    Math_ApproachF(&ROD->camAt.z, focus->z, 0.25f, 40.0f * ROD->camBlend);
    Math_ApproachF(&ROD->camEye.x, eyeTarget.x, 0.2f, 25.0f * ROD->camBlend);
    Math_ApproachF(&ROD->camEye.y, eyeTarget.y, 0.2f, 25.0f * ROD->camBlend);
    Math_ApproachF(&ROD->camEye.z, eyeTarget.z, 0.2f, 25.0f * ROD->camBlend);

    Play_SetCameraAtEye(play, ROD->camId, &ROD->camAt, &ROD->camEye);
}

/* ---------------------------------------------------------------------------------------------------------------
 * Init / update
 */

void EnFishmod_RodInit(EnFishmod* this, PlayState* play) {
    bzero(ROD, sizeof(*ROD));
    ROD->camId = SUB_CAM_ID_DONE;
    ROD->state = ROD_STATE_NONE;
    ROD->bait = FISHMOD_BAIT_NONE;
    ROD->baitDepth = BAIT_DEPTH_START;
    this->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    EnFishmod_WaterGridAcquire(play);

    // the fish manager spawns the scene's fish (its roster)
    EnFishmod_ManagerEnsure(play);
}

void EnFishmod_RodDestroy(EnFishmod* this, PlayState* play) {
    Rod_CameraStop(play);
    EnFishmod_WaterGridRelease(play);
}

/** The area's music comes back after the fish-on music (it only restores if the fish-on music is playing). */
static void Rod_StopFightMusic(void) {
    func_800F5B58();
}

static void Rod_EndCast(PlayState* play, Player* player) {
    Rod_StopFightMusic();
    ROD->state = ROD_STATE_HELD;
    ROD->biting = false;
    ROD->fish = NULL;
    ROD->dip = ROD->dipTarget = 0.0f;
    ROD->tension = 0.0f;
    player->unk_860 = 0; // Link goes back to standing with the rod
    Rod_CameraStop(play);
}

static void Rod_StartFight(PlayState* play, Player* player) {
    EnFishmod* fish = ROD->fish;

    ROD->state = ROD_STATE_FIGHT;
    ROD->biting = false;
    ROD->tension = 35.0f;
    ROD->overTimer = ROD->slackTimer = 0;
    ROD->bait = FISHMOD_BAIT_NONE; // the fish took it
    EnFishmod_FishSetState(fish, FISH_STATE_HOOKED);
    // it ate (the bait), and it's terrified; its thrashing scares the fish round about
    fish->mood.hunger = 30;
    fish->mood.fear = 255;
    EnFishmod_Disturb(play, &fish->actor.world.pos, 220.0f, 120);
    Rumble_Override(0.0f, 180, 20, 10);
    Sfx_PlaySfxCentered(NA_SE_IT_SWORD_SWING_HARD);
    EnFishmod_RodMessage(1, "Hooked!", NULL);
    func_800F5ACC(NA_BGM_FISHMOD_FIGHT); // fish-on music in place of the area's, until the fight ends
}

static void Rod_LoseFish(PlayState* play, Player* player, const char* why) {
    EnFishmod* fish = ROD->fish;

    gFishmodRodInfo.lost++;
    Rod_StopFightMusic();
    if (fish != NULL) {
        EnFishmod_FishSetState(fish, FISH_STATE_FLEE);
    }
    ROD->fish = NULL;
    ROD->state = ROD_STATE_FLOAT;
    ROD->tension = 0.0f;
    ROD->dip = ROD->dipTarget = 0.0f;
    ROD->bait = FISHMOD_BAIT_NONE;
    // the bobber pops back up where the fish was
    if (fish != NULL) {
        ROD->bobberPos.x = fish->actor.world.pos.x;
        ROD->bobberPos.z = fish->actor.world.pos.z;
    }
    EnFishmod_RodMessage(2, why, NULL);
}

/** Line length is measured from the rod tip, horizontally plus vertically. */
static f32 Rod_DistFromTip(Vec3f* pos) {
    return Math_Vec3f_DistXYZ(&ROD->tipPos, pos);
}

/**
 * The fight. The fish tries to swim along runYaw; the line holds it at lineOut from the rod tip.
 * Tension rises when the fish pulls against a taut line and when reeling against it, and the reel's drag
 * pays out line above ~85. Holding the stick against the fish's run tires it faster and eases the tension.
 */
static void Rod_UpdateFight(EnFishmod* this, PlayState* play, Player* player, Input* input) {
    EnFishmod* fish = ROD->fish;
    const FishmodBehavior* b = &fish->species->behavior;
    Vec3f next;
    Vec3f toFish;
    f32 dist;
    f32 taut;
    f32 tensionTarget;
    f32 reel = 0.0f;
    f32 staminaFrac = fish->stamina / fish->staminaMax;
    f32 pullSpeed;
    s32 opposing = false;
    s16 stickYaw;
    f32 stickMag = sqrtf(SQ((f32)input->rel.stick_x) + SQ((f32)input->rel.stick_y));

    // reel
    if (CHECK_BTN_ALL(input->cur.button, BTN_A)) {
        reel = CHECK_BTN_ALL(input->cur.button, BTN_R) ? 2.2f : 1.2f;
        ROD->lineOut -= reel;
        Sfx_PlaySfxCentered(CHECK_BTN_ALL(input->cur.button, BTN_R) ? NA_SE_IT_FISHING_REEL_HIGH - SFX_FLAG
                                                                       : NA_SE_IT_FISHING_REEL_SLOW - SFX_FLAG);
    }

    // stick against the run: the stick is camera-relative
    if (stickMag > 40.0f) {
        Camera* cam = GET_ACTIVE_CAM(play);

        stickYaw = Math_Atan2S(input->rel.stick_y, -input->rel.stick_x) + Camera_GetInputDirYaw(cam) - 0x4000;
        opposing = ABS((s16)(stickYaw - fish->runYaw - 0x8000)) < 0x3000;
    }

    // where the fish wants to go
    // Bigger and stronger fish pull harder, a fresh fish harder than a tired one, and a run (stateTimer) much
    // harder: faster than the reel, so the line goes tight and the drag pays out.
    pullSpeed = (0.7f + b->strength * 0.02f + fish->lengthInches * 0.02f) * (0.35f + staminaFrac * 0.65f);
    if (fish->stateTimer > 0) {
        pullSpeed *= 2.2f;
    }
    if (fish->jumping) {
        pullSpeed *= 0.3f;
    }
    next.x = fish->actor.world.pos.x + Math_SinS(fish->runYaw) * pullSpeed;
    next.y = fish->actor.world.pos.y;
    next.z = fish->actor.world.pos.z + Math_CosS(fish->runYaw) * pullSpeed;

    dist = Rod_DistFromTip(&next);
    taut = dist - ROD->lineOut;

    tensionTarget = 0.0f;
    if (taut > 0.0f) {
        // the line holds the fish back
        toFish.x = next.x - ROD->tipPos.x;
        toFish.y = next.y - ROD->tipPos.y;
        toFish.z = next.z - ROD->tipPos.z;
        next.x = ROD->tipPos.x + toFish.x * (ROD->lineOut / dist);
        next.z = ROD->tipPos.z + toFish.z * (ROD->lineOut / dist);
        tensionTarget = 30.0f + taut * 8.0f + b->strength * 0.35f * (0.3f + staminaFrac * 0.7f);
        fish->stamina -= (0.4f + b->strength * 0.01f) * (opposing ? 2.5f : 1.0f);
        if (opposing) {
            tensionTarget -= 12.0f;
        }
    }
    if (reel > 0.0f) {
        tensionTarget += (taut > -6.0f) ? reel * 18.0f : 0.0f;
        if (fish->jumping) {
            tensionTarget += 30.0f; // reeling while it leaps: very likely to tear the hook out
        }
    }

    Math_ApproachF(&ROD->tension, tensionTarget, 0.3f, 6.0f);

    // reel drag pays out line instead of snapping right away
    if ((ROD->tension > 85.0f) && (taut > 0.0f)) {
        ROD->lineOut += pullSpeed * 0.8f; // the fish takes line, but against the drag
        Sfx_PlaySfxCentered(NA_SE_IT_FISHING_REEL_HIGH - SFX_FLAG);
    }

    ROD->lineOut = CLAMP(ROD->lineOut, LINE_MIN, LINE_MAX);

    fish->target = next;

    // snap
    if (ROD->tension >= 100.0f) {
        if (++ROD->overTimer > 12) {
            Rumble_Override(0.0f, 255, 30, 10);
            Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
            Rod_LoseFish(play, player, "The line snapped!");
            return;
        }
    } else {
        ROD->overTimer = 0;
    }

    // slack: the hook can fall out
    if (ROD->tension < 6.0f) {
        if (++ROD->slackTimer > 90) {
            Rod_LoseFish(play, player, "It shook the hook...");
            return;
        }
    } else {
        ROD->slackTimer = 0;
    }

    if ((play->gameplayFrames % 8) == 0) {
        Rumble_Override(0.0f, (u8)CLAMP(ROD->tension * 1.5f, 20.0f, 200.0f), 4, 4);
    }

    // landed
    if ((ROD->lineOut <= LINE_MIN + 2.0f) || fish->beached ||
        (Math_Vec3f_DistXZ(&player->actor.world.pos, &fish->actor.world.pos) < 32.0f)) {
        f32 weight = fish->species->weightCoef * fish->lengthInches * fish->lengthInches * fish->lengthInches *
                     0.000001f;

        ROD->state = ROD_STATE_LANDING;
        ROD->timer = 25; // Link holds it up before the textbox opens
        ROD->tension = 0.0f; // the rod relaxes once the fish is out of the water
        ROD->choice = 0;
        ROD->messageTimer = 0;
        gFishmodRodInfo.caught++;
        player->unk_860 = 3; // catch animation
        EnFishmod_FishSetState(fish, FISH_STATE_CAUGHT);
        Rumble_Override(0.0f, 120, 10, 5);
        Sfx_PlaySfxCentered(NA_SE_EV_JUMP_OUT_WATER);
        Rod_StopFightMusic();
        Audio_PlayFanfare(NA_BGM_ITEM_GET | 0x900);

        sCatchRecord = Fishmod_RecordCatch(fish->species - gFishmodSpecies, fish->lengthInches);
        if (weight < 1.0f) {
            sprintf(sCatchSize, "%d.%d in, %d oz", (s32)fish->lengthInches, (s32)(fish->lengthInches * 10.0f) % 10,
                    (s32)(weight * 16.0f + 0.5f));
        } else {
            sprintf(sCatchSize, "%d.%d in, %d.%d lbs", (s32)fish->lengthInches, (s32)(fish->lengthInches * 10.0f) % 10,
                    (s32)weight, (s32)(weight * 10.0f) % 10);
        }
    }
}

/**
 * The float depth, set on every cast: the bait sinks under the bobber until A is pressed once (that press only
 * stops it; A reels as usual once it's let go), or it stops by itself a little above the bottom, so shallow water
 * isn't a race. The camera follows it down.
 */
static void Rod_UpdateSink(PlayState* play, Input* input) {
    f32 surface;
    f32 floor;
    f32 deepest;

    if (ROD->reelLock && !CHECK_BTN_ALL(input->cur.button, BTN_A)) {
        ROD->reelLock = false;
    }
    if (!ROD->sinking) {
        return;
    }
    if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
        ROD->sinking = false;
        ROD->reelLock = true;
        Sfx_PlaySfxCentered(NA_SE_IT_FISHING_REEL_SLOW);
        ROD->dip = 3.0f; // the bobber stands up as the line comes tight
        return;
    }
    deepest = BAIT_DEPTH_MAX;
    if (EnFishmod_WaterFloor(play, ROD->bobberPos.x, ROD->bobberPos.z, &surface, &floor, false)) {
        deepest = CLAMP_MAX((surface - floor) - BAIT_ABOVE_BOTTOM, BAIT_DEPTH_MAX);
    }
    // sinking faster as the line pays out, like a weighted bait
    ROD->baitDepth += CLAMP(0.8f + ROD->baitDepth * 0.012f, 0.8f, 3.0f);
    if (ROD->baitDepth >= deepest) {
        ROD->baitDepth = CLAMP_MIN(deepest, BAIT_DEPTH_START);
        ROD->sinking = false;
        ROD->dip = 3.0f;
        Rumble_Override(0.0f, 60, 3, 10); // a small tug as the line comes tight near the bottom
    }
}

static void Rod_UpdateFloat(EnFishmod* this, PlayState* play, Player* player, Input* input) {
    f32 waterY;
    f32 groundY;
    f32 dist;
    Vec3f toTip;

    Rod_UpdateSink(play, input);

    // reel
    if (CHECK_BTN_ALL(input->cur.button, BTN_A) && !ROD->sinking && !ROD->reelLock) {
        f32 reel = CHECK_BTN_ALL(input->cur.button, BTN_R) ? 3.5f : 1.6f;

        ROD->lineOut -= reel;
        Sfx_PlaySfxCentered(CHECK_BTN_ALL(input->cur.button, BTN_R) ? NA_SE_IT_FISHING_REEL_HIGH - SFX_FLAG
                                                                       : NA_SE_IT_FISHING_REEL_SLOW - SFX_FLAG);
        if ((play->gameplayFrames % 6) == 0) {
            Vec3f ripple = ROD->bobberPos;

            ripple.y = ROD->waterY;
            EffectSsGRipple_Spawn(play, &ripple, 30, 150, 0);
        }
    }

    // twitch: a quick stick flick hops the bobber and draws fish in
    if (ROD->twitchTimer > 0) {
        ROD->twitchTimer--;
    }
    if ((ROD->twitchTimer == 0) && ((ABS(input->press.stick_x) > 35) || (ABS(input->press.stick_y) > 35)) &&
        !ROD->biting) {
        ROD->twitchTimer = 20;
        ROD->dip = 2.0f;
        ROD->lineOut -= 3.0f;
        Actor_PlaySfx(&this->actor, NA_SE_EV_LURE_MOVE_W);
        Rod_Splash(play, &ROD->bobberPos, false);
    }

    // hookset
    if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
        Sfx_PlaySfxCentered(NA_SE_IT_SWORD_SWING);
        if (ROD->biting && (ROD->fish != NULL)) {
            Rod_StartFight(play, player);
            return;
        }
        if ((ROD->fish != NULL) && (ROD->fish->fishState == FISH_STATE_NIBBLE)) {
            EnFishmod_RodMessage(2, "Too early!", NULL);
            EnFishmod_FishSetState(ROD->fish, FISH_STATE_FLEE);
            ROD->fish = NULL;
        }
        ROD->lineOut -= 10.0f; // yanking pulls the bobber toward you
        ROD->dip = 3.0f;
    }

    // a bite that wasn't answered: the fish steals the bait
    if (ROD->biting) {
        if (--ROD->biteTimer <= 0) {
            if (ROD->fish != NULL) {
                EnFishmod_FishSetState(ROD->fish, FISH_STATE_FLEE);
            }
            ROD->fish = NULL;
            ROD->biting = false;
            ROD->dipTarget = 0.0f;
            if (ROD->bait != FISHMOD_BAIT_NONE) {
                ROD->bait = FISHMOD_BAIT_NONE;
                EnFishmod_RodMessage(2, "It stole the bait!", NULL);
            } else {
                EnFishmod_RodMessage(2, "Too late...", NULL);
            }
        }
    }

    ROD->lineOut = CLAMP(ROD->lineOut, 0.0f, LINE_MAX);

    // the line pulls the bobber toward the rod tip when it's shorter than the distance
    toTip.x = ROD->tipPos.x - ROD->bobberPos.x;
    toTip.z = ROD->tipPos.z - ROD->bobberPos.z;
    dist = sqrtf(SQ(toTip.x) + SQ(toTip.z));
    if (dist > ROD->lineOut) {
        f32 step = CLAMP_MAX(dist - ROD->lineOut, 6.0f);

        ROD->bobberPos.x += toTip.x / dist * step;
        ROD->bobberPos.z += toTip.z / dist * step;
    }

    if (ROD->lineOut <= LINE_MIN) {
        Rod_EndCast(play, player);
        return;
    }

    // stay on the water; if it was dragged onto land it becomes a ground bobber
    if (Rod_WaterY(play, &ROD->bobberPos, &waterY) && (Rod_GroundY(play, &ROD->bobberPos) < waterY - 2.0f)) {
        ROD->waterY = waterY;
    } else {
        ROD->state = ROD_STATE_GROUND;
        if (ROD->fish != NULL) {
            EnFishmod_FishSetState(ROD->fish, FISH_STATE_FLEE);
            ROD->fish = NULL;
        }
        ROD->biting = false;
        return;
    }

    Math_ApproachF(&ROD->dip, ROD->dipTarget, 0.35f, 3.0f);
    ROD->bobberPos.y = ROD->waterY - ROD->dip + Math_SinS(play->gameplayFrames * 0x500) * 0.6f;
    (void)groundY;
}

void EnFishmod_RodUpdate(EnFishmod* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Input* input = &play->state.input[0];
    s32 holding = (player->heldItemAction == PLAYER_IA_FISHING_POLE);

    if (gFishmodRodInfo.notice != NULL) {
        // something outside the rod has news (e.g. bait from a beehive)
        EnFishmod_RodMessage(1, gFishmodRodInfo.notice, NULL);
        gFishmodRodInfo.notice = NULL;
    }
    if (ROD->messageTimer > 0) {
        ROD->messageTimer--;
    }


    // stop everything if the rod was put away (or Link got hit, entered water...)
    if (!holding || (player->stateFlags1 & (PLAYER_STATE1_27 | PLAYER_STATE1_DEAD))) {
        if (ROD->state >= ROD_STATE_FLYING) {
            if (ROD->fish != NULL) {
                EnFishmod_FishSetState(ROD->fish, FISH_STATE_FLEE);
            }
            ROD->fish = NULL;
            ROD->biting = false;
            Rod_CameraStop(play);
        }
        ROD->state = holding ? ROD_STATE_HELD : ROD_STATE_NONE;
        return;
    }

    // Link was knocked out of his fishing pose (damage, a cutscene...) while the line was out: end the cast
    if ((ROD->state >= ROD_STATE_FLYING) && (ROD->state != ROD_STATE_LANDING) && (player->unk_860 <= 0)) {
        if (ROD->fish != NULL) {
            EnFishmod_FishSetState(ROD->fish, FISH_STATE_FLEE);
        }
        Rod_EndCast(play, player);
    }

    // sound and effects come from the bobber
    this->actor.world.pos = ROD->bobberPos;

    switch (ROD->state) {
        case ROD_STATE_NONE:
            ROD->state = ROD_STATE_HELD;
            FALLTHROUGH;
        case ROD_STATE_HELD:
            Math_ApproachZeroF(&player->unk_85C, 1.0f, 0.1f);
            Math_ApproachZeroF(&player->unk_858, 1.0f, 0.1f);
            // a bait bottle used from a C button (Fishmod_UseItem) asks for bait on the hook
            if (gFishmodRodInfo.baitRequest != FISHMOD_BAIT_NONE) {
                ROD->bait = gFishmodRodInfo.baitRequest;
                gFishmodRodInfo.baitRequest = FISHMOD_BAIT_NONE;
            }
            if (player->unk_860 == 1) {
                // Link started the cast animation
                ROD->state = ROD_STATE_WINDUP;
                ROD->timer = WINDUP_FRAMES;
                ROD->castPower = 0.0f;
                ROD->castYaw = 0;
            }
            break;

        case ROD_STATE_WINDUP: {
            // Hold the rod's button to charge the cast, release to throw. A tap lobs it a few steps; a full
            // charge reaches about 350 units. The stick aims left/right while charging.
            static u16 sItemButtons[] = { BTN_B, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT };
            u16 rodButton = sItemButtons[CLAMP(player->heldItemButton, 0, 3)];
            s32 charging = CHECK_BTN_ALL(input->cur.button, rodButton) && (ROD->castPower < 1.0f + 0.5f);

            ROD->castYaw = -input->rel.stick_x * 60;
            if (charging && (ROD->timer <= WINDUP_FRAMES - 8)) {
                // hold Link wound back while charging
                player->skelAnime.playSpeed = 0.0f;
                Math_ApproachF(&ROD->castPower, 1.0f, 1.0f, 1.0f / 28.0f);
                if ((ROD->castPower >= 1.0f) && ((play->gameplayFrames % 8) == 0)) {
                    Sfx_PlaySfxCentered(NA_SE_SY_CURSOR);
                }
                ROD->castHolding = true;
                break;
            }
            player->skelAnime.playSpeed = 1.0f;
            ROD->castHolding = false;
            if (--ROD->timer <= 0) {
                s16 yaw = player->actor.shape.rot.y + ROD->castYaw;
                f32 power = CLAMP(ROD->castPower, 0.0f, 1.0f);
                f32 speed = (2.0f + power * 16.0f) * STRENGTH_CAST_SCALE();

                ROD->state = ROD_STATE_FLYING;
                ROD->timer = 0;
                // launch from where the rod tip ends up after the swing: ahead of and above Link
                ROD->bobberPos.x = player->actor.world.pos.x + Math_SinS(yaw) * 30.0f;
                ROD->bobberPos.y = player->actor.world.pos.y + (LINK_IS_CHILD ? 55.0f : 80.0f);
                ROD->bobberPos.z = player->actor.world.pos.z + Math_CosS(yaw) * 30.0f;
                ROD->bobberVel.x = Math_SinS(yaw) * speed;
                ROD->bobberVel.z = Math_CosS(yaw) * speed;
                ROD->bobberVel.y = 4.0f + power * 5.0f;
                ROD->dip = ROD->dipTarget = 0.0f;
                ROD->zoom = 3; // start zoomed all the way in on the bobber (Z cycles it)
                Sfx_PlaySfxCentered(NA_SE_IT_SWORD_SWING_HARD);
                Rod_CameraStart(play);
            }
            break;
        }

        case ROD_STATE_FLYING: {
            Vec3f prev = ROD->bobberPos;
            Vec3f result;
            CollisionPoly* poly;
            s32 bgId;
            f32 waterY;
            f32 groundY;

            // A brakes the line mid-air for a shorter cast
            if (CHECK_BTN_ALL(input->cur.button, BTN_A)) {
                ROD->bobberVel.x *= 0.85f;
                ROD->bobberVel.z *= 0.85f;
            }
            ROD->bobberPos.x += ROD->bobberVel.x;
            ROD->bobberPos.y += ROD->bobberVel.y;
            ROD->bobberPos.z += ROD->bobberVel.z;
            ROD->bobberVel.y -= 0.8f;
            ROD->bobberVel.x *= 0.985f;
            ROD->bobberVel.z *= 0.985f;

            // walls and trees stop it
            ROD->timer++;
            if ((ROD->timer > 3) && BgCheck_EntityLineTest1(&play->colCtx, &prev, &ROD->bobberPos, &result, &poly,
                                                            true, false, false, true, &bgId)) {
                ROD->bobberPos.x = result.x - ROD->bobberVel.x * 0.5f;
                ROD->bobberPos.z = result.z - ROD->bobberVel.z * 0.5f;
                ROD->bobberVel.x = ROD->bobberVel.z = 0.0f;
                if (ROD->bobberVel.y > 0.0f) {
                    ROD->bobberVel.y = 0.0f;
                }
            }

            ROD->lineOut = Rod_DistFromTip(&ROD->bobberPos);

            groundY = Rod_GroundY(play, &ROD->bobberPos);
            if (Rod_WaterY(play, &ROD->bobberPos, &waterY) && (groundY < waterY) &&
                (ROD->bobberPos.y <= waterY)) {
                ROD->state = ROD_STATE_FLOAT;
                ROD->waterY = waterY;
                ROD->bobberPos.y = waterY;
                ROD->dip = 6.0f;
                ROD->splashed = true;
                EnFishmod_Disturb(play, &ROD->bobberPos, 0.0f, 0);
                // the bait starts sinking under the bobber: A stops it at a depth (once per cast), or it stops by
                // itself a little above the bottom
                ROD->baitDepth = BAIT_DEPTH_START;
                ROD->sinking = true;
                ROD->reelLock = false;
                ROD->lineOut = Math_Vec3f_DistXZ(&ROD->tipPos, &ROD->bobberPos);
                Actor_PlaySfx(&this->actor, NA_SE_EV_BOMB_DROP_WATER);
                Rod_Splash(play, &ROD->bobberPos, true);
                player->unk_860 = 2;
            } else if (ROD->bobberPos.y <= groundY) {
                ROD->state = ROD_STATE_GROUND;
                ROD->bobberPos.y = groundY;
                ROD->lineOut = Math_Vec3f_DistXZ(&ROD->tipPos, &ROD->bobberPos);
                player->unk_860 = 2;
            } else if (ROD->bobberPos.y < ROD->tipPos.y - 600.0f) {
                Rod_EndCast(play, player); // fell off a cliff
            }
            Rod_CameraUpdate(play, player, &ROD->bobberPos);
            break;
        }

        case ROD_STATE_GROUND:
            // only reeling makes sense here
            if (CHECK_BTN_ALL(input->cur.button, BTN_A) || CHECK_BTN_ALL(input->press.button, BTN_B)) {
                Vec3f toTip;
                f32 dist;

                ROD->lineOut -= 4.0f;
                toTip.x = ROD->tipPos.x - ROD->bobberPos.x;
                toTip.z = ROD->tipPos.z - ROD->bobberPos.z;
                dist = sqrtf(SQ(toTip.x) + SQ(toTip.z));
                if (dist > 1.0f) {
                    ROD->bobberPos.x += toTip.x / dist * CLAMP_MAX(dist, 4.0f);
                    ROD->bobberPos.z += toTip.z / dist * CLAMP_MAX(dist, 4.0f);
                }
                ROD->bobberPos.y = Rod_GroundY(play, &ROD->bobberPos);
                Sfx_PlaySfxCentered(NA_SE_IT_FISHING_REEL_SLOW - SFX_FLAG);
                if (dist < LINE_MIN) {
                    Rod_EndCast(play, player);
                    break;
                }
                {
                    // dragged off the bank into the water: it floats again
                    f32 waterY;

                    if (Rod_WaterY(play, &ROD->bobberPos, &waterY) &&
                        (ROD->bobberPos.y < waterY - 2.0f)) {
                        ROD->state = ROD_STATE_FLOAT;
                        ROD->waterY = waterY;
                        ROD->bobberPos.y = waterY;
                        ROD->dip = ROD->dipTarget = 0.0f;
                        ROD->baitDepth = BAIT_DEPTH_START;
                        ROD->sinking = false;
                        ROD->lineOut = Math_Vec3f_DistXZ(&ROD->tipPos, &ROD->bobberPos);
                        Rod_Splash(play, &ROD->bobberPos, false);
                    }
                }
            }
            Rod_CameraUpdate(play, player, &ROD->bobberPos);
            break;

        case ROD_STATE_FLOAT:
            if (CHECK_BTN_ALL(input->press.button, BTN_Z)) {
                ROD->zoom = (ROD->zoom + 1) % 4;
                Sfx_PlaySfxCentered((ROD->zoom == 0) ? NA_SE_SY_CAMERA_ZOOM_DOWN : NA_SE_SY_CAMERA_ZOOM_UP);
            }
            Rod_UpdateFloat(this, play, player, input);
            if (ROD->state == ROD_STATE_FLOAT) {
                // look at the bait, part way down the line (all the way down would lose the bobber)
                Vec3f focus = ROD->bobberPos;

                focus.y -= ROD->baitDepth * 0.6f;
                Rod_CameraUpdate(play, player, &focus);
            }
            break;

        case ROD_STATE_FIGHT:
            if (ROD->fish == NULL) {
                ROD->state = ROD_STATE_FLOAT;
                break;
            }
            Rod_UpdateFight(this, play, player, input);
            if ((ROD->state == ROD_STATE_FIGHT) || (ROD->state == ROD_STATE_LANDING)) {
                Vec3f focus;

                focus = ROD->fish->actor.world.pos;
                ROD->bobberPos.x = focus.x;
                ROD->bobberPos.z = focus.z;
                ROD->bobberPos.y = CLAMP_MIN(focus.y, ROD->waterY - 4.0f);
                Rod_CameraUpdate(play, player, &focus);
            }
            break;

        case ROD_STATE_LANDING: {
            // Link holds the fish up to the fanfare, then a textbox names it and asks whether to keep it (into an
            // empty bottle) or let it go. Without an empty bottle it just goes back.
            EnFishmod* fish = ROD->fish;
            s32 decided = 0; // 1 keep, 2 release
            u8 textState = Message_GetState(&play->msgCtx);

            ROD->timer--;
            if (ROD->timer == 0) {
                ROD->choice = Inventory_HasEmptyBottle() ? 1 : 2;
                Fishmod_ShowCatchMessage(play, (fish != NULL) ? fish->species->name : "fish", sCatchSize, sCatchRecord,
                                         ROD->choice == 1);
            } else if (ROD->timer < 0) {
                if ((ROD->choice == 1) && (textState == TEXT_STATE_CHOICE) && Message_ShouldAdvance(play)) {
                    decided = (play->msgCtx.choiceIndex == 0) ? 1 : 2;
                    Message_CloseTextbox(play);
                } else if ((ROD->choice == 2) && (textState == TEXT_STATE_DONE) && Message_ShouldAdvance(play)) {
                    decided = 2;
                    Message_CloseTextbox(play);
                } else if ((textState == TEXT_STATE_NONE) && (ROD->timer < -10)) {
                    decided = 2; // the textbox went away some other way
                }
            }

            if (decided != 0) {
                ROD->choice = 0;
                if (fish != NULL) {
                    if (decided == 1) {
                        Fishmod_BottleFish(fish->species - gFishmodSpecies, fish->lengthInches, fish->look);
                        Item_Give(play, ITEM_BOTTLE_FISH);
                        EnFishmod_RosterCaught(play, fish);
                        Sfx_PlaySfxCentered(NA_SE_SY_GET_ITEM);
                        EnFishmod_RodMessage(1, "Into the bottle!", NULL);
                        // FLEE releases it from the rod (clearing ROD->fish), so use our own pointer
                        EnFishmod_FishSetState(fish, FISH_STATE_FLEE);
                        Actor_Kill(&fish->actor);
                    } else {
                        // back into the water where it was hooked, somewhere deep enough to swim off
                        Vec3f spot;
                        f32 surface;
                        f32 floor;

                        if (!EnFishmod_FindWaterPoint(play, &ROD->bobberPos, 40.0f, fish->halfHeight * 2.0f + 4.0f,
                                                      &spot, &surface, &floor) &&
                            !EnFishmod_FindWaterPoint(play, &fish->actor.home.pos, 60.0f,
                                                      fish->halfHeight * 2.0f + 4.0f, &spot, &surface, &floor)) {
                            spot = fish->actor.home.pos;
                        }
                        fish->actor.world.pos = spot;
                        fish->actor.shape.rot.z = 0;
                        EnFishmod_FishSetState(fish, FISH_STATE_FLEE);
                        Rod_Splash(play, &ROD->bobberPos, true);
                        Actor_PlaySfx(&this->actor, NA_SE_EV_DIVE_INTO_WATER_L);
                        EnFishmod_RodMessage(0, "Released.", NULL);
                    }
                }
                ROD->fish = NULL;
                Rod_EndCast(play, player);
            } else if (fish != NULL) {
                Rod_CameraUpdate(play, player, &fish->actor.world.pos);
            }
            break;
        }
    }

    // stick drives Link's reel animation like in the pond
    if ((ROD->state == ROD_STATE_FLOAT) || (ROD->state == ROD_STATE_FIGHT)) {
        f32 lean = input->rel.stick_x * 0.02f;

        Math_SmoothStepToF(&player->unk_85C, input->rel.stick_y * 0.02f, 0.3f, 5.0f, 0.0f);
        if ((ROD->state == ROD_STATE_FIGHT) && (ROD->fish != NULL)) {
            // the pull drags Link's arms toward the fish's side
            s16 rel = Math_Vec3f_Yaw(&player->actor.world.pos, &ROD->fish->actor.world.pos) - player->actor.shape.rot.y;

            lean -= Math_SinS(rel) * CLAMP(ROD->tension * 0.01f, 0.0f, 1.0f) * 0.6f;
        }
        Math_SmoothStepToF(&player->unk_858, lean, 0.3f, 5.0f, 0.0f);
        player->unk_85C = CLAMP(player->unk_85C, -1.0f, 1.0f);
        player->unk_858 = CLAMP(player->unk_858, -1.0f, 1.0f);
    }

    if (ROD->state >= ROD_STATE_FLYING) {
        Rod_UpdateLineFloor(play);
    }

    gFishmodRodInfo.state = ROD->state;
    gFishmodRodInfo.biting = ROD->biting;
    gFishmodRodInfo.nibbling = (ROD->fish != NULL) && (ROD->fish->fishState == FISH_STATE_NIBBLE);
    gFishmodRodInfo.tension = ROD->tension;
    gFishmodRodInfo.castPower = ROD->castPower;
    gFishmodRodInfo.choice = ROD->choice;
    gFishmodRodInfo.bait = ROD->bait;

#if FISHMOD_DEBUG
    {
        static const char* sStateNames[] = { "none", "held", "windup", "flying", "float", "ground", "fight", "land" };

        Fishmod_DebugPrintf(2, "rod %s line %d bait %d%s", sStateNames[ROD->state], (s32)ROD->lineOut,
                            (s32)ROD->baitDepth, ROD->sinking ? " sink" : "");
        Fishmod_DebugPrintf(3, "ten %d stam %d bite %d hold st%d d%d", (s32)ROD->tension,
                            (ROD->fish != NULL) ? (s32)ROD->fish->stamina : -1, ROD->biting,
                            (ROD->fish != NULL) ? ROD->fish->fishState : -1,
                            (ROD->fish != NULL) ? (s32)Math_Vec3f_DistXZ(&ROD->fish->actor.world.pos, &ROD->bobberPos)
                                                : -1);
        Fishmod_DebugPrintf(4, "bob %d %d %d t%d p%d v%d %d", (s32)ROD->bobberPos.x, (s32)ROD->bobberPos.y,
                            (s32)ROD->bobberPos.z, ROD->timer, (s32)(ROD->castPower * 100.0f),
                            (s32)(ROD->bobberVel.x * 10.0f), (s32)(ROD->bobberVel.z * 10.0f));
        Fishmod_DebugPrintf(7, "tip %d %d %d drape %d", (s32)ROD->tipPos.x, (s32)ROD->tipPos.y, (s32)ROD->tipPos.z,
                            sLineDraped);
        sLineDraped = 0;
    }
#endif
}

/* ---------------------------------------------------------------------------------------------------------------
 * Drawing: rod (from the pond), line, bobber, hook; HUD (tension gauge and messages)
 */

static f32 sRodScales[22] = {
    1.0f,        1.0f,        1.0f,        0.9625f,     0.925f, 0.8875f,     0.85f,       0.8125f,
    0.775f,      0.73749995f, 0.7f,        0.6625f,     0.625f, 0.5875f,     0.54999995f, 0.5125f,
    0.47499996f, 0.4375f,     0.39999998f, 0.36249995f, 0.325f, 0.28749996f,
};

static f32 sRodBendRatios[22] = {
    0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.06f,   0.12f,   0.18f,   0.24f,   0.30f,   0.36f,
    0.42f, 0.48f, 0.54f, 0.60f, 0.60f, 0.5142f, 0.4285f, 0.3428f, 0.2571f, 0.1714f, 0.0857f,
};

/**
 * Turns the rod's bend toward the fish. The segments bend about their local x axis, which curls the tip toward
 * local -y; rolling the rod about its own axis (z) first points that curl wherever the line pulls. A pull straight
 * along the rod barely bends it, a pull across it bends it fully. Uses the current matrix (the rod's base).
 */
static void Rod_AimBend(PlayState* play) {
    f32 rollTarget = 0.0f;
    f32 pullTarget = 1.0f;

    if ((ROD->state == ROD_STATE_FIGHT) && (ROD->tension > 1.0f)) {
        MtxF mf;
        f32 dx = ROD->bobberPos.x - ROD->tipPos.x;
        f32 dy = ROD->bobberPos.y - ROD->tipPos.y;
        f32 dz = ROD->bobberPos.z - ROD->tipPos.z;
        f32 lx;
        f32 ly;
        f32 lz;
        f32 across;

        Matrix_Get(&mf);
        // the pull in the rod's own axes (the axes share one scale, so no need to normalize)
        lx = dx * mf.xx + dy * mf.yx + dz * mf.zx;
        ly = dx * mf.xy + dy * mf.yy + dz * mf.zy;
        lz = dx * mf.xz + dy * mf.yz + dz * mf.zz;
        across = sqrtf(SQ(lx) + SQ(ly));
        if (across > 0.0f) {
            // roll so local -y faces (lx, ly): rotating about z by r takes -y to (sin r, -cos r)
            rollTarget = Math_FAtan2F(lx, -ly);
            pullTarget = 0.3f + 0.7f * (across / sqrtf(SQ(across) + SQ(lz)));
        }
    }
    // turn the shorter way round, and not instantly
    {
        f32 diff = rollTarget - ROD->bendRoll;

        while (diff > M_PI) {
            diff -= 2.0f * M_PI;
        }
        while (diff < -M_PI) {
            diff += 2.0f * M_PI;
        }
        ROD->bendRoll += diff * 0.2f;
        if (ROD->bendRoll > M_PI) {
            ROD->bendRoll -= 2.0f * M_PI;
        } else if (ROD->bendRoll < -M_PI) {
            ROD->bendRoll += 2.0f * M_PI;
        }
    }
    Math_ApproachF(&ROD->bendPull, pullTarget, 0.2f, 0.1f);
}

static void Rod_DrawRod(PlayState* play, Player* player) {
    static Vec3f sZero = { 0.0f, 0.0f, 0.0f };
    f32 bend;
    s32 i;

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);

    // the rod bends with the line tension and the reel stick
    bend = -0.03f + (ROD->tension * 0.012f * ROD->bendPull) + player->unk_85C * -0.15f;
    if (ROD->state == ROD_STATE_WINDUP) {
        bend += 0.6f;
    }
    if ((ROD->state == ROD_STATE_FIGHT) && (ROD->fish != NULL) && (ROD->fish->stateTimer > 0)) {
        // a running fish shakes the tip
        bend += Math_SinS(play->gameplayFrames * 0x3800) * ROD->tension * 0.0012f;
    }

    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, gFishingRodMaterialDL);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 155, 0, 255);

    Matrix_Mult(&player->mf_9E0, MTXMODE_NEW);
    if (LINK_IS_CHILD) {
        Matrix_Translate(0.0f, 230.0f, 0.0f, MTXMODE_APPLY);
    } else {
        Matrix_Translate(0.0f, 400.0f, 0.0f, MTXMODE_APPLY);
    }
    Matrix_RotateY(0.41f * M_PI, MTXMODE_APPLY);
    Matrix_RotateX(-M_PI / 5.0000003f, MTXMODE_APPLY);
    Matrix_RotateZ((player->unk_858 * 0.5f) + 3.0f * M_PI / 20.0f, MTXMODE_APPLY);
    Matrix_RotateX((20.0f) * 0.01f * M_PI, MTXMODE_APPLY);
    Matrix_Scale(0.70000005f, 0.70000005f, 0.70000005f, MTXMODE_APPLY);
    Matrix_Translate(0.0f, 0.0f, -1300.0f, MTXMODE_APPLY);
    Rod_AimBend(play);
    Matrix_RotateZ(ROD->bendRoll, MTXMODE_APPLY);

    // At rest the rod is nearly straight: draw its segments in pairs, the second of each pair from the first's
    // matrix times a fixed one-segment offset (the segments are 500 long, scaled 0.52 along the rod). The pair's
    // two bends are applied together, so the tip still ends up where the line is attached.
    if (ABS(bend) < 0.08f) {
        static Mtx sNextSegment;
        static s32 sNextSegmentReady = false;

        if (!sNextSegmentReady) {
            guTranslate(&sNextSegment, 0.0f, 0.0f, 500.0f / 0.52f);
            sNextSegmentReady = true;
        }
        for (i = 0; i < 22; i += 2) {
            s32 k;

            Matrix_RotateX(bend * (sRodBendRatios[i] + sRodBendRatios[i + 1]) * 0.5f, MTXMODE_APPLY);
            Matrix_Push();
            Matrix_Scale(sRodScales[i], sRodScales[i], 0.52f, MTXMODE_APPLY);
            MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
            for (k = i; k < i + 2; k++) {
                if (k == i + 1) {
                    gSPMatrix(POLY_OPA_DISP++, &sNextSegment, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
                }
                if (k < 5) {
                    gDPLoadTextureBlock(POLY_OPA_DISP++, gFishingRodSegmentBlackTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16,
                                        8, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD,
                                        G_TX_NOLOD);
                } else if ((k < 8) || ((k % 2) == 0)) {
                    gDPLoadTextureBlock(POLY_OPA_DISP++, gFishingRodSegmentWhiteTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16,
                                        8, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD,
                                        G_TX_NOLOD);
                } else {
                    gDPLoadTextureBlock(POLY_OPA_DISP++, gFishingRodSegmentStripTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16,
                                        8, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD,
                                        G_TX_NOLOD);
                }
                gSPDisplayList(POLY_OPA_DISP++, gFishingRodSegmentDL);
            }
            Matrix_Pop();
            Matrix_Translate(0.0f, 0.0f, 1000.0f, MTXMODE_APPLY);
        }
        Matrix_MultVec3f(&sZero, &ROD->tipPos);
    } else

    for (i = 0; i < 22; i++) {
        Matrix_RotateX(bend * sRodBendRatios[i] * 0.5f, MTXMODE_APPLY);

        Matrix_Push();
        Matrix_Scale(sRodScales[i], sRodScales[i], 0.52f, MTXMODE_APPLY);
        MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);

        if (i < 5) {
            gDPLoadTextureBlock(POLY_OPA_DISP++, gFishingRodSegmentBlackTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 8, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD, G_TX_NOLOD);
        } else if ((i < 8) || ((i % 2) == 0)) {
            gDPLoadTextureBlock(POLY_OPA_DISP++, gFishingRodSegmentWhiteTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 8, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD, G_TX_NOLOD);
        } else {
            gDPLoadTextureBlock(POLY_OPA_DISP++, gFishingRodSegmentStripTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 8, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD, G_TX_NOLOD);
        }
        gSPDisplayList(POLY_OPA_DISP++, gFishingRodSegmentDL);

        Matrix_Pop();
        Matrix_Translate(0.0f, 0.0f, 500.0f, MTXMODE_APPLY);
        if (i == 21) {
            Matrix_MultVec3f(&sZero, &ROD->tipPos);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
}

static void Rod_DrawLineSegment(PlayState* play, Vec3f* a, Vec3f* b, f32 thickness) {
    f32 dx = b->x - a->x;
    f32 dy = b->y - a->y;
    f32 dz = b->z - a->z;
    f32 distXZ = sqrtf(SQ(dx) + SQ(dz));
    f32 dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);

    Matrix_Translate(a->x, a->y, a->z, MTXMODE_NEW);
    Matrix_RotateY(Math_FAtan2F(dx, dz), MTXMODE_APPLY);
    Matrix_RotateX(-Math_FAtan2F(dy, distXZ), MTXMODE_APPLY);
    Matrix_Scale(thickness, 1.0f, dist * 0.001f, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    gSPDisplayList(POLY_XLU_DISP++, gFishingLineModelDL);

    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
}

/** Where point t (0 at the tip, 1 at the bobber) of the line hangs, before draping it over the ground. */
static void Rod_LinePoint(Vec3f* end, f32 sag, f32 t, Vec3f* out) {
    out->x = ROD->tipPos.x + (end->x - ROD->tipPos.x) * t;
    out->y = ROD->tipPos.y + (end->y - ROD->tipPos.y) * t - sag * 4.0f * t * (1.0f - t);
    out->z = ROD->tipPos.z + (end->z - ROD->tipPos.z) * t;
}

static void Rod_LineShape(Vec3f* end, f32* sag) {
    *end = ROD->bobberPos;
    if (ROD->state == ROD_STATE_FIGHT) {
        end->y = CLAMP_MIN(end->y, ROD->waterY);
    }

    // a slack line sags; a tight one is straight
    *sag = Math_Vec3f_DistXYZ(&ROD->tipPos, end) * 0.12f;
    if (ROD->state == ROD_STATE_FIGHT) {
        *sag *= CLAMP(1.0f - ROD->tension / 40.0f, 0.0f, 1.0f);
    } else if (ROD->state == ROD_STATE_FLYING) {
        *sag *= 0.3f;
    }
}

/**
 * Finds how low each point of the line may hang so it lies on the banks and floats on the water instead of
 * cutting through them. Done in update (it raycasts); the draw then shapes the line from the current rod tip.
 */
static void Rod_UpdateLineFloor(PlayState* play) {
    Vec3f end;
    Vec3f point;
    f32 sag;
    f32 top;
    f32 floorY;
    f32 waterY;
    s32 i;

    Rod_LineShape(&end, &sag);
    top = CLAMP_MIN(ROD->tipPos.y, end.y) + 10.0f;
    for (i = 1; i < LINE_SEGMENTS; i++) {
        Rod_LinePoint(&end, sag, (f32)i / LINE_SEGMENTS, &point);
        if (EnFishmod_WaterFloor(play, point.x, point.z, &waterY, &floorY, false)) {
            floorY += 0.8f;
            if (waterY > floorY) {
                floorY = waterY + 0.3f; // a slack line floats
            }
        } else {
            CollisionPoly* poly;

            // cast from above the whole line so a point already inside a bank still finds the bank's top
            point.y = top;
            floorY = BgCheck_EntityRaycastDown1(&play->colCtx, &poly, &point) + 0.8f;
        }
        ROD->lineFloor[i] = floorY;
    }
}

static void Rod_DrawLine(PlayState* play) {
    Vec3f end;
    Vec3f prev;
    Vec3f cur;
    f32 sag;
    s32 i;

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);

    POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_20);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 90);

    Rod_LineShape(&end, &sag);
    // The whole line lies in one vertical plane, so it's one flat strip under a single matrix (the same 0.3-unit
    // wide horizontal ribbon the per-segment model drew). Vertices are relative to the rod tip in 1/32 units.
    {
        Vtx* vtx = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Vtx) * 2 * (LINE_SEGMENTS + 1));
        f32 yaw = Math_FAtan2F(end.x - ROD->tipPos.x, end.z - ROD->tipPos.z);
        s16 sideX = (s16)(cosf(yaw) * 0.15f * 32.0f + ((cosf(yaw) < 0.0f) ? -0.5f : 0.5f));
        s16 sideZ = (s16)(-sinf(yaw) * 0.15f * 32.0f + ((sinf(yaw) > 0.0f) ? -0.5f : 0.5f));

        if ((sideX == 0) && (sideZ == 0)) {
            sideX = 5;
        }
        for (i = 0; i <= LINE_SEGMENTS; i++) {
            Vtx* v = &vtx[2 * i];
            s32 k;

            if (i == 0) {
                cur = ROD->tipPos;
            } else if (i < LINE_SEGMENTS) {
                Rod_LinePoint(&end, sag, (f32)i / LINE_SEGMENTS, &cur);
#if FISHMOD_DEBUG
                if (cur.y < ROD->lineFloor[i]) {
                    sLineDraped++;
                }
#endif
                cur.y = CLAMP_MIN(cur.y, ROD->lineFloor[i]);
            } else {
                cur = end;
            }
            for (k = 0; k < 2; k++) {
                s16 sign = (k == 0) ? -1 : 1;

                v[k].v.ob[0] = (s16)((cur.x - ROD->tipPos.x) * 32.0f) + sign * sideX;
                v[k].v.ob[1] = (s16)((cur.y - ROD->tipPos.y) * 32.0f);
                v[k].v.ob[2] = (s16)((cur.z - ROD->tipPos.z) * 32.0f) + sign * sideZ;
                v[k].v.flag = 0;
                v[k].v.tc[0] = v[k].v.tc[1] = 0;
                v[k].v.cn[0] = v[k].v.cn[1] = v[k].v.cn[2] = v[k].v.cn[3] = 255;
            }
        }

        Matrix_Translate(ROD->tipPos.x, ROD->tipPos.y, ROD->tipPos.z, MTXMODE_NEW);
        Matrix_Scale(1.0f / 32.0f, 1.0f / 32.0f, 1.0f / 32.0f, MTXMODE_APPLY);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
        // the vertex buffer holds 32: points 0-15 first, then 15-16
        gSPVertex(POLY_XLU_DISP++, vtx, 32, 0);
        for (i = 0; i < 15; i++) {
            gSP2Triangles(POLY_XLU_DISP++, 2 * i, 2 * i + 1, 2 * i + 3, 0, 2 * i, 2 * i + 3, 2 * i + 2, 0);
        }
        gSPVertex(POLY_XLU_DISP++, &vtx[30], 4, 0);
        gSP2Triangles(POLY_XLU_DISP++, 0, 1, 3, 0, 0, 3, 2, 0);
        (void)prev;
    }

    // leader from the bobber down to the hook
    if ((ROD->state == ROD_STATE_FLOAT) || (ROD->state == ROD_STATE_GROUND) || (ROD->state == ROD_STATE_FLYING)) {
        Vec3f hook = ROD->bobberPos;

        hook.y -= (ROD->state == ROD_STATE_FLOAT) ? ROD->baitDepth : 4.0f;
        Rod_DrawLineSegment(play, &ROD->bobberPos, &hook, 0.001f);
    }

    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
}

static void Rod_DrawBobber(PlayState* play) {
    f32 tilt;

    if (ROD->state == ROD_STATE_FIGHT || ROD->state == ROD_STATE_LANDING) {
        return; // it's on the fish
    }

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);

    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    tilt = (ROD->dip > 1.0f) ? ROD->dip * 0.04f : 0.0f;
    Matrix_Translate(ROD->bobberPos.x, ROD->bobberPos.y, ROD->bobberPos.z, MTXMODE_NEW);
    Matrix_RotateZ(tilt, MTXMODE_APPLY);
    Matrix_Scale(0.012f, 0.012f, 0.012f, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    gSPDisplayList(POLY_OPA_DISP++, sBobberDL);

    // hook under the bobber
    if (ROD->state == ROD_STATE_FLOAT) {
        Matrix_Translate(ROD->bobberPos.x, ROD->bobberPos.y - ROD->baitDepth, ROD->bobberPos.z, MTXMODE_NEW);
        Matrix_Scale(0.004f, 0.004f, 0.004f, MTXMODE_APPLY);
        MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
        gSPDisplayList(POLY_OPA_DISP++, gFishingLureHookDL);
    }

    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
}

static void Rod_FillRect(Gfx** gfxP, s32 x0, s32 y0, s32 x1, s32 y1, u8 r, u8 g, u8 b) {
    Gfx* gfx = *gfxP;

    gDPPipeSync(gfx++);
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gfx++, 0, 0, r, g, b, 220);
    gDPFillRectangle(gfx++, x0, y0, x1, y1);
    *gfxP = gfx;
}

static void Rod_DrawHud(PlayState* play) {
    Gfx* gfx;
    GfxPrint printer;

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);

    gfx = OVERLAY_DISP;

    // tension gauge while fighting: green in the middle, red at the ends
    if (ROD->state == ROD_STATE_FIGHT) {
        s32 x0 = 110;
        s32 x1 = 210;
        s32 y = 200;
        s32 fill = x0 + (s32)(CLAMP(ROD->tension, 0.0f, 100.0f));
        u8 r;
        u8 g;

        if ((ROD->tension > 85.0f) || (ROD->tension < 12.0f)) {
            r = 240;
            g = 60;
        } else if ((ROD->tension > 70.0f) || (ROD->tension < 25.0f)) {
            r = 240;
            g = 200;
        } else {
            r = 80;
            g = 230;
        }
        Rod_FillRect(&gfx, x0 - 2, y - 2, x1 + 2, y + 8, 20, 20, 30);
        Rod_FillRect(&gfx, x0 + 25, y - 2, x0 + 70, y - 1, 120, 230, 120); // sweet spot marker
        Rod_FillRect(&gfx, x0, y, fill, y + 6, r, g, 40);
    }

    // cast power while charging
    if ((ROD->state == ROD_STATE_WINDUP) && ROD->castHolding) {
        s32 x0 = 110;
        s32 x1 = 210;
        s32 y = 200;
        s32 fill = x0 + (s32)(CLAMP(ROD->castPower, 0.0f, 1.0f) * 100.0f);

        Rod_FillRect(&gfx, x0 - 2, y - 2, x1 + 2, y + 8, 20, 20, 30);
        Rod_FillRect(&gfx, x0, y, fill, y + 6, 90, 170, 255);
    }

    // messages
    if (true) {
        GfxPrint_Init(&printer);
        GfxPrint_Open(&printer, gfx);
        if (ROD->state == ROD_STATE_FIGHT) {
            GfxPrint_SetColor(&printer, 255, 255, 255, 255);
            GfxPrint_SetPos(&printer, 13, 23);
            GfxPrint_Printf(&printer, "TENSION");
        } else if (ROD->state == ROD_STATE_WINDUP) {
            GfxPrint_SetColor(&printer, 255, 255, 255, 255);
            GfxPrint_SetPos(&printer, 13, 23);
            GfxPrint_Printf(&printer, "CAST");
        }
        if (ROD->messageTimer == 0) {
            const char* hint = NULL;

            switch (ROD->state) {
                case ROD_STATE_HELD:
                    hint = "Hold the rod button to cast";
                    break;
                case ROD_STATE_WINDUP:
                    hint = "Release to cast  Stick: aim";
                    break;
                case ROD_STATE_FLOAT:
                    hint = "A:Reel  B:Set hook  Z:Zoom";
                    break;
                case ROD_STATE_GROUND:
                    hint = "A: Reel in";
                    break;
                case ROD_STATE_FIGHT:
                    hint = "A:Reel  Stick: pull against it";
                    break;
            }
            if ((ROD->state == ROD_STATE_HELD) || (ROD->state == ROD_STATE_FLOAT)) {
                // what's on the hook, under the hint
                static char sBaitLine[40];

                if (ROD->bait != FISHMOD_BAIT_NONE) {
                    sprintf(sBaitLine, "Bait: %s", Fishmod_BaitName(ROD->bait));
                } else if (ROD->state == ROD_STATE_HELD) {
                    sprintf(sBaitLine, "No bait: use a bait bottle");
                } else {
                    sprintf(sBaitLine, "No bait");
                }
                GfxPrint_SetColor(&printer, 255, 230, 160, 200);
                GfxPrint_SetPos(&printer, 20 - strlen(sBaitLine) / 2, 8);
                GfxPrint_Printf(&printer, "%s", sBaitLine);
            }
            if (hint != NULL) {
                // just under the top of the HUD (the bottom has the map and rupees), until there's a tutorial
                GfxPrint_SetColor(&printer, 220, 220, 220, 200);
                GfxPrint_SetPos(&printer, 20 - strlen(hint) / 2, 7);
                GfxPrint_Printf(&printer, "%s", hint);
            }
        }
        if (ROD->messageTimer > 0) {
            if (ROD->messageColor == 1) {
                GfxPrint_SetColor(&printer, 255, 240, 120, 255);
            } else if (ROD->messageColor == 2) {
                GfxPrint_SetColor(&printer, 255, 140, 120, 255);
            } else {
                GfxPrint_SetColor(&printer, 255, 255, 255, 255);
            }
            GfxPrint_SetPos(&printer, 20 - strlen(ROD->message[0]) / 2, 5);
            GfxPrint_Printf(&printer, "%s", ROD->message[0]);
            if (ROD->message[1][0] != '\0') {
                GfxPrint_SetColor(&printer, 255, 255, 255, 255);
                GfxPrint_SetPos(&printer, 20 - strlen(ROD->message[1]) / 2, 7);
                GfxPrint_Printf(&printer, "%s", ROD->message[1]);
            }
        }
        gfx = GfxPrint_Close(&printer);
        GfxPrint_Destroy(&printer);
    }

    OVERLAY_DISP = gfx;

    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
}

void EnFishmod_RodDraw(EnFishmod* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (ROD->state == ROD_STATE_NONE) {
        if ((ROD->messageTimer > 0) && (R_PAUSE_BG_PRERENDER_STATE == PAUSE_BG_PRERENDER_OFF)) {
            Rod_DrawHud(play);
        }
        return;
    }

    Rod_DrawRod(play, player);
    if (ROD->state >= ROD_STATE_FLYING) {
        Rod_DrawLine(play);
        Rod_DrawBobber(play);
    }
    // not in the picture the pause menu takes of the screen
    if (R_PAUSE_BG_PRERENDER_STATE == PAUSE_BG_PRERENDER_OFF) {
        Rod_DrawHud(play);
    }
}
