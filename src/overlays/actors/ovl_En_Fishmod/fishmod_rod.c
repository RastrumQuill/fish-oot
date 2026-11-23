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
#include "fishmod_feel.h"

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
#include "letterbox.h"
#include "sequence.h"
#include "seqcmd.h"
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
#define BAIT_ABOVE_BOTTOM 6.0f  // the bait never hangs nearer the bed than this
#define BAIT_DEPTH_FRACTION 0.35f // of the water's depth under the bobber: where the bait hangs
// The most line under a bobber (about the rod's length, some 5 m): a float rig fishes the top of the water, not the
// deep basin or the Water Temple's gate (that's for sinkers and lures)
#define BAIT_DEPTH_MAX 180.0f
// the other rigs (FishmodRig): how deep each goes and how fast (Rod_UpdateRigDepth)
#define POND_DIVE_SLOW 18.0f   // the pond lure's dive while reeled (A), and fast (A and R)
#define POND_DIVE_FAST 30.0f
#define SINK_RATE 1.2f         // the sinking lure going down, units a frame
#define SINK_LIFT_SLOW 12.0f   // how far the reel lifts it off the bed (A), and fast
#define SINK_LIFT_FAST 26.0f
#define SINKER_LEADER 18.0f    // the sinker's leader: the bait floats this far above the weight on the bed
#define SINKER_RATE 1.6f
#define UNDER_CAM_IN 45.0f     // the camera follows the tackle under once the bait's this deep, and back up at...
#define UNDER_CAM_OUT 28.0f    // ...this (apart, so it doesn't flicker between the two)
#define RIG_UNDERWATER() ((ROD->rig == FISHMOD_RIG_LURE_SINKING) || (ROD->rig == FISHMOD_RIG_SINKER))
#define LINE_MAX_BASE 460.0f
// Strength upgrades (Goron's Bracelet, Silver and Golden Gauntlets) let Link cast further and play out more line
// (+35% each: about twice as far with the Golden Gauntlets), scaled to the scene's water (Fishmod_CastSceneScale)
static s16 sRodScene; // the scene the rod is in (EnFishmod_RodUpdate), for the cast's reach there
#define STRENGTH_CAST_SCALE() \
    ((1.0f + 0.35f * CUR_UPG_VALUE(UPG_STRENGTH)) * Fishmod_CastSceneScale(sRodScene))
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

// the sinker: a lead teardrop (its ring on top), grey, shaded by its vertex colours
static Vtx sSinkerVtx[] = {
    VTX(0, 100, 0, 0, 0, 150, 155, 165, 255),  // the ring's end
    VTX(45, 20, 0, 0, 0, 120, 125, 135, 255),  // the belly, round
    VTX(0, 20, 45, 0, 0, 105, 110, 120, 255),
    VTX(-45, 20, 0, 0, 0, 85, 90, 100, 255),
    VTX(0, 20, -45, 0, 0, 100, 105, 115, 255),
    VTX(0, -45, 0, 0, 0, 70, 72, 80, 255),     // the bottom
};

static Gfx sSinkerDL[] = {
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPClearGeometryMode(G_LIGHTING | G_CULL_BACK),
    gsSPVertex(sSinkerVtx, 6, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(0, 3, 4, 0, 0, 4, 1, 0),
    gsSP2Triangles(5, 2, 1, 0, 5, 3, 2, 0),
    gsSP2Triangles(5, 4, 3, 0, 5, 1, 4, 0),
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

/** A fish comes to the bait: it's the rod's fish unless another already is. Returns true if it's this one. */
s32 EnFishmod_RodClaim(EnFishmod* fish) {
    if ((ROD->fish == NULL) || (ROD->fish == fish)) {
        ROD->fish = fish;
        return true;
    }
    return false;
}

/*
 * Rumble (the Rumble Pak), felt like Twilight Princess's fishing: a tap for each nibble, a hard jolt when it takes the
 * bait and a harder one when the hook goes in, then the fight through the line: a steady rumble that follows the
 * tension (none on a slack line), a jolt when the fish starts a run or leaps, a strong buzz while the drag pays out.
 * The motor is pulsed: each retrace (60 a second) the strength is added to a counter and the motor gets a pulse when it
 * overflows 255 (sys_rumble.c), so 255 is on every retrace and 60 about one in four; durations are in retraces (3 to a
 * game frame). Strengths are scaled by the feel tuner's rumble_* (include/fishmod_feel.h).
 */
static s16 sRumbleJolt;
static u8 sRumbleRunning; // (the fight's last frame: a run, a leap)
static u8 sRumbleLeaping; // game frames a jolt still has the motor (the fight's steady rumble waits for it)
enum { FIGHT_CH_BASS, FIGHT_CH_ROOT, FIGHT_CH_PAD, FIGHT_CH_DRUMS, FIGHT_CH_STRAIN, FIGHT_CH_SURGE, FIGHT_CH_MAX };
static s16 sFightMusicSent[FIGHT_CH_MAX];
// the landing (TP's): at Link's feet a window opens now and then, shorter the bigger the fish; A in it lands it
static u8 sLandPhase;
static s16 sLandTimeout;
static s16 sLandWait;
static s16 sLandWindow;
// a leap: seen, and the rod dropped for it
static u8 sLeapSeen;
static u8 sLeapDipped;
// the catch's hero shot: the camera's distance, easing back
static f32 sHeroDist;


static void Rod_Rumble(f32 scale, f32 strength, f32 duration, s32 decay, s32 joltFrames) {
    if (scale <= 0.0f) {
        return;
    }
    strength *= scale;
    if (scale > 1.0f) {
        duration *= scale; // (past full strength, longer)
    }
    Rumble_Override(0.0f, (u8)CLAMP(strength, 1.0f, 255.0f), (u8)CLAMP(duration, 1.0f, 255.0f), decay);
    if (joltFrames > sRumbleJolt) {
        sRumbleJolt = joltFrames;
    }
}

void EnFishmod_RodNibble(EnFishmod* fish) {
    if (ROD->state != ROD_STATE_FLOAT) {
        return;
    }
    ROD->fish = fish;
    ROD->dip = 4.0f; // quick tick down
    ROD->dipTarget = 0.0f;
    Sfx_PlaySfxAtPos(&gFishmodRod->projectedPos, NA_SE_EV_LURE_MOVE_W);
    Rod_Rumble(FEEL(FEEL_RUMBLE_NIBBLE), 255.0f, 3.0f, 85, 2); // a sharp tap in the hand for each peck
}

void EnFishmod_RodBite(EnFishmod* fish) {
    if (ROD->state != ROD_STATE_FLOAT) {
        return;
    }
    ROD->fish = fish;
    ROD->biting = true;
    ROD->dipTarget = 14.0f;
    ROD->biteTimer = (s16)(fish->species->behavior.biteWindow * FEEL(FEEL_BITE_WINDOW)) + 4; // + the bobber going down
    Sfx_PlaySfxAtPos(&gFishmodRod->projectedPos, NA_SE_EV_DIVE_WATER);
    Rod_Rumble(FEEL(FEEL_RUMBLE_BITE), 255.0f, 18.0f, 20, 10); // it takes it: a hard jolt
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
    // placed now: a new sub camera sits at the world's origin, and Rod_CameraUpdate first runs next frame (the cast
    // starts it from the windup), so this frame was drawn from (0, 0, 0): a one-frame flash of sky at a wrong angle
    Play_SetCameraAtEye(play, ROD->camId, &ROD->camAt, &ROD->camEye);
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
        Math_ApproachF(&sHeroDist, 95.0f, 0.04f, 1.0f); // (the hero shot: close, then easing back)
        ROD->camEye.x = player->actor.world.pos.x + Math_SinS(eyeYaw) * sHeroDist;
        ROD->camEye.z = player->actor.world.pos.z + Math_CosS(eyeYaw) * sHeroDist;
        Math_ApproachF(&ROD->camEye.y, player->actor.world.pos.y + height + 10.0f, 0.2f, 10.0f);
        at = player->actor.world.pos;
        at.y += height;
        Math_ApproachF(&ROD->camAt.x, at.x, 0.25f, 20.0f);
        Math_ApproachF(&ROD->camAt.y, at.y, 0.25f, 20.0f);
        Math_ApproachF(&ROD->camAt.z, at.z, 0.25f, 20.0f);
        Play_SetCameraAtEye(play, ROD->camId, &ROD->camAt, &ROD->camEye);
        return;
    }

    // under the water after the tackle (the sinking lure, the sinker, any bait down deep), as the Fishing Pond's
    // camera follows its lure down: behind the bait from the rod's side, a little above it, inside the water
    if (ROD->state == ROD_STATE_FLOAT) {
        if (!ROD->underCam && (ROD->baitDepth > UNDER_CAM_IN)) {
            ROD->underCam = true;
            ROD->camBlend = 0.2f;
        } else if (ROD->underCam && (ROD->baitDepth < UNDER_CAM_OUT)) {
            ROD->underCam = false;
            ROD->camBlend = 0.2f;
        }
    } else {
        ROD->underCam = false;
    }
    if (ROD->underCam) {
        static Vec3f sUnderFocus;
        f32 dx = ROD->tipPos.x - ROD->bobberPos.x;
        f32 dz = ROD->tipPos.z - ROD->bobberPos.z;
        f32 len = CLAMP_MIN(sqrtf(SQ(dx) + SQ(dz)), 1.0f);
        f32 eyeSurface = ROD->waterY;
        f32 eyeFloor = ROD->floorY;
        s32 k;

        sUnderFocus.x = ROD->bobberPos.x;
        sUnderFocus.y = FISHMOD_ROD_BAIT_Y(ROD);
        sUnderFocus.z = ROD->bobberPos.z;
        dx /= len;
        dz /= len;
        for (k = 0; k < 3; k++) {
            // back toward the rod, nearer if that's out of the water (a bank, the shallows)
            f32 back = 55.0f - k * 15.0f;

            eyeTarget.x = sUnderFocus.x + dx * back + dz * 12.0f;
            eyeTarget.z = sUnderFocus.z + dz * back - dx * 12.0f;
            if (EnFishmod_WaterFloor(play, eyeTarget.x, eyeTarget.z, &eyeSurface, &eyeFloor, false) &&
                (eyeSurface - eyeFloor > 30.0f)) {
                break;
            }
        }
        eyeTarget.y = CLAMP(sUnderFocus.y + 12.0f, eyeFloor + 10.0f, ROD->waterY - 10.0f);
        focus = &sUnderFocus;
        zoom = 0.0f;
    } else {
        // zooming moves the eye along the line toward the focus
        zoom = (ROD->state == ROD_STATE_FIGHT) ? 0.0f : sZoomFactors[ROD->zoom];
    }
    if ((ROD->state == ROD_STATE_FLOAT) && ROD->biting && !ROD->underCam) {
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
    if ((ROD->state == ROD_STATE_FLOAT) && !ROD->underCam && (eyeTarget.y < ROD->waterY + 20.0f)) {
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
    ROD->rig = 0xFF; // (the save's rig taken up quietly the first frame the rod's held)
    this->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    EnFishmod_WaterGridAcquire(play);

    // the fish manager spawns the scene's fish (its roster), where there are any
    if (Fishmod_SceneHasFish(play)) {
        EnFishmod_ManagerEnsure(play);
    }
}

/** The area's music comes back after the fish-on music (it only restores if the fish-on music is playing). */
static void Rod_StopFightMusic(void) {
    func_800F5B58();
}

/** The line comes in without a catch (the rod put away, Link hurt, the rod gone): the fish swims off. */
static void Rod_DropLine(PlayState* play) {
    if (ROD->state == ROD_STATE_FIGHT) {
        Rod_StopFightMusic();
    }
    if (ROD->fish != NULL) {
        EnFishmod_FishSetState(ROD->fish, FISH_STATE_FLEE);
    }
    ROD->fish = NULL;
    ROD->biting = false;
    ROD->tension = 0.0f;
    Rod_CameraStop(play);
}

void EnFishmod_RodDestroy(EnFishmod* this, PlayState* play) {
    if (ROD->state >= ROD_STATE_FLYING) {
        Rod_DropLine(play);
    } else {
        Rod_CameraStop(play);
    }
    Fishmod_ResetRodInfo();
    EnFishmod_WaterGridRelease(play);
}

/** What the rest of the game sees of the rod (gFishmodRodInfo), every frame whatever the rod is doing. */
static void Rod_PublishInfo(void) {
    gFishmodRodInfo.state = ROD->state;
    gFishmodRodInfo.biting = ROD->biting;
    gFishmodRodInfo.nibbling = (ROD->fish != NULL) && (ROD->fish->fishState == FISH_STATE_NIBBLE);
    gFishmodRodInfo.tension = ROD->tension;
    gFishmodRodInfo.castPower = ROD->castPower;
    gFishmodRodInfo.choice = ROD->choice;
    gFishmodRodInfo.bait = ROD->bait;
    gFishmodRodInfo.landOpen = (ROD->state == ROD_STATE_FIGHT) && sLandPhase && (sLandWindow > 0);
    gFishmodRodInfo.leaping = (ROD->state == ROD_STATE_FIGHT) && (ROD->fish != NULL) && ROD->fish->jumping;
}

static void Rod_EndCast(PlayState* play, Player* player) {
    Rod_StopFightMusic();
    ROD->state = ROD_STATE_HELD;
    ROD->biting = false;
    ROD->fish = NULL;
    ROD->dip = ROD->dipTarget = 0.0f;
    ROD->tension = 0.0f;
    ROD->underCam = false;
    ROD->anchored = false;
    sLandPhase = false;
    Letterbox_SetSizeTarget(0); // (the catch's hero shot over)
    player->unk_860 = 0; // Link goes back to standing with the rod
    Rod_CameraStop(play);
}

/*
 * The fight's music in layers (the fight sequence's channels, fishmod/tools/gen_fight_seq.py), faded in and out with
 * the fight: the held roots and the tension line always; the bass ostinato and the drums with a run; a trilled
 * strain line as the line nears breaking; a pounding surge when the fish is spent or at Link's feet. Neither TP nor
 * Sega Bass Fishing moves its fight music with the fight: this does.
 */
static void Rod_FightMusicReset(void) {
    s32 i;

    for (i = 0; i < FIGHT_CH_MAX; i++) {
        sFightMusicSent[i] = -1;
    }
}

static void Rod_StartFight(PlayState* play, Player* player) {
    EnFishmod* fish = ROD->fish;

    ROD->state = ROD_STATE_FIGHT;
    ROD->biting = false;
    ROD->tension = 35.0f;
    ROD->overTimer = ROD->slackTimer = 0;
    ROD->hookedBait = ROD->bait;
    ROD->bait = FISHMOD_BAIT_NONE; // the fish took it
    EnFishmod_FishHook(fish);
    // its thrashing scares the fish round about
    EnFishmod_Disturb(play, &fish->actor.world.pos, 220.0f, 120);
    Rod_Rumble(FEEL(FEEL_RUMBLE_BITE), 255.0f, 24.0f, 12, 14); // the hook goes in: the hardest
    sRumbleRunning = sRumbleLeaping = false;
    Sfx_PlaySfxCentered(NA_SE_IT_SWORD_SWING_HARD);
    EnFishmod_RodMessage(1, "FISH ON!", NULL);
    Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
    func_800F5ACC(NA_BGM_FISHMOD_FIGHT); // fish-on music in place of the area's, until the fight ends
    Rod_FightMusicReset();
    sLandPhase = sLeapSeen = sLeapDipped = false;
}

static void Rod_LoseFish(PlayState* play, Player* player, const char* why) {
    EnFishmod* fish = ROD->fish;

    gFishmodRodInfo.lost++;
    Rod_StopFightMusic();
    sLandPhase = false;
    if (fish != NULL) {
        EnFishmod_FishSetState(fish, FISH_STATE_FLEE);
        EnFishmod_LearnBait(fish, ROD->hookedBait); // it got away with that one in its mouth
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
// What the fight's indicators show (Rod_DrawHud) and sound: which way to hold the stick (the angle the stick has when
// it's right against the fish's run, as Rod_UpdateFight reads the stick), whether it's held there, whether the fish is
// in a run, and how much it has left.
static s16 sPullAngle;
static u8 sOpposing;
static u8 sRunning;
static f32 sFishLeft;
// the fight's zones on the tension's 0-100 (Rod_UpdateFight's own numbers: the hook falls out under 6 held slack,
// the drag pays out over 85, the line parts at 100)
#define TENSION_SLACK 12.0f
#define TENSION_LOW 25.0f
#define TENSION_HIGH 70.0f
#define TENSION_DRAG FEEL(FEEL_DRAG_AT) // (the drag: include/fishmod_feel.h)

static void Rod_FightMusic(EnFishmod* fish) {
    s16 want[FIGHT_CH_MAX];
    s32 running = (fish != NULL) && (fish->stateTimer > 0);
    f32 left = (fish != NULL) ? fish->stamina / fish->staminaMax : 0.0f;
    f32 strain = CLAMP((ROD->tension - TENSION_HIGH) / (TENSION_DRAG - TENSION_HIGH), 0.0f, 1.0f);
    s32 i;

    if (Audio_GetActiveSeqId(SEQ_PLAYER_BGM_MAIN) != NA_BGM_FISHMOD_FIGHT) {
        Rod_FightMusicReset(); // (not playing yet, or the area's again: its channels aren't ours to fade)
        return;
    }
    want[FIGHT_CH_BASS] = running ? 127 : 64;
    want[FIGHT_CH_ROOT] = 127;
    want[FIGHT_CH_PAD] = 110;
    want[FIGHT_CH_DRUMS] = running ? 127 : (ROD->tension > TENSION_HIGH) ? 90 : 24;
    want[FIGHT_CH_STRAIN] = (s16)(strain * 127.0f);
    want[FIGHT_CH_SURGE] = (sLandPhase || (left < 0.3f)) ? 127 : 0;
    for (i = 0; i < FIGHT_CH_MAX; i++) {
        if ((sFightMusicSent[i] < 0) || (ABS(want[i] - sFightMusicSent[i]) > 10) ||
            ((want[i] == 0) != (sFightMusicSent[i] == 0))) {
            SEQCMD_SET_CHANNEL_VOLUME(SEQ_PLAYER_BGM_MAIN, i, (sFightMusicSent[i] < 0) ? 1 : 10, want[i]);
            sFightMusicSent[i] = want[i];
        }
    }
}

static void Rod_Land(PlayState* play, Player* player, EnFishmod* fish);

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
    f32 pounds = fish->species->weightCoef * fish->lengthInches * fish->lengthInches * fish->lengthInches * 0.000001f;
    f32 load = CLAMP(sqrtf(pounds * FEEL(FEEL_LOAD_PER_LB)), FEEL(FEEL_LOAD_MIN), FEEL(FEEL_LOAD_MAX));

    // reel
    if (CHECK_BTN_ALL(input->cur.button, BTN_A)) {
        // a light fish can't hold against the reel: the lighter, the more line each turn takes in (up to reel_light
        // times at the lightest load, none at a load of 1, a 2 lb fish), so a small one comes in without fast-reeling
        // to keep its line from going slack. The tension that reeling adds stays the reel's own.
        f32 light = CLAMP((1.0f - load) / MAX(1.0f - FEEL(FEEL_LOAD_MIN), 0.01f), 0.0f, 1.0f);

        reel = CHECK_BTN_ALL(input->cur.button, BTN_R) ? FEEL(FEEL_REEL_FAST) : FEEL(FEEL_REEL_SPEED);
        ROD->lineOut -= reel * (1.0f + (FEEL(FEEL_REEL_LIGHT) - 1.0f) * light);
        Sfx_PlaySfxCentered(CHECK_BTN_ALL(input->cur.button, BTN_R) ? NA_SE_IT_FISHING_REEL_HIGH - SFX_FLAG
                                                                       : NA_SE_IT_FISHING_REEL_SLOW - SFX_FLAG);
    }

    // stick against the run: the stick is camera-relative
    if (stickMag > 40.0f) {
        Camera* cam = GET_ACTIVE_CAM(play);

        stickYaw = Math_Atan2S(input->rel.stick_y, -input->rel.stick_x) + Camera_GetInputDirYaw(cam) - 0x4000;
        opposing = ABS((s16)(stickYaw - fish->runYaw - 0x8000)) < 0x3000;
    }
    // the indicators: the stick's angle that makes the test above come out right in the middle, and the cues
    {
        s32 running = (fish->stateTimer > 0);

        sPullAngle = (s16)(fish->runYaw + 0x8000 + 0x4000 - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play)));
        if (running && !sRunning && !fish->jumping) {
            // it's off on a run: the command, as Sega Bass Fishing calls it (which way to hold the rod against it:
            // the stick's way that the arrow shows)
            static const char* sCommand[] = { "! Pull right !", "! Pull left !", "! Push forward !", "! Pull back !" };
            f32 sx = -Math_CosS(sPullAngle);
            f32 sy = Math_SinS(sPullAngle);

            Sfx_PlaySfxCentered(NA_SE_SY_ATTENTION_ON);
            EnFishmod_RodMessage(2, sCommand[(fabsf(sx) > fabsf(sy)) ? ((sx > 0.0f) ? 0 : 1) : ((sy > 0.0f) ? 2 : 3)],
                                 "against the run");
            ROD->messageTimer = 30;
        }
        if (opposing && !sOpposing) {
            Sfx_PlaySfxCentered(NA_SE_SY_LOCK_ON); // the stick is against it
        }
        sRunning = running;
        sOpposing = opposing;
        sFishLeft = staminaFrac;
    }

    // a leap (TP's): the rod down (the stick down) while it's in the air, or at the top of the leap it may throw the
    // hook, likelier the tighter the line; done right, the line eases
    if (fish->jumping) {
        if (!sLeapSeen) {
            sLeapSeen = true;
            sLeapDipped = false;
            EnFishmod_RodMessage(2, "It's leaping!", "Rod down! (stick down)");
            ROD->messageTimer = 25;
        }
        if (input->rel.stick_y < -40) {
            if (!sLeapDipped) {
                Sfx_PlaySfxCentered(NA_SE_SY_CURSOR);
            }
            sLeapDipped = true;
        }
        if ((fish->leapVelPrev > 0.0f) && (fish->vel.y <= 0.0f)) {
            if (sLeapDipped) {
                ROD->tension *= 0.5f;
            } else if (Fishmod_RandZeroOne(gFishmodRngNow) <
                       FEEL(FEEL_LEAP_THROW) * (0.5f + 0.5f * CLAMP(ROD->tension / 100.0f, 0.0f, 1.0f))) {
                Rod_LoseFish(play, player, "It threw the hook!");
                return;
            }
        }
    } else {
        sLeapSeen = false;
    }

    // where the fish wants to go
    // Bigger and stronger fish pull harder, a fresh fish harder than a tired one, and a run (stateTimer) much
    // harder: faster than the reel, so the line goes tight and the drag pays out.
    pullSpeed = (0.7f + b->strength * 0.02f + fish->lengthInches * 0.02f) * (0.35f + staminaFrac * 0.65f);
    if (fish->stateTimer > 0) {
        pullSpeed *= FEEL(FEEL_RUN_PULL);
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
        // The strain is this fish's own: its weight (the species' formula on its length) sets how much of its
        // strength and of the tight line reaches the rod. `load` is 1 at 2 lbs, 0.35 for a few ounces, 1.8 from
        // 6.5 lbs up: a minnow sits low on the gauge and comes straight in, a big pike has to be played. (It was
        // 30 + 8 a unit of tight line + the species' strength, whatever the fish's size: a hair trigger.)
        // Balanced on Lake Hylia's fish (species.json's sizes): a typical one, fresh, on a tight line sits between
        // 23 (greengill) and 66 (hearty bass) with the reel still, in the green or just under it, and the reel adds
        // 22; the biggest of the heavy species sit at 72 and go over the drag's 85 if reeled against; a run adds up
        // to 18 more, which holding the stick against it takes back.
        // (the numbers: include/fishmod_feel.h, tuned live from the desktop in test builds)
        tensionTarget = FEEL(FEEL_TENSION_BASE) + CLAMP_MAX(taut, FEEL(FEEL_TAUT_CAP)) * FEEL(FEEL_TAUT_GAIN) * load +
                        b->strength * FEEL(FEEL_STRENGTH_GAIN) * (0.3f + staminaFrac * 0.7f) * load;
        if (fish->stateTimer > 0) {
            tensionTarget += FEEL(FEEL_RUN_ADD) * load; // (a run is felt on the line)
        }
        EnFishmod_FishTire(fish, (0.4f + b->strength * 0.01f) * (opposing ? FEEL(FEEL_TIRE_OPPOSE) : 1.0f));
        if (opposing) {
            tensionTarget -= FEEL(FEEL_OPPOSE_RELIEF);
        }
    }
    if (reel > 0.0f) {
        tensionTarget += (taut > -6.0f) ? reel * FEEL(FEEL_REEL_GAIN) : 0.0f;
        if (fish->jumping) {
            tensionTarget += 30.0f; // reeling while it leaps: very likely to tear the hook out
        }
    }

    Math_ApproachF(&ROD->tension, tensionTarget, FEEL(FEEL_GAUGE_RATE), FEEL(FEEL_GAUGE_STEP)); // (was 0.3, 6: too sudden)

    // reel drag pays out line instead of snapping right away
    if ((ROD->tension > FEEL(FEEL_DRAG_AT)) && (taut > 0.0f)) {
        ROD->lineOut += pullSpeed * 0.8f; // the fish takes line, but against the drag
        Sfx_PlaySfxCentered(NA_SE_IT_FISHING_REEL_HIGH - SFX_FLAG);
    }

    ROD->lineOut = CLAMP(ROD->lineOut, LINE_MIN, LINE_MAX);

    EnFishmod_FishPullTo(fish, &next);

    // heard as well as seen: a count that quickens as the line nears parting, a slow tick while it hangs slack
    if (ROD->tension > TENSION_DRAG) {
        if ((play->gameplayFrames % ((ROD->tension >= 100.0f) ? 3 : 6)) == 0) {
            Sfx_PlaySfxCentered(NA_SE_SY_WARNING_COUNT_E);
        }
    } else if (ROD->tension > TENSION_HIGH) {
        if ((play->gameplayFrames % 14) == 0) {
            Sfx_PlaySfxCentered(NA_SE_SY_WARNING_COUNT_N);
        }
    } else if (ROD->tension < TENSION_SLACK) {
        if ((play->gameplayFrames % 12) == 0) {
            Sfx_PlaySfxCentered(NA_SE_SY_METRONOME);
        }
    }

    // snap: only a fish heavy enough breaks the line (a small one can't: the drag gives it line)
    if ((ROD->tension >= 100.0f) && (pounds < FEEL(FEEL_SNAP_LBS))) {
        ROD->tension = 99.0f;
        ROD->overTimer = 0;
    } else if (ROD->tension >= 100.0f) {
        if (++ROD->overTimer > (s16)FEEL(FEEL_SNAP_FRAMES)) {
            Rod_Rumble(FEEL(FEEL_RUMBLE_BITE), 255.0f, 30.0f, 10, 15);
            Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
            Rod_LoseFish(play, player, "The line snapped!");
            return;
        }
    } else {
        ROD->overTimer = 0;
    }

    // slack: the hook can fall out
    if (ROD->tension < 6.0f) {
        if (++ROD->slackTimer > (s16)FEEL(FEEL_SLACK_FRAMES)) {
            Rod_LoseFish(play, player, "It shook the hook...");
            return;
        }
    } else {
        ROD->slackTimer = 0;
    }

    // felt through the line: a jolt as a run starts or the fish leaps, else a steady rumble that follows the tension
    // (renewed each frame, fading within a few retraces when it stops: none on a slack line), a strong buzz while the
    // drag pays out
    if ((fish->stateTimer > 0) && !sRumbleRunning) {
        Rod_Rumble(FEEL(FEEL_RUMBLE_FIGHT), 255.0f, 9.0f, 40, 4);
    }
    if (fish->jumping && !sRumbleLeaping) {
        Rod_Rumble(FEEL(FEEL_RUMBLE_FIGHT), 255.0f, 6.0f, 60, 3);
    }
    sRumbleRunning = (fish->stateTimer > 0);
    sRumbleLeaping = fish->jumping;
    if (sRumbleJolt > 0) {
        sRumbleJolt--;
    } else if ((ROD->tension > FEEL(FEEL_DRAG_AT)) && (taut > 0.0f)) {
        Rod_Rumble(FEEL(FEEL_RUMBLE_FIGHT), 235.0f, 3.0f, 60, 0);
    } else if (ROD->tension >= TENSION_SLACK) {
        // (pulsing with its tail beat, as TP's rumble does)
        Rod_Rumble(FEEL(FEEL_RUMBLE_FIGHT),
                   (40.0f + ROD->tension * 1.6f + (sRumbleRunning ? 50.0f : 0.0f)) *
                       (0.65f + 0.35f * Math_SinS(fish->timer * 0x2400)),
                   3.0f, 60, 0);
    }

    Rod_FightMusic(fish);

    // at Link's feet (or dragged up the bank): the landing (TP's). A window opens now and then, shorter the bigger the
    // fish (half a second for the biggest); A in it lands it, A outside it puts the next one off (no mashing); too
    // long and it slips away.
    if ((ROD->lineOut <= LINE_MIN + 2.0f) || fish->beached ||
        (Math_Vec3f_DistXZ(&player->actor.world.pos, &fish->actor.world.pos) < 32.0f)) {
        ROD->lineOut = CLAMP_MAX(ROD->lineOut, LINE_MIN + 2.0f);
        if (!sLandPhase) {
            sLandPhase = true;
            sLandTimeout = 140;
            sLandWait = 8;
            sLandWindow = 0;
            EnFishmod_RodMessage(1, "Land it!", "A when it shows");
        }
        if (--sLandTimeout <= 0) {
            Rod_LoseFish(play, player, "It slipped away!");
            return;
        }
        if (sLandWindow > 0) {
            if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
                Rod_Land(play, player, fish);
                return;
            }
            if (--sLandWindow == 0) {
                sLandWait = 30 + (s16)Fishmod_RandZeroFloat(gFishmodRngNow, 25.0f);
            }
        } else {
            if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
                sLandWait += 30; // (too soon: the next one waits)
                Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
            }
            if (--sLandWait <= 0) {
                f32 big = CLAMP((load - FEEL(FEEL_LOAD_MIN)) / (FEEL(FEEL_LOAD_MAX) - FEEL(FEEL_LOAD_MIN)), 0.0f, 1.0f);

                sLandWindow = (s16)((20.0f - 12.0f * big) * FEEL(FEEL_LAND_WINDOW));
                EnFishmod_RodMessage(1, "A: LAND!", NULL);
                ROD->messageTimer = sLandWindow;
                Sfx_PlaySfxCentered(NA_SE_SY_FSEL_CURSOR);
                Rod_Rumble(FEEL(FEEL_RUMBLE_FIGHT), 200.0f, 3.0f, 80, 2);
            }
        }
    } else {
        sLandPhase = false;
    }
}

/** Landed: Link holds it up; the hero shot (letterboxed, the camera easing back), its size, the jingle (a record's
 * the bigger one). */
static void Rod_Land(PlayState* play, Player* player, EnFishmod* fish) {
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
    Rod_Rumble(FEEL(FEEL_RUMBLE_FIGHT), 200.0f, 12.0f, 15, 6); // landed
    Sfx_PlaySfxCentered(NA_SE_EV_JUMP_OUT_WATER);
    Rod_StopFightMusic();
    sLandPhase = false;
    sCatchRecord = Fishmod_RecordCatch(fish->species - gFishmodSpecies, fish->lengthInches);
    // (a record gets the big fanfare, any other the small one)
    Audio_PlayFanfare(sCatchRecord ? (NA_BGM_ITEM_GET | 0x900) : NA_BGM_SMALL_ITEM_GET);
    Letterbox_SetSizeTarget(30);
    sHeroDist = 55.0f;
    if (weight < 1.0f) {
        sprintf(sCatchSize, "%d.%d in, %d oz", (s32)fish->lengthInches, (s32)(fish->lengthInches * 10.0f) % 10,
                (s32)(weight * 16.0f + 0.5f));
    } else {
        sprintf(sCatchSize, "%d.%d in, %d.%d lbs", (s32)fish->lengthInches, (s32)(fish->lengthInches * 10.0f) % 10,
                (s32)weight, (s32)(weight * 10.0f) % 10);
    }
}

/**
 * The float depth, set by the water and not by the player: the bait hangs BAIT_DEPTH_FRACTION of the way down the
 * water under the bobber, where most species hold (their depth bands, fishmod/data/species.json: six of the ten
 * include it), no deeper than a float rig reaches (BAIT_DEPTH_MAX) and never within BAIT_ABOVE_BOTTOM of the bed. It
 * sinks to that after the cast and follows the bed as the bobber is reeled in, so it never drags in the shallows. The
 * bottom species (catfish, loach) and deep basins are for another rig (a sinker, a sinking lure), not for this one.
 * (It used to sink until A was pressed, once a cast: test 5's "needs a more intuitive control".)
 */
static void Rod_UpdateSink(PlayState* play, Input* input) {
    f32 surface;
    f32 floor;
    f32 want;

    // (the water under the bobber: every 4th frame, and at once after a cast)
    if (ROD->sinking || ((play->gameplayFrames & 3) == 0)) {
        want = ROD->baitTarget;
        if (EnFishmod_WaterFloor(play, ROD->bobberPos.x, ROD->bobberPos.z, &surface, &floor, false)) {
            f32 depth = surface - floor;

            want = CLAMP(depth * BAIT_DEPTH_FRACTION, BAIT_DEPTH_START, BAIT_DEPTH_MAX);
            want = CLAMP_MAX(want, depth - BAIT_ABOVE_BOTTOM);
            want = CLAMP_MIN(want, 2.0f);
        }
        ROD->baitTarget = want;
    }
    if (ROD->sinking) {
        // sinking faster as the line pays out, like a weighted bait
        ROD->baitDepth += CLAMP(0.8f + ROD->baitDepth * 0.012f, 0.8f, 3.0f);
        if (ROD->baitDepth >= ROD->baitTarget) {
            ROD->baitDepth = ROD->baitTarget;
            ROD->sinking = false;
            ROD->dip = 3.0f; // the bobber stands up as the line comes tight
            Rod_Rumble(FEEL(FEEL_RUMBLE_NIBBLE), 200.0f, 2.0f, 100, 1); // the line comes tight: a small tick
        }
    } else {
        // reeled in over shallower water (or out over deeper): the bait eases to its new depth
        Math_ApproachF(&ROD->baitDepth, ROD->baitTarget, 0.2f, 2.0f);
    }
}

/*
 * The other rigs' depths (FishmodRig): baitDepth stays the bait's depth under the surface, where the fish go for it,
 * whatever's on the line; bobberPos the tackle itself (Rod_UpdateFloat's end), where the line goes.
 *   pond lure      floats; reeled, it dives (deeper reeled fast) and a twitch ducks it; stopped, it rises again
 *   sinking lure   sinks to just over the bed; reeled, it lifts off it (more reeled fast), stopped, it sinks back
 *   sinker         the weight sinks to the bed and anchors there; the bait floats above it on its leader
 */
static void Rod_UpdateRigDepth(PlayState* play, Input* input) {
    f32 surface;
    f32 floor;
    f32 depth;
    f32 want;
    f32 rate;

    if (!EnFishmod_WaterFloor(play, ROD->bobberPos.x, ROD->bobberPos.z, &surface, &floor, false)) {
        floor = ROD->waterY - 30.0f;
    }
    ROD->floorY = floor;
    depth = CLAMP_MIN(ROD->waterY - floor, 2.0f);
    switch (ROD->rig) {
        case FISHMOD_RIG_LURE_POND:
            want = (ROD->reeling == 2) ? POND_DIVE_FAST : (ROD->reeling == 1) ? POND_DIVE_SLOW : 0.0f;
            if (ROD->twitchTimer > 12) {
                want = CLAMP_MIN(want, 12.0f);
            }
            want = CLAMP_MAX(want, depth - 6.0f);
            rate = (want > ROD->baitDepth) ? 1.6f : 0.7f; // (dives quickly, floats up slowly)
            break;
        case FISHMOD_RIG_LURE_SINKING: {
            f32 lift = (ROD->reeling == 2) ? SINK_LIFT_FAST : (ROD->reeling == 1) ? SINK_LIFT_SLOW : 0.0f;

            if (ROD->twitchTimer > 10) {
                lift = CLAMP_MIN(lift, 14.0f);
            }
            want = depth - 3.0f - lift;
            rate = (want > ROD->baitDepth) ? SINK_RATE : 2.0f;
            break;
        }
        default: // the sinker
            want = depth - 1.0f - SINKER_LEADER;
            rate = SINKER_RATE;
            break;
    }
    want = CLAMP(want, 0.0f, CLAMP_MIN(depth - 2.0f, 0.0f));
    Math_StepToF(&ROD->baitDepth, want, rate);
    ROD->sinking = (ROD->rig != FISHMOD_RIG_LURE_POND) && (ROD->baitDepth < want - 1.0f);
    // reaching the bed: a tick through the line (the sinker anchors; the lure settles)
    if ((ROD->rig != FISHMOD_RIG_LURE_POND) && !ROD->anchored && !ROD->sinking && (ROD->baitDepth > 4.0f)) {
        ROD->anchored = true;
        ROD->dip = 3.0f;
        Rod_Rumble(FEEL(FEEL_RUMBLE_NIBBLE), 200.0f, 2.0f, 100, 1);
    }
}

/** Where the sinker is (the bait's on its leader above it): on the bed, or on its way down under the bait. */
static f32 Rod_SinkerY(void) {
    return CLAMP_MIN(ROD->waterY - ROD->baitDepth - SINKER_LEADER, ROD->floorY + 1.0f);
}

static void Rod_UpdateFloat(EnFishmod* this, PlayState* play, Player* player, Input* input) {
    f32 waterY;
    f32 groundY;
    f32 dist;
    Vec3f toTip;

    if (ROD->rig == FISHMOD_RIG_BOBBER) {
        Rod_UpdateSink(play, input);
    } else {
        Rod_UpdateRigDepth(play, input);
    }

    // the bait's action, for the fish: reeled (slowly or fast), how long since it last moved, twitches lately
    ROD->reeling = !CHECK_BTN_ALL(input->cur.button, BTN_A) ? 0 : CHECK_BTN_ALL(input->cur.button, BTN_R) ? 2 : 1;
    if (ROD->reeling != 0) {
        ROD->stillFrames = 0;
    } else if (ROD->stillFrames < 0x7FFF) {
        ROD->stillFrames++;
    }
    if ((ROD->twitchHeat > 0) && ((play->gameplayFrames % 40) == 0)) {
        ROD->twitchHeat--;
    }

    // reel
    if (CHECK_BTN_ALL(input->cur.button, BTN_A)) {
        f32 reel = CHECK_BTN_ALL(input->cur.button, BTN_R) ? 3.5f : 1.6f;

        ROD->lineOut -= reel;
        Sfx_PlaySfxCentered(CHECK_BTN_ALL(input->cur.button, BTN_R) ? NA_SE_IT_FISHING_REEL_HIGH - SFX_FLAG
                                                                       : NA_SE_IT_FISHING_REEL_SLOW - SFX_FLAG);
        if (((play->gameplayFrames % 6) == 0) && (ROD->rig == FISHMOD_RIG_BOBBER || ROD->baitDepth < 8.0f)) {
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
        ROD->stillFrames = 0;
        ROD->twitchHeat = MIN(ROD->twitchHeat + 1, 10);
        ROD->dip = 2.0f;
        ROD->lineOut -= 3.0f;
        Actor_PlaySfx(&this->actor, NA_SE_EV_LURE_MOVE_W);
        if ((ROD->rig == FISHMOD_RIG_BOBBER) || (ROD->baitDepth < 8.0f)) {
            Rod_Splash(play, &ROD->bobberPos, false);
        }
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
                EnFishmod_LearnBait(ROD->fish, ROD->bait); // it got the bait free (or felt the hook come late)
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
    switch (ROD->rig) {
        case FISHMOD_RIG_BOBBER:
            ROD->bobberPos.y = ROD->waterY - ROD->dip + Math_SinS(play->gameplayFrames * 0x500) * 0.6f;
            break;
        case FISHMOD_RIG_SINKER:
            // the weight (a tug at the bait shows as the bait pulled down toward it: Rod_DrawBobber)
            ROD->bobberPos.y = Rod_SinkerY();
            break;
        default:
            // the lure itself, where its bait is: a bite pulls it under (dip); at the surface it rides the ripples
            ROD->bobberPos.y = ROD->waterY - ROD->baitDepth - ROD->dip * 0.6f +
                               ((ROD->baitDepth < 1.0f) ? Math_SinS(play->gameplayFrames * 0x500) * 0.4f : 0.0f);
            break;
    }
    (void)groundY;
}

#if FISHMOD_TESTMODE
/*
 * Test mode, for trying the fishing's feel without waiting: L + D-pad Right.
 * - With the line out of the water: picks which fish to call (each species in turn, then "any": the nearest).
 * - With the bobber in the water and no fish at it: the nearest fish of that kind (in the scene's water, not caught)
 *   is put a few body lengths from the bobber at the bait's depth and comes to it. From there it's the fish's own:
 *   its nibbles at its species' rhythm, its bite, its fight.
 */
static s8 sTestFishSpecies = -1; // -1: any (the nearest)

static void Rod_TestCallFish(PlayState* play, Input* input) {
    Actor* a;
    EnFishmod* best = NULL;
    f32 bestDist = 1e9f;
    s32 i;

    s32 pressed = CHECK_BTN_ALL(input->cur.button, BTN_L) && CHECK_BTN_ALL(input->press.button, BTN_DRIGHT);

#if FISHMOD_DEBUG
    if (gFishmodTestCallFish) {
        gFishmodTestCallFish = 0;
        pressed = true;
    }
#endif
    if (!pressed) {
        return;
    }
    if (ROD->state != ROD_STATE_FLOAT) {
        for (i = 0; i < FISHMOD_SPECIES_MAX + 1; i++) {
            sTestFishSpecies++;
            if (sTestFishSpecies >= FISHMOD_SPECIES_MAX) {
                sTestFishSpecies = -1;
                break;
            }
            if (gFishmodSpecies[sTestFishSpecies].name != NULL) {
                break;
            }
        }
        EnFishmod_RodMessage(1, "Test: L+Right calls",
                             (sTestFishSpecies < 0) ? "the nearest fish" : gFishmodSpecies[sTestFishSpecies].name);
        Sfx_PlaySfxCentered(NA_SE_SY_CURSOR);
        return;
    }
    if (ROD->fish != NULL) {
        return;
    }
    for (a = play->actorCtx.actorLists[ACTORCAT_FISH].head; a != NULL; a = a->next) {
        EnFishmod* f = (EnFishmod*)a;
        f32 d;

        if ((a->id != ACTOR_EN_FISHMOD) || (f->species == NULL) ||
            ((sTestFishSpecies >= 0) && (f->species != &gFishmodSpecies[sTestFishSpecies])) ||
            ((f->fishState != FISH_STATE_WANDER) && (f->fishState != FISH_STATE_APPROACH))) {
            continue;
        }
        d = Math_Vec3f_DistXYZ(&a->world.pos, &ROD->bobberPos);
        if (d < bestDist) {
            bestDist = d;
            best = f;
        }
    }
    if (best == NULL) {
        EnFishmod_RodMessage(2, "Test: none of those here",
                             (sTestFishSpecies < 0) ? NULL : gFishmodSpecies[sTestFishSpecies].name);
        return;
    }
    {
        // a few body lengths out from the bobber, away from Link, at the bait's depth
        s16 yaw = Math_Vec3f_Yaw(&GET_PLAYER(play)->actor.world.pos, &ROD->bobberPos);
        f32 r = 30.0f + best->halfLength * 4.0f;

        best->actor.world.pos.x = ROD->bobberPos.x + Math_SinS(yaw) * r;
        best->actor.world.pos.z = ROD->bobberPos.z + Math_CosS(yaw) * r;
        best->actor.world.pos.y = FISHMOD_ROD_BAIT_Y(ROD) - 4.0f;
        best->actor.prevPos = best->actor.world.pos;
        best->actor.shape.rot.y = best->actor.world.rot.y = yaw + 0x8000;
    }
    EnFishmod_FishSetState(best, FISH_STATE_APPROACH);
    best->stateTimer = 400;
    EnFishmod_RodMessage(1, "Test: here comes", best->species->name);
}
#endif

void EnFishmod_RodUpdate(EnFishmod* this, PlayState* play) {
    sRodScene = play->sceneId;
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
#if FISHMOD_TESTMODE
    if (holding) {
        Rod_TestCallFish(play, input);
    }
#endif
#if FISHMOD_DEBUG
    gFishmodDebugRod = ROD; // (where the rod's state is, for the emulator: the overlay moves)
#endif


    // stop everything if the rod was put away (or Link got hit, entered water...)
    if (!holding || (player->stateFlags1 & (PLAYER_STATE1_27 | PLAYER_STATE1_DEAD))) {
        if (ROD->state >= ROD_STATE_FLYING) {
            Rod_DropLine(play);
        }
        ROD->state = holding ? ROD_STATE_HELD : ROD_STATE_NONE;
        if (!holding) {
            // put away: the bait stays on the hook, but a bottle's request for it is dropped
            gFishmodRodInfo.baitRequest = FISHMOD_BAIT_NONE;
            // where there are no fish the controller only lives while the rod is out (it frees its memory)
            if (!Fishmod_SceneHasFish(play)) {
                Actor_Kill(&this->actor);
            }
        }
        Rod_PublishInfo();
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
            // the tackle (Fishmod_UseItem changes the save's rig): a lure is its own bait; off, the hook's bare
            if (FISHMOD_RIG() != ROD->rig) {
                s32 quiet = (ROD->rig == 0xFF);

                ROD->rig = FISHMOD_RIG();
                if (ROD->rig == FISHMOD_RIG_LURE_POND) {
                    ROD->bait = FISHMOD_BAIT_LURE_POND;
                } else if (ROD->rig == FISHMOD_RIG_LURE_SINKING) {
                    ROD->bait = FISHMOD_BAIT_LURE_SINKING;
                } else if (ROD->bait >= FISHMOD_BAIT_LURE_POND) {
                    ROD->bait = FISHMOD_BAIT_NONE;
                }
                if (!quiet) {
                    static const char* sRigNames[] = { "Bobber on", "Pond lure on", "Sinking lure on", "Sinker on" };
                    static const char* sRigHints[] = {
                        "Bait it from a bottle",
                        "Reel to dive it, stop to let it rise",
                        "It sinks to the bottom: reel to hop it",
                        "It rests on the bed, the bait above it",
                    };

                    EnFishmod_RodMessage(1, sRigNames[ROD->rig], sRigHints[ROD->rig]);
                }
            }
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
            static u16 sItemButtons[] = { BTN_B,   BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT,
                                          BTN_DUP, BTN_DDOWN, BTN_DLEFT, BTN_DRIGHT };
            u16 rodButton = sItemButtons[CLAMP(player->heldItemButton, 0, FISHMOD_BUTTON_MAX - 1)];
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
                ROD->reeling = ROD->twitchHeat = 0;
                ROD->stillFrames = 0x7FFF; // (still since it landed: no stop-and-go yet)
                EnFishmod_Disturb(play, &ROD->bobberPos, 0.0f, 0);
                // the bait sinks under the bobber to the depth the water gives it (Rod_UpdateSink)
                ROD->baitDepth = (ROD->rig == FISHMOD_RIG_BOBBER) ? BAIT_DEPTH_START : 0.0f;
                ROD->baitTarget = ROD->baitDepth;
                ROD->sinking = true;
                ROD->anchored = false;
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

                // the bobber's bait partway down (all the way would lose the bobber); the other rigs' tackle itself
                focus.y = (ROD->rig == FISHMOD_RIG_BOBBER) ? (focus.y - ROD->baitDepth * 0.6f) : FISHMOD_ROD_BAIT_Y(ROD);
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
                        EnFishmod_FishLetGo(fish, &spot);
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

    Rod_PublishInfo();

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
    gSPDisplayList(POLY_OPA_DISP++, gFishmodRodMaterialDL);
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
                    gDPLoadTextureBlock(POLY_OPA_DISP++, gFishmodRodSegmentBlackTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16,
                                        8, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD,
                                        G_TX_NOLOD);
                } else if ((k < 8) || ((k % 2) == 0)) {
                    gDPLoadTextureBlock(POLY_OPA_DISP++, gFishmodRodSegmentWhiteTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16,
                                        8, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD,
                                        G_TX_NOLOD);
                } else {
                    gDPLoadTextureBlock(POLY_OPA_DISP++, gFishmodRodSegmentStripTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16,
                                        8, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD,
                                        G_TX_NOLOD);
                }
                gSPDisplayList(POLY_OPA_DISP++, gFishmodRodSegmentDL);
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
            gDPLoadTextureBlock(POLY_OPA_DISP++, gFishmodRodSegmentBlackTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 8, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD, G_TX_NOLOD);
        } else if ((i < 8) || ((i % 2) == 0)) {
            gDPLoadTextureBlock(POLY_OPA_DISP++, gFishmodRodSegmentWhiteTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 8, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD, G_TX_NOLOD);
        } else {
            gDPLoadTextureBlock(POLY_OPA_DISP++, gFishmodRodSegmentStripTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 8, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 4, 3, G_TX_NOLOD, G_TX_NOLOD);
        }
        gSPDisplayList(POLY_OPA_DISP++, gFishmodRodSegmentDL);

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
    gSPDisplayList(POLY_XLU_DISP++, gFishmodLineModelDL);

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
    } else if ((ROD->state == ROD_STATE_FLOAT) && (ROD->rig != FISHMOD_RIG_BOBBER)) {
        *sag *= 0.4f; // (down through the water to the tackle, near enough straight)
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
            if ((waterY > floorY) && !((ROD->state == ROD_STATE_FLOAT) && (ROD->rig != FISHMOD_RIG_BOBBER) &&
                                       (ROD->baitDepth > 2.0f))) {
                floorY = waterY + 0.3f; // a slack line floats (not one going down to a lure or a sinker)
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

    // the leader: from the bobber down to the hook, or from the sinker up to it
    if ((ROD->rig == FISHMOD_RIG_BOBBER) &&
        ((ROD->state == ROD_STATE_FLOAT) || (ROD->state == ROD_STATE_GROUND) || (ROD->state == ROD_STATE_FLYING))) {
        Vec3f hook = ROD->bobberPos;

        hook.y -= (ROD->state == ROD_STATE_FLOAT) ? ROD->baitDepth : 4.0f;
        Rod_DrawLineSegment(play, &ROD->bobberPos, &hook, 0.001f);
    } else if ((ROD->rig == FISHMOD_RIG_SINKER) && (ROD->state == ROD_STATE_FLOAT)) {
        Vec3f hook = ROD->bobberPos;

        hook.y = FISHMOD_ROD_BAIT_Y(ROD) - ROD->dip;
        Rod_DrawLineSegment(play, &ROD->bobberPos, &hook, 0.001f);
    }

    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
}

/**
 * A hook (the pond's: two plates crossed) hanging from pos (its eye), turned to yaw. scale: HOOK_LURE as the pond's
 * lures carry them; HOOK_BAIT for a baited hook, drawn larger than life so it reads (the mesh lies flat in its own
 * x-z plane, eye at z 27: turned to hang down).
 */
#define HOOK_LURE 0.004f
#define HOOK_BAIT 0.0095f
#define HOOK_DROP(scale) ((scale) * 586.0f * 1.25f) // eye to bend

static void Rod_DrawHook(PlayState* play, Vec3f* pos, f32 yaw, f32 scale) {
    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    Matrix_Translate(pos->x, pos->y, pos->z, MTXMODE_NEW);
    Matrix_RotateY(yaw, MTXMODE_APPLY);
    Matrix_RotateX(-M_PI / 2, MTXMODE_APPLY);
    Matrix_Scale(scale, scale, scale * 1.25f, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    gSPDisplayList(POLY_OPA_DISP++, gFishmodLureHookDL);
    Matrix_RotateZ(M_PI / 2, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    gSPDisplayList(POLY_OPA_DISP++, gFishmodLureHookDL);
    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
}

// a bead for the bait (a 12-point ball, lit from the upper left in its vertex colours, its colour the primitive's)
static Vtx sBeadVtx[] = {
    VTX(-53, 85, 0, 0, 0, 234, 234, 234, 255),  VTX(53, 85, 0, 0, 0, 156, 156, 156, 255),
    VTX(-53, -85, 0, 0, 0, 107, 107, 107, 255), VTX(53, -85, 0, 0, 0, 107, 107, 107, 255),
    VTX(0, -53, 85, 0, 0, 115, 115, 115, 255),  VTX(0, 53, 85, 0, 0, 225, 225, 225, 255),
    VTX(0, -53, -85, 0, 0, 107, 107, 107, 255), VTX(0, 53, -85, 0, 0, 107, 107, 107, 255),
    VTX(85, 0, -53, 0, 0, 107, 107, 107, 255),  VTX(85, 0, 53, 0, 0, 107, 107, 107, 255),
    VTX(-85, 0, -53, 0, 0, 131, 131, 131, 255), VTX(-85, 0, 53, 0, 0, 209, 209, 209, 255),
};

static Gfx sBeadSetupDL[] = {
    gsDPPipeSync(),
    gsSPTexture(0, 0, 0, G_TX_RENDERTILE, G_OFF),
    gsDPSetCombineLERP(SHADE, 0, PRIMITIVE, 0, 0, 0, 0, 1, 0, 0, 0, COMBINED, 0, 0, 0, COMBINED),
    gsSPClearGeometryMode(G_LIGHTING | G_CULL_BACK),
    gsSPEndDisplayList(),
};

static Gfx sBeadDL[] = {
    gsSPVertex(sBeadVtx, 12, 0),
    gsSP2Triangles(0, 11, 5, 0, 0, 5, 1, 0),
    gsSP2Triangles(0, 1, 7, 0, 0, 7, 10, 0),
    gsSP2Triangles(0, 10, 11, 0, 1, 5, 9, 0),
    gsSP2Triangles(5, 11, 4, 0, 11, 10, 2, 0),
    gsSP2Triangles(10, 7, 6, 0, 7, 1, 8, 0),
    gsSP2Triangles(3, 9, 4, 0, 3, 4, 2, 0),
    gsSP2Triangles(3, 2, 6, 0, 3, 6, 8, 0),
    gsSP2Triangles(3, 8, 9, 0, 4, 9, 5, 0),
    gsSP2Triangles(2, 4, 11, 0, 6, 2, 10, 0),
    gsSP2Triangles(8, 6, 7, 0, 9, 8, 1, 0),
    gsSPEndDisplayList(),
};

/*
 * The bait on the hook, drawn so it reads (somewhat larger than life, as the hook): beads in its shape and colour,
 * hung round the hook's bend. x across (turned to face the rod), y up, in units; r its radius.
 */
typedef struct BaitBead {
    f32 x, y, r;
} BaitBead;
static const BaitBead sWormBeads[] = { { 0.0f, 1.8f, 0.6f },  { 0.9f, 1.3f, 0.6f },   { 1.3f, 0.4f, 0.6f },
                                       { 1.1f, -0.5f, 0.6f }, { 0.4f, -1.1f, 0.6f },  { -0.4f, -1.3f, 0.55f },
                                       { -1.1f, -0.9f, 0.5f } };
static const BaitBead sLarvaBeads[] = { { 0.0f, 1.0f, 1.05f }, { 0.95f, 0.4f, 1.0f }, { 1.0f, -0.6f, 0.9f },
                                        { 0.25f, -1.15f, 0.7f } };
static const BaitBead sBugBeads[] = { { 0.0f, 1.2f, 0.55f }, { 0.0f, 0.25f, 0.7f }, { 0.0f, -0.9f, 0.95f } };

static void Rod_DrawBait(PlayState* play, Vec3f* at, f32 yaw) {
    const BaitBead* beads;
    s32 n;
    s32 i;
    u8 rgb[3][3]; // (a colour for each part: the bug's head, thorax and abdomen differ)
    f32 cy = cosf(yaw);
    f32 sy = sinf(yaw);

    switch (ROD->bait) {
        case FISHMOD_BAIT_WORM:
            beads = sWormBeads, n = ARRAY_COUNT(sWormBeads);
            rgb[0][0] = rgb[1][0] = rgb[2][0] = 255, rgb[0][1] = rgb[1][1] = rgb[2][1] = 140,
            rgb[0][2] = rgb[1][2] = rgb[2][2] = 150; // (a bright pink: the water's fog takes some of it)
            break;
        case FISHMOD_BAIT_BEE_LARVA:
            beads = sLarvaBeads, n = ARRAY_COUNT(sLarvaBeads);
            rgb[0][0] = rgb[1][0] = rgb[2][0] = 252, rgb[0][1] = rgb[1][1] = rgb[2][1] = 240,
            rgb[0][2] = rgb[1][2] = rgb[2][2] = 196;
            break;
        case FISHMOD_BAIT_BUG:
            beads = sBugBeads, n = ARRAY_COUNT(sBugBeads);
            rgb[0][0] = 60, rgb[0][1] = 44, rgb[0][2] = 30;
            rgb[1][0] = 84, rgb[1][1] = 110, rgb[1][2] = 46;
            rgb[2][0] = 52, rgb[2][1] = 78, rgb[2][2] = 36;
            break;
        default:
            return;
    }
    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sBeadSetupDL);
    for (i = 0; i < n; i++) {
        f32 x = beads[i].x * 1.4f;
        f32 y = beads[i].y * 1.4f;
        s32 part = (n == 3) ? i : 0;

        if (ROD->bait == FISHMOD_BAIT_WORM) {
            x += Math_SinS(play->gameplayFrames * 0x900 + i * 0x2400) * 0.35f; // (it wriggles)
        }
        Matrix_Translate(at->x + x * cy, at->y + y, at->z - x * sy, MTXMODE_NEW);
        Matrix_Scale(beads[i].r * 0.014f, beads[i].r * 0.014f, beads[i].r * 0.014f, MTXMODE_APPLY);
        MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, rgb[part][0], rgb[part][1], rgb[part][2], 255);
        gSPDisplayList(POLY_OPA_DISP++, sBeadDL);
    }
    gSPSetGeometryMode(POLY_OPA_DISP++, G_LIGHTING);
    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
}

/** The bobber's and the sinker's hook, its bait round the bend, at the bait's place (`bait`: where the fish go). */
static void Rod_DrawBaitedHook(PlayState* play, Vec3f* bait) {
    f32 yaw = Math_FAtan2F(ROD->tipPos.x - ROD->bobberPos.x, ROD->tipPos.z - ROD->bobberPos.z);
    Vec3f eye = *bait;

    eye.y += HOOK_DROP(HOOK_BAIT) * 0.75f; // (the bait round the bend, at the bait's place)
    Rod_DrawHook(play, &eye, yaw, HOOK_BAIT);
    Rod_DrawBait(play, bait, yaw + M_PI / 2);
}

/**
 * The pond's lure (the Fishing Pond's own model): nose toward the rod (it's drawn along the line), dipping its nose as
 * it's reeled under, wagging side to side while it swims; its two hooks hang under it as the pond's do.
 */
static void Rod_DrawPondLure(PlayState* play) {
    static f32 sPitch;
    Vec3f hookAt;
    Vec3f hookPos[2];
    s32 swimming = (ROD->state == ROD_STATE_FLOAT) && (ROD->reeling != 0);
    f32 yaw = Math_FAtan2F(ROD->tipPos.x - ROD->bobberPos.x, ROD->tipPos.z - ROD->bobberPos.z);
    s32 i;

    Math_ApproachF(&sPitch, swimming ? -0.5f : 0.0f, 0.2f, 0.1f);
    if (swimming) {
        yaw += Math_SinS(play->gameplayFrames * 0x2400) * ((ROD->reeling == 2) ? 0.35f : 0.22f);
    }
    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Translate(ROD->bobberPos.x, ROD->bobberPos.y, ROD->bobberPos.z, MTXMODE_NEW);
    Matrix_RotateY(yaw, MTXMODE_APPLY);
    Matrix_RotateX(sPitch, MTXMODE_APPLY);
    Matrix_Scale(0.004f, 0.004f, 0.004f, MTXMODE_APPLY);
    Matrix_RotateZ(M_PI / 2, MTXMODE_APPLY);
    Matrix_RotateY(M_PI / 2, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    gSPDisplayList(POLY_OPA_DISP++, gFishmodLureFloatDL);
    // (where the pond hangs its two hooks, in the lure's own space)
    hookAt.x = 500.0f;
    hookAt.y = 0.0f;
    hookAt.z = -300.0f;
    Matrix_MultVec3f(&hookAt, &hookPos[0]);
    hookAt.x = 2100.0f;
    hookAt.z = -50.0f;
    Matrix_MultVec3f(&hookAt, &hookPos[1]);
    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    for (i = 0; i < 2; i++) {
        Rod_DrawHook(play, &hookPos[i], yaw, HOOK_LURE);
    }
}

/**
 * The sinking lure (the Fishing Pond's): a soft body of segments, each drawn facing the camera, the tail following the
 * head 0.85 units behind each other and lifting a little in the water, as the pond's does.
 */
#define SINK_SEGS 20
static Vec3f sSinkSeg[SINK_SEGS];

static void Rod_DrawSinkingLure(PlayState* play) {
    static f32 sSizes[SINK_SEGS] = {
        1.0f, 1.5f, 1.8f, 2.0f, 1.8f, 1.6f, 1.4f, 1.2f, 1.0f, 1.0f,
        0.9f, 0.85f, 0.8f, 0.7f, 0.8f, 1.0f, 1.2f, 1.1f, 1.0f, 0.8f,
    };
    // the tail floats up a little under water (a soft lure), as the pond's does in use; it hangs down in the air
    f32 lift = (ROD->bobberPos.y >= ROD->waterY) ? -5.0f : 0.5f;
    s32 i;

    if (Math_Vec3f_DistXYZ(&sSinkSeg[0], &ROD->bobberPos) > 30.0f) {
        for (i = 0; i < SINK_SEGS; i++) {
            sSinkSeg[i] = ROD->bobberPos; // (cast anew: gathered at the head)
        }
    }
    sSinkSeg[0] = ROD->bobberPos;
    for (i = 1; i < SINK_SEGS; i++) {
        Vec3f d;
        f32 len;

        d.x = sSinkSeg[i].x - sSinkSeg[i - 1].x;
        d.y = sSinkSeg[i].y + lift - sSinkSeg[i - 1].y;
        d.z = sSinkSeg[i].z - sSinkSeg[i - 1].z;
        len = sqrtf(SQ(d.x) + SQ(d.y) + SQ(d.z));
        if (len < 0.01f) {
            d.x = 0.0f;
            d.y = 1.0f;
            d.z = 0.0f;
            len = 1.0f;
        }
        sSinkSeg[i].x = sSinkSeg[i - 1].x + d.x * (0.85f / len);
        sSinkSeg[i].y = sSinkSeg[i - 1].y + d.y * (0.85f / len);
        sSinkSeg[i].z = sSinkSeg[i - 1].z + d.z * (0.85f / len);
        if ((ROD->state == ROD_STATE_FLOAT) && (sSinkSeg[i].y < ROD->floorY + 0.5f)) {
            sSinkSeg[i].y = ROD->floorY + 0.5f; // (on the bed, not through it)
        }
    }

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    // (in the translucent pass, after the scenery: its soft-edged mode tests the depth but doesn't write it, so drawn
    // with the opaque things it was painted over by any plant drawn after it, however far behind)
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, gFishmodSinkingLureSegmentMaterialDL);
    for (i = SINK_SEGS - 1; i >= 0; i--) {
        Matrix_Translate(sSinkSeg[i].x, sSinkSeg[i].y, sSinkSeg[i].z, MTXMODE_NEW);
        Matrix_Scale(sSizes[i] * 0.04f, sSizes[i] * 0.04f, sSizes[i] * 0.04f, MTXMODE_APPLY);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
        gSPDisplayList(POLY_XLU_DISP++, gFishmodSinkingLureSegmentModelDL);
    }
    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    Rod_DrawHook(play, &sSinkSeg[0], Math_FAtan2F(ROD->tipPos.x - ROD->bobberPos.x, ROD->tipPos.z - ROD->bobberPos.z),
                 HOOK_LURE * 1.2f);
}

/** The sinker on the bed (or falling), and the baited hook up its leader. */
static void Rod_DrawSinker(PlayState* play) {
    Vec3f hook = ROD->bobberPos;

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Translate(ROD->bobberPos.x, ROD->bobberPos.y, ROD->bobberPos.z, MTXMODE_NEW);
    Matrix_Scale(0.03f, 0.03f, 0.03f, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    gSPDisplayList(POLY_OPA_DISP++, sSinkerDL);
    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    hook.y = (ROD->state == ROD_STATE_FLOAT) ? (FISHMOD_ROD_BAIT_Y(ROD) - ROD->dip) : (hook.y + 3.0f);
    Rod_DrawBaitedHook(play, &hook);
}

static void Rod_DrawBobber(PlayState* play) {
    f32 tilt;

    if (ROD->state == ROD_STATE_FIGHT || ROD->state == ROD_STATE_LANDING) {
        return; // it's on the fish
    }
    if (ROD->rig == FISHMOD_RIG_LURE_POND) {
        Rod_DrawPondLure(play);
        return;
    }
    if (ROD->rig == FISHMOD_RIG_LURE_SINKING) {
        Rod_DrawSinkingLure(play);
        return;
    }
    if (ROD->rig == FISHMOD_RIG_SINKER) {
        Rod_DrawSinker(play);
        return;
    }

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);

    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    tilt = (ROD->dip > 1.0f) ? ROD->dip * 0.04f : 0.0f;
    Matrix_Translate(ROD->bobberPos.x, ROD->bobberPos.y, ROD->bobberPos.z, MTXMODE_NEW);
    Matrix_RotateZ(tilt, MTXMODE_APPLY);
    Matrix_Scale(0.012f, 0.012f, 0.012f, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../fishmod_rod.c", __LINE__);
    gSPDisplayList(POLY_OPA_DISP++, sBobberDL);

    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rod.c", __LINE__);

    // the baited hook under the bobber
    if (ROD->state == ROD_STATE_FLOAT) {
        Vec3f hook = ROD->bobberPos;

        hook.y = FISHMOD_ROD_BAIT_Y(ROD) - ROD->dip;
        Rod_DrawBaitedHook(play, &hook);
    }
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
    s32 fight = (ROD->state == ROD_STATE_FIGHT);

#if FISHMOD_DEBUG
    if (gFishmodHudTest && (ROD->state == ROD_STATE_FLOAT)) {
        // (a script's look at the fight's indicators with no fish on: the needle swept end to end, the arrow turned
        // round, the run and the stick's cases each in turn: fishmod/scripts/hud_fight.inc.c)
        s32 t = play->gameplayFrames;

        fight = true;
        ROD->tension = (f32)((t * 2) % 200 < 100 ? (t * 2) % 200 : 200 - (t * 2) % 200);
        sPullAngle = (s16)(t * 0x200);
        sRunning = ((t / 60) & 1);
        sOpposing = ((t / 30) & 1);
        sFishLeft = 1.0f - (f32)(t % 400) / 400.0f;
    }
#endif
    // The fight. The tension as a needle over its zones (slack, low, good, high, the drag: red, amber, green, amber,
    // red), the frame flashing in either red one; under it how much the fish has left; over it the stick: an arrow the
    // way to hold it against the run, amber until it's held there and green once it is, bigger while the fish runs.
    if (fight) {
        static const u8 sZone[5][4] = {
            { 0, 200, 50, 40 },   // (from, r, g, b)
            { 12, 200, 170, 40 },
            { 25, 60, 190, 70 },
            { 70, 200, 170, 40 },
            { 85, 200, 50, 40 },
        };
        s32 x0 = 110;
        s32 x1 = 210;
        s32 y = 200;
        s32 needle = x0 + (s32)(CLAMP(ROD->tension, 0.0f, 100.0f));
        s32 danger = (ROD->tension > TENSION_DRAG) || (ROD->tension < TENSION_SLACK);
        s32 flash = danger && ((play->gameplayFrames & 2) != 0);
        s32 z;

        Rod_FillRect(&gfx, x0 - 2, y - 2, x1 + 2, y + 8, flash ? 255 : 20, flash ? 60 : 20, flash ? 40 : 30);
        for (z = 0; z < 5; z++) {
            Rod_FillRect(&gfx, x0 + sZone[z][0], y, x0 + ((z < 4) ? sZone[z + 1][0] : 100), y + 6, sZone[z][1],
                         sZone[z][2], sZone[z][3]);
        }
        Rod_FillRect(&gfx, needle - 2, y - 3, needle + 2, y + 9, 10, 10, 10);
        Rod_FillRect(&gfx, needle - 1, y - 3, needle + 1, y + 9, 255, 255, 255);
        // the fish: what it has left
        Rod_FillRect(&gfx, x0 - 2, y + 11, x1 + 2, y + 17, 20, 20, 30);
        Rod_FillRect(&gfx, x0, y + 12, x0 + (s32)(CLAMP(sFishLeft, 0.0f, 1.0f) * 100.0f), y + 16, 90, 170, 255);
        // the stick
        {
            s32 cx = 160;
            s32 cy = 172;
            f32 dx = -Math_SinS(sPullAngle);
            f32 dy = -Math_CosS(sPullAngle);
            s32 pulse = sRunning && !sOpposing && ((play->gameplayFrames & 4) != 0);
            u8 r = sOpposing ? 70 : 255;
            u8 g = sOpposing ? 235 : (pulse ? 255 : 190);
            u8 bl = sOpposing ? 90 : (pulse ? 200 : 40);
            s32 n;

            Rod_FillRect(&gfx, cx - 17, cy - 17, cx + 17, cy + 17, 20, 20, 30);
            Rod_FillRect(&gfx, cx - 2, cy - 2, cx + 2, cy + 2, 150, 150, 160);
            for (n = 1; n <= 4; n++) {
                s32 px = cx + (s32)(dx * (n * 3.5f));
                s32 py = cy + (s32)(dy * (n * 3.5f));
                s32 h = (n == 4) ? (sRunning ? 4 : 3) : 1; // (the last one the arrow's head)

                Rod_FillRect(&gfx, px - h, py - h, px + h + 1, py + h + 1, r, g, bl);
            }
        }
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
        if (fight) {
            // the gauge's ends named, the one the needle is in lit; and what to do about it, in a word
            const char* now = NULL;
            s32 slack = (ROD->tension < TENSION_SLACK);
            s32 drag = (ROD->tension > TENSION_DRAG);

            GfxPrint_SetColor(&printer, 255, slack ? 90 : 255, slack ? 70 : 255, slack ? 255 : 150);
            GfxPrint_SetPos(&printer, 8, 25); // (left of the gauge; the right end's goes above it, clear of the map)
            GfxPrint_Printf(&printer, "SLACK");
            GfxPrint_SetColor(&printer, 255, drag ? 90 : 255, drag ? 70 : 255, drag ? 255 : 150);
            GfxPrint_SetPos(&printer, 23, 24);
            GfxPrint_Printf(&printer, "SNAP");
            GfxPrint_SetColor(&printer, 150, 200, 255, 200);
            GfxPrint_SetPos(&printer, 9, 26);
            GfxPrint_Printf(&printer, "FISH");
            if (drag) {
                now = "EASE OFF!";
                GfxPrint_SetColor(&printer, 255, 90, 70, 255);
            } else if (slack) {
                now = "REEL!";
                GfxPrint_SetColor(&printer, 255, 90, 70, 255);
            } else if (sRunning && !sOpposing) {
                now = "PULL!";
                GfxPrint_SetColor(&printer, 255, 220, 80, 255);
            } else if (sOpposing) {
                now = "GOOD";
                GfxPrint_SetColor(&printer, 110, 240, 120, 255);
            }
            if (now != NULL) {
                GfxPrint_SetPos(&printer, 20 - strlen(now) / 2, 18);
                GfxPrint_Printf(&printer, "%s", now);
            }
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
                    hint = "A:Reel  Stick: the way the arrow points";
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
