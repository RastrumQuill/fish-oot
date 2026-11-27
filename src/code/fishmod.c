/*
 * File: fishmod.c
 * Description: fishmod core. Species data, which scenes have fishing, and the hooks that load the fishing
 * object and spawn the fishing actor in those scenes. Always part of the code segment.
 */

#include "fishmod.h"
#include "fishmod_feel.h"
#include "fishmod_bedcard.h"
#if FISHMOD_WATER_SURFACE
#include "assets/objects/object_spot06_objects/fishmod_water_colours.h"
#endif
#include "src/fishmod_data/fishmod_profile.h"
#include "assert.h"
#include "fishmod_species_data.h"

#include "array_count.h"
#include "actor.h"
#include "object.h"
#include "play_state.h"
#include "gfx.h"
#include "thga.h"
#include "player.h"
#include "segmented_address.h"
#include "actor_profile.h"
#include "controller.h"
#include "interface.h"
#include "item.h"
#include "inventory.h"
#include "message.h"
#include "message_data_fmt.h"
#include "save.h"
#include "environment.h"
#include "skybox.h"
#include "sfx.h"
#include "sequence.h"
#include "audio.h"
#include "z_lib.h"
#include "string.h"
#include "regs.h"
#include "transition.h"
#include "libc64/sprintf.h"
#include "rand.h"
#include "bgcheck.h"
#include "libc/math.h"
#include "ultra64/convert.h"
#include "speed_meter.h"
#include "libu64/gfxprint.h"
#include "ultra64/rcp.h"

#if FISHMOD_TESTMODE
static s32 sFpsDetail = 1; // L + D-pad Left: the RDP readout on or off
#endif

s32 Object_LoadPersistent(ObjectContext* objectCtx, s16 objectId);

Actor* gFishmodRod = NULL;
static void Fishmod_WeatherLeaveScene(void);
static u8 sRainSoundPending; // start the rain's sound once the scene's music has started (see the weather)
Vec3f gFishmodHivePos; // where the scene's added beehive went (debug display)

#if FISHMOD_TESTMODE
static void Fishmod_TestSetAge(s32 adult);
static s8 sTestPendingAge = -1; // the age the next scene loads with (set by the test warps)

/** Test mode, at the start of Play_Init: the warp's age, before the scene and Link load (not during the fade: the
 * Link leaving would be drawn holding the other age's gear). */
void Fishmod_TestPlayInit(void) {
    if (sTestPendingAge >= 0) {
        Fishmod_TestSetAge(sTestPendingAge == LINK_AGE_ADULT);
        sTestPendingAge = -1;
    }
}
#endif

/** What B holds normally: the equipped sword, or nothing. */
static u8 Fishmod_SwordBItem(void) {
    switch (CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD)) {
        case EQUIP_VALUE_SWORD_KOKIRI:
            return ITEM_SWORD_KOKIRI;
        case EQUIP_VALUE_SWORD_MASTER:
            return ITEM_SWORD_MASTER;
        case EQUIP_VALUE_SWORD_BIGGORON:
            return ITEM_SWORD_BIGGORON;
        default:
            return ITEM_NONE;
    }
}

FishmodPhoto gFishmodPhoto;
FishmodStudio gFishmodStudio;
FishmodAnimCheck gFishmodAnimCheck;
FishmodRodInfo gFishmodRodInfo;

const FishmodSceneSpawns* Fishmod_GetSceneSpawns(s16 sceneId) {
    s32 i;

#if FISHMOD_DEBUG
    if ((gFishmodStress == 1) && (sceneId == SCENE_LAKESIDE_LABORATORY)) {
        sceneId = SCENE_LAKE_HYLIA; // the fish stress bench: Lake Hylia's fish in the lab's tank
    }
#endif
    for (i = 0; i < gFishmodSceneSpawnsCount; i++) {
        if (gFishmodSceneSpawns[i].sceneId == sceneId) {
            return &gFishmodSceneSpawns[i];
        }
    }
    return NULL;
}

/**
 * Called from Play_InitScene after the scene header commands ran, before any room objects are loaded,
 * so the fishing object can be loaded as a persistent object for the whole scene.
 */
/*
 * Fish kept from each scene's roster today: they don't come back until the day changes. A few scenes
 * are remembered; the oldest is forgotten first.
 */
typedef struct FishmodRosterCaught {
    s16 sceneId;
    s16 index; // in the scene's roster, plus 1 (0: an empty entry)
    s32 day;
} FishmodRosterCaught;

// Kept fish are few (they need bottles), so a short list; the oldest is forgotten first.
static FishmodRosterCaught sRosterCaught[24];
static s32 sRosterCaughtNext;

s32 Fishmod_RosterIsCaught(s16 sceneId, s32 day, s32 index) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sRosterCaught); i++) {
        if ((sRosterCaught[i].index == index + 1) && (sRosterCaught[i].sceneId == sceneId) &&
            (sRosterCaught[i].day == day)) {
            return true;
        }
    }
    return false;
}

void Fishmod_RosterSetCaught(s16 sceneId, s32 day, s32 index) {
    FishmodRosterCaught* entry = &sRosterCaught[sRosterCaughtNext];

    if (Fishmod_RosterIsCaught(sceneId, day, index)) {
        return;
    }
    sRosterCaughtNext = (sRosterCaughtNext + 1) % ARRAY_COUNT(sRosterCaught);
    entry->sceneId = sceneId;
    entry->index = index + 1;
    entry->day = day;
}

/** The scene has fish (a spawn table; or a debug job wants them): their object, the manager, the roster. */
s32 Fishmod_SceneHasFish(PlayState* play) {
    return (Fishmod_GetSceneSpawns(play->sceneId) != NULL)
#if FISHMOD_DEBUG
           || Fishmod_DebugWantsFish()
#endif
        ;
}

FishmodRng gFishmodRng;
FishmodRng* gFishmodRngNow = &gFishmodRng;

void Fishmod_RandSeed(FishmodRng* rng, u32 seed) {
    // (a few rounds of mixing, so nearby seeds give unrelated streams)
    seed ^= seed >> 16;
    seed *= 0x7FEB352D;
    seed ^= seed >> 15;
    seed *= 0x846CA68B;
    seed ^= seed >> 16;
    rng->state = (seed != 0) ? seed : 0x2545F491;
}

/** The next number: xorshift32. */
u32 Fishmod_RandNext(FishmodRng* rng) {
    u32 x = rng->state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng->state = x;
    return x;
}

/** [0, 1) */
f32 Fishmod_RandZeroOne(FishmodRng* rng) {
    return (Fishmod_RandNext(rng) >> 8) * (1.0f / 16777216.0f);
}

/** [0, f) */
f32 Fishmod_RandZeroFloat(FishmodRng* rng, f32 f) {
    return Fishmod_RandZeroOne(rng) * f;
}

/** [-f/2, f/2) */
f32 Fishmod_RandCenteredFloat(FishmodRng* rng, f32 f) {
    return (Fishmod_RandZeroOne(rng) - 0.5f) * f;
}

/*
 * The game's View_Init sets the X of the view's look-at point and leaves its Y and Z as the memory was. The camera
 * fills them in at the end of the first frame, but Kokiri Forest's fairies (ObjectKankyo_Fairies) read them on it.
 * In the game as shipped that memory holds zero or the last scene's look-at point. A build that starts straight in
 * the forest (the test and debug builds) finds whatever used the memory before: seen, two words of code, which as
 * floats are denormal, and the CPU stops on arithmetic with those (a floating-point exception on the first frame, in
 * one build and not the next: it goes by where things fall in memory). A value that isn't a usable number (denormal,
 * infinite or not a number) is made zero here, after View_Init and before anything reads it; a usable one is left as
 * the game would have had it.
 */
static void Fishmod_ViewSane(f32* f) {
    u32 bits = *(u32*)f;
    u32 exponent = bits & 0x7F800000;

    if ((exponent == 0x7F800000) || ((exponent == 0) && ((bits & 0x007FFFFF) != 0))) {
        *f = 0.0f;
    }
}

void Fishmod_OnSceneLoad(PlayState* play) {
#ifdef FISHMOD_TEST_VIEW_GARBAGE // (the fix's own test: the two words the crash was seen with)
    *(u32*)&play->view.at.y = 0x04410002;
    *(u32*)&play->view.at.z = 0x00400821;
#endif
#ifndef FISHMOD_TEST_VIEW_NOFIX
    Fishmod_ViewSane(&play->view.at.y);
    Fishmod_ViewSane(&play->view.at.z);
#endif
    gFishmodRod = NULL;
    Fishmod_ResetRodInfo();
    Fishmod_WeatherLeaveScene();
    Fishmod_RandSeed(&gFishmodRng, FISHMOD_TIME_SEED() + play->sceneId);

    if (!Fishmod_SceneHasFish(play)) {
        return;
    }

    if (Object_GetSlot(&play->objectCtx, OBJECT_FISH) < 0) {
        Object_LoadPersistent(&play->objectCtx, OBJECT_FISH);
    }
}

/**
 * Beehives fishmod adds for bee larva bait: per scene, a spot by the water; the hive goes on the nearest wall around it
 * (within 400 units), hanging toward the water.
 */
typedef struct FishmodHiveSpot {
    s16 sceneId;
    Vec3f lookFrom;
} FishmodHiveSpot;

static const FishmodHiveSpot sHiveSpots[] = {
    { SCENE_KOKIRI_FOREST, { 960.0f, 75.0f, -60.0f } }, // in sight of the moat
};

static void Fishmod_SpawnSceneExtras(PlayState* play) {
    s32 n;

    for (n = 0; n < ARRAY_COUNT(sHiveSpots); n++) {
        Vec3f from = sHiveSpots[n].lookFrom;
        Vec3f best;
        f32 bestDist = 400.0f;
        s16 bestYaw = 0;
        s32 i;

        if (sHiveSpots[n].sceneId != play->sceneId) {
            continue;
        }
        for (i = 0; i < 16; i++) {
            s16 yaw = i * 0x1000;
            Vec3f to;
            Vec3f hit;
            CollisionPoly* poly;
            s32 bgId;

            to.x = from.x + Math_SinS(yaw) * 400.0f;
            to.y = from.y;
            to.z = from.z + Math_CosS(yaw) * 400.0f;
            if (BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &hit, &poly, true, false, false, true, &bgId)) {
                f32 dist = Math_Vec3f_DistXZ(&from, &hit);

                if ((dist > 60.0f) && (dist < bestDist)) {
                    bestDist = dist;
                    bestYaw = yaw;
                    best = hit;
                }
            }
        }
        if (bestDist < 400.0f) {
            // just off the wall, hanging toward the water
            best.x -= Math_SinS(bestYaw) * 10.0f;
            best.z -= Math_CosS(bestYaw) * 10.0f;
            Actor_Spawn(&play->actorCtx, play, ACTOR_OBJ_COMB, best.x, best.y, best.z, 0, bestYaw + 0x8000, 0,
                        0xFF02);
            gFishmodHivePos = best;
        }
    }
}

/**
 * Called every Play_Update. Spawns the fishing actor once the scene is running.
 */
void Fishmod_PlayUpdate(PlayState* play) {
    Actor* rod;

    Fishmod_TimeUpdate(play);
#if FISHMOD_TESTMODE
    Fishmod_FeelUpdate(); // (the desktop tuner's lines: fishmod_feel.inc.c)
#endif

    // the rain's sound (the nature ambience's rain channel, as vanilla's rain weather plays it), once the scene's
    // own music has started
    if (sRainSoundPending && (play->transitionMode == TRANS_MODE_OFF) && (play->state.frames > 30)) {
        sRainSoundPending = false;
        Environment_PlayStormNatureAmbience(play);
    }

    // Repair files saved by old builds: one bug left a Deku Stick on B (vanilla never puts it there), and L used to
    // put the rod on B (only the Fishing Pond's rental does that now, and vanilla gives the sword back after it)
    if ((gSaveContext.save.info.equips.buttonItems[0] == ITEM_DEKU_STICK) ||
        ((gSaveContext.save.info.equips.buttonItems[0] == ITEM_FISHING_POLE) && (play->sceneId != SCENE_FISHING_POND))) {
        gSaveContext.save.info.equips.buttonItems[0] = Fishmod_SwordBItem();
        if (gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) {
            Interface_LoadItemIcon1(play, 0);
        }
    }

#if FISHMOD_TESTMODE
    // test mode: L + D-pad Up warps to Kokiri Forest (small water) as child Link, L + D-pad Down to Lake Hylia (big
    // water) as adult Link with the lake full, in the Iron Boots
    if ((play->pauseCtx.state == 0) && (Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) &&
        !Play_InCsMode(play) && (play->transitionTrigger == TRANS_TRIGGER_OFF)) {
        // (behind L, so the D-pad stays free for the game: hold L and press the D-pad)
        u16 press = CHECK_BTN_ALL(play->state.input[0].cur.button, BTN_L) ? play->state.input[0].press.button : 0;
        s16 entrance = -1;

        // L + D-pad Left: the RDP readout
        if (CHECK_BTN_ALL(press, BTN_DLEFT)) {
            sFpsDetail ^= 1;
        }
        if (CHECK_BTN_ALL(press, BTN_DUP) && (play->sceneId != SCENE_KOKIRI_FOREST)) {
            entrance = ENTR_KOKIRI_FOREST_0;
        } else if (CHECK_BTN_ALL(press, BTN_DDOWN) && (play->sceneId != SCENE_LAKE_HYLIA)) {
            entrance = ENTR_LAKE_HYLIA_0;
        }
        if (entrance >= 0) {
            sTestPendingAge = (entrance == ENTR_LAKE_HYLIA_0) ? LINK_AGE_ADULT : LINK_AGE_CHILD;
            play->nextEntranceIndex = entrance;
            play->transitionTrigger = TRANS_TRIGGER_START;
            play->transitionType = TRANS_TYPE_FADE_BLACK;
        }
    }
#endif

    // The rod's controller: from the start in a scene with fish (it brings the manager and the scene's extras), and
    // in any other scene once Link takes the rod out (it goes again when he puts it away). Not in the Fishing Pond,
    // whose own fishing actor runs the rod there.
    if ((gFishmodRod != NULL) || (GET_PLAYER(play) == NULL) || (play->sceneId == SCENE_FISHING_POND)) {
        return;
    }
    if (!Fishmod_SceneHasFish(play) && (GET_PLAYER(play)->heldItemAction != PLAYER_IA_FISHING_POLE)) {
        return;
    }

    rod = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, 0.0f, 0.0f, 0.0f, 0, 0, 0,
                      FISHMOD_PARAMS(FISHMOD_TYPE_ROD, 0));
    if (rod != NULL) {
        rod->room = -1; // persists through room changes
        gFishmodRod = rod;
        if (Fishmod_SceneHasFish(play)) {
            Fishmod_SpawnSceneExtras(play);
        }
    }
}

f32 gFishmodSpawnLength;
u8 gFishmodSpawnLook;
// a fishmod fish poured out on land (the game's flopping fish stands in for it until it's bottled again)
static FishmodBottleFish sFloppingFish;

static const char* sBaitNames[FISHMOD_BAIT_MAX] = {
    "None", "Bee larva", "Bug", "Worm", "Coral earring", "Fish", "Pond lure", "Sinking lure",
};

const char* Fishmod_BaitName(s32 bait) {
    return ((bait >= 0) && (bait < FISHMOD_BAIT_MAX)) ? sBaitNames[bait] : "?";
}

/** Forget the rod's state (a new scene, or the rod gone): nothing held, nothing on the hook. */
void Fishmod_ResetRodInfo(void) {
    gFishmodRodInfo.state = 0;
    gFishmodRodInfo.biting = false;
    gFishmodRodInfo.nibbling = false;
    gFishmodRodInfo.tension = 0.0f;
    gFishmodRodInfo.castPower = 0.0f;
    gFishmodRodInfo.choice = 0;
    gFishmodRodInfo.notice = NULL;
    gFishmodRodInfo.bait = FISHMOD_BAIT_NONE;
    gFishmodRodInfo.baitRequest = FISHMOD_BAIT_NONE;
    gFishmodRodInfo.spareBait = FISHMOD_BAIT_NONE; // (with the hook's own: it didn't outlive the scene, or the file)
}

/** Link has the rod out in a fishing scene (its actor is there and he's holding it). */
static s32 Fishmod_RodHeld(PlayState* play) {
    Player* player = GET_PLAYER(play);

    return (gFishmodRod != NULL) && (player != NULL) && (player->heldItemAction == PLAYER_IA_FISHING_POLE);
}

/** The rig on the line changed: kept in the save, and the sinker item's icon on a C button redrawn (it shows the
 * bobber while the sinker's on: Fishmod_TackleShown; the D-pad's notices by itself). */
void Fishmod_SetRig(PlayState* play, s32 rig) {
    s32 i;

    FISHMOD_SET_RIG(rig);
    for (i = 1; i <= 3; i++) {
        if (gSaveContext.save.info.equips.buttonItems[i] == ITEM_SINKER) {
            Interface_LoadItemIcon1(play, i);
        }
    }
}

/**
 * A bottle of bait (or bugs) used from a button while Link holds the rod, before casting: one goes on the hook
 * and the bottle that was pressed has one less. Anything else about these bottles does nothing for now (there's no
 * pouring larvae out). Returns true if it handled the item.
 */
s32 Fishmod_UseItem(PlayState* play, s32 item, s32 button) {
    static u8 sNextLarva[] = { ITEM_BOTTLE_LARVA_2, ITEM_BOTTLE_LARVA_1, ITEM_BOTTLE_EMPTY };
    s32 bait;
    s32 next;

    if (FISHMOD_IS_LURE_ITEM(item)) {
        // tackle: onto the line with the rod in hand and not cast (the rod says what's on: fishmod_rod.c); the same
        // again takes it off, back to the bobber
        s32 rig = FISHMOD_RIG_LURE_POND + (item - ITEM_LURE_POND);

        if (!Fishmod_RodHeld(play) || (gFishmodRodInfo.state != 1)) {
            Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
            return true;
        }
        Fishmod_SetRig(play, (FISHMOD_RIG() == rig) ? FISHMOD_RIG_BOBBER : rig);
        Sfx_PlaySfxCentered(NA_SE_PL_PUT_OUT_ITEM);
        return true;
    }
    if (FISHMOD_IS_BAIT_ITEM(item)) {
        bait = FISHMOD_BAIT_BEE_LARVA;
        next = sNextLarva[item - ITEM_BOTTLE_LARVA_3];
    } else if (item == ITEM_BOTTLE_WORM) {
        bait = FISHMOD_BAIT_WORM;
        next = ITEM_BOTTLE_WORM; // (one fewer worm in it: counted below, empty after the last)
    } else if ((item == ITEM_BOTTLE_BUG) && Fishmod_RodHeld(play)) {
        bait = FISHMOD_BAIT_BUG;
        next = ITEM_BOTTLE_EMPTY;
    } else {
        return false;
    }
    // only onto a bare hook, with the rod held and not cast (a lure on the line comes off for the bobber: a lure
    // takes no bait; the sinker's hook does)
    if (Fishmod_RodHeld(play) && (gFishmodRodInfo.state == 1) &&
        ((FISHMOD_RIG() == FISHMOD_RIG_LURE_POND) || (FISHMOD_RIG() == FISHMOD_RIG_LURE_SINKING))) {
        Fishmod_SetRig(play, FISHMOD_RIG_BOBBER);
        if (gFishmodRodInfo.spareBait != FISHMOD_BAIT_NONE) {
            // the bait set aside when the lure went on goes back on the hook (the rod, next frame): none used up
            Sfx_PlaySfxCentered(NA_SE_PL_PUT_OUT_ITEM);
            return true;
        }
        gFishmodRodInfo.bait = FISHMOD_BAIT_NONE;
    }
    if (!Fishmod_RodHeld(play) || (gFishmodRodInfo.state != 1) || (gFishmodRodInfo.bait != FISHMOD_BAIT_NONE) ||
        (gFishmodRodInfo.baitRequest != FISHMOD_BAIT_NONE)) {
        Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
        return true;
    }
    gFishmodRodInfo.baitRequest = bait;
    if (item == ITEM_BOTTLE_WORM) {
        // the bottle pressed (else the first of worms): one worm fewer, empty after the last
        s32 slot = ((button >= 1) && (button < FISHMOD_BUTTON_MAX)) ? Fishmod_ButtonSlot(button) : SLOT_NONE;
        s32 i;

        if (!FISHMOD_IS_BOTTLE_SLOT(slot) || (INV_SLOT(slot) != ITEM_BOTTLE_WORM)) {
            for (i = 0; i < FISHMOD_BOTTLES; i++) {
                if (INV_SLOT(FISHMOD_BOTTLE_SLOT(i)) == ITEM_BOTTLE_WORM) {
                    slot = FISHMOD_BOTTLE_SLOT(i);
                    break;
                }
            }
        }
        if (FISHMOD_IS_BOTTLE_SLOT(slot)) {
            FishmodBottleFish* rec = &FISHMOD_SAVE->bottleFish[FISHMOD_BOTTLE_INDEX(slot)];

            if ((rec->lengthTenths == 0) || (rec->lengthTenths > FISHMOD_WORMS_PER_BOTTLE)) {
                rec->lengthTenths = FISHMOD_WORMS_PER_BOTTLE; // (a bottle of worms come by some other way: full)
            }
            rec->lengthTenths--;
            if (rec->lengthTenths == 0) {
                next = ITEM_BOTTLE_EMPTY;
                rec->species = rec->look = 0;
            }
        }
    }
    if ((button >= 1) && (button < FISHMOD_BUTTON_MAX)) {
        Inventory_UpdateBottleItem(play, next, button); // the bottle on the button that was pressed
    } else {
        Inventory_ReplaceItem(play, item, next);
    }
    Sfx_PlaySfxCentered(NA_SE_PL_PUT_OUT_ITEM);
    return true;
}

static u8 sLarvaCaughtText;

void Fishmod_OnBottleCatch(s32 actorId, s32 slot) {
    if (actorId == ACTOR_EN_FISHMOD) {
        sLarvaCaughtText = true;
    } else if ((actorId == ACTOR_EN_FISH) && FISHMOD_IS_BOTTLE_SLOT(slot)) {
        // the flopping fish poured out on land: if it was one of fishmod's, it's that fish again
        FISHMOD_SAVE->bottleFish[FISHMOD_BOTTLE_INDEX(slot)] = sFloppingFish;
        sFloppingFish.species = 0;
        sFloppingFish.lengthTenths = 0;
    }
}

/** Notes what just went into a bottle: the first empty one, which is where Item_Give puts a fish. */
void Fishmod_BottleFish(s32 species, f32 lengthInches, s32 look) {
    s32 i;

    for (i = 0; i < FISHMOD_BOTTLES; i++) {
        if (INV_SLOT(FISHMOD_BOTTLE_SLOT(i)) == ITEM_BOTTLE_EMPTY) {
            FISHMOD_SAVE->bottleFish[i].species = species + 1;
            FISHMOD_SAVE->bottleFish[i].lengthTenths = (u16)(lengthInches * 10.0f);
            FISHMOD_SAVE->bottleFish[i].look = look;
            return;
        }
    }
}

/**
 * Link pours a bottled fish out (`slot` is its inventory slot). In a fishing scene, over water, it goes back in as
 * the fish it was (a vanilla bottled fish becomes one of the fish that live there). Returns false to let the game
 * drop a vanilla fish instead (on land, or where fishmod has no fish).
 */
s32 Fishmod_ReleaseBottleFish(PlayState* play, s32 slot, Vec3f* pos, s16 yaw) {
    const FishmodSceneSpawns* spawns = Fishmod_GetSceneSpawns(play->sceneId);
    FishmodBottleFish* bottle;
    WaterBox* waterBox;
    f32 surface;
    s32 species;
    s32 i;

    if ((spawns == NULL) || (gFishmodRod == NULL) || !FISHMOD_IS_BOTTLE_SLOT(slot)) {
        return false;
    }
    bottle = &FISHMOD_SAVE->bottleFish[FISHMOD_BOTTLE_INDEX(slot)];
    // into real water a little ahead of Link (he pours it out in front of him): water boxes reach under the banks
    // in places, so check there's water above the ground, deep enough to swim in
    for (i = 0; i < 3; i++) {
        f32 x = pos->x + Math_SinS(yaw) * (i * 20.0f);
        f32 z = pos->z + Math_CosS(yaw) * (i * 20.0f);

        if (BgCheck_GetWaterSurface(play, &play->colCtx, x, z, &surface, &waterBox) && (surface < pos->y + 20.0f)) {
            CollisionPoly* poly;
            Vec3f probe;

            probe.x = x;
            probe.y = surface + 30.0f;
            probe.z = z;
            if (surface - BgCheck_EntityRaycastDown1(&play->colCtx, &poly, &probe) > 10.0f) {
                break;
            }
        }
    }
    if (i == 3) {
        // on land: the game's own flopping fish, which remembers what it was if it's bottled again
        sFloppingFish = *bottle;
        bottle->species = 0;
        bottle->lengthTenths = 0;
        return false;
    }
    if (bottle->species != 0) {
        species = bottle->species - 1;
        gFishmodSpawnLength = bottle->lengthTenths * 0.1f;
        gFishmodSpawnLook = bottle->look;
    } else {
        species = spawns->entries[(s32)Fishmod_RandZeroFloat(gFishmodRngNow, spawns->count - 0.01f)].species;
        gFishmodSpawnLength = 0.0f;
    }
    bottle->species = 0;
    bottle->lengthTenths = 0;
    bottle->look = 0;
    Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, pos->x + Math_SinS(yaw) * (i * 20.0f), surface - 6.0f,
                pos->z + Math_CosS(yaw) * (i * 20.0f), 0, yaw, 0, FISHMOD_PARAMS(FISHMOD_TYPE_FISH, species));
    gFishmodSpawnLength = 0.0f;
    return true;
}

s32 Fishmod_RecordCatch(s32 species, f32 lengthInches) {
    FishmodSpeciesRecord* record;
    u16 tenths = (u16)(lengthInches * 10.0f);

    if ((species < 0) || (species >= FISHMOD_SAVE_SPECIES_MAX)) {
        return false;
    }
    record = &FISHMOD_SAVE->records[species];
    if (record->count < 0xFFFF) {
        record->count++;
    }
    if (tenths > record->bestTenths) {
        s32 isFirst = (record->bestTenths == 0);

        record->bestTenths = tenths;
        return !isFirst; // the first catch of a species isn't announced as a record
    }
    return false;
}

/* ---------------------------------------------------------------------------------------------------------------
 * Fishing journal: extra pages on the records plaque in Link's house (message 0x22A)
 */

static u8* Journal_Put(u8* p, u8* end, const char* str) {
    while ((*str != '\0') && (p < end)) {
        *p++ = (u8)*str++;
    }
    return p;
}

static u8* Journal_PutNum(u8* p, u8* end, u32 value) {
    char digits[10];
    s32 n = 0;

    do {
        digits[n++] = '0' + (value % 10);
        value /= 10;
    } while ((value != 0) && (n < ARRAY_COUNT(digits)));
    while ((n > 0) && (p < end)) {
        *p++ = (u8)digits[--n];
    }
    return p;
}

static u8* Journal_PutColor(u8* p, u8* end, u8 color) {
    if (p + 2 <= end) {
        *p++ = MESSAGE_COLOR;
        *p++ = 0x40 | color;
    }
    return p;
}

/** Every species that can be caught somewhere, in spawn table order. Returns how many. */
static s32 Journal_Species(u8* out) {
    s32 count = 0;
    s32 i;
    s32 j;
    s32 k;

    for (i = 0; i < gFishmodSceneSpawnsCount; i++) {
        for (j = 0; j < gFishmodSceneSpawns[i].count; j++) {
            u8 species = gFishmodSceneSpawns[i].entries[j].species;

            for (k = 0; k < count; k++) {
                if (out[k] == species) {
                    break;
                }
            }
            if ((k == count) && (count < FISHMOD_SPECIES_MAX)) {
                out[count++] = species;
            }
        }
    }
    return count;
}

// The catch textbox's text, built when a fish is landed and swapped in when its message loads
static u8 sCatchMsg[160];
static s32 sCatchMsgLength;

static u8* Msg_PutString(u8* p, u8* end, const char* str) {
    while ((*str != '\0') && (p < end)) {
        *p++ = (u8)*str++;
    }
    return p;
}

/**
 * Opens the textbox for a landed fish: its name and size, a new record, and the choice to keep it (in an empty
 * bottle) or let it go. Without an empty bottle there's no choice; it goes back. The answer is
 * play->msgCtx.choiceIndex (0 keep, 1 release) once the choice is made.
 */
void Fishmod_ShowCatchMessage(PlayState* play, const char* name, const char* size, s32 newRecord, s32 canKeep) {
    u8* p = sCatchMsg;
    u8* end = sCatchMsg + sizeof(sCatchMsg) - 1;

    p = Msg_PutString(p, end, "You caught a ");
    p = Journal_PutColor(p, end, TEXT_COLOR_RED);
    p = Msg_PutString(p, end, name);
    p = Journal_PutColor(p, end, TEXT_COLOR_DEFAULT);
    p = Msg_PutString(p, end, "!");
    if (p < end) {
        *p++ = MESSAGE_NEWLINE;
    }
    p = Msg_PutString(p, end, size);
    if (newRecord) {
        p = Msg_PutString(p, end, "  ");
        p = Journal_PutColor(p, end, TEXT_COLOR_YELLOW);
        p = Msg_PutString(p, end, "New record!");
        p = Journal_PutColor(p, end, TEXT_COLOR_DEFAULT);
    }
    // the choice gets a box of its own (a box holds four lines)
    if (p < end) {
        *p++ = MESSAGE_BOX_BREAK;
    }
    if (canKeep) {
        p = Msg_PutString(p, end, "Keep it in a bottle?");
        // (an empty line before the choices, as the game's own two-choice messages have: the choice arrow is drawn
        // at a four-line box's third and fourth lines, and with three lines the box centres its text 4 pixels off
        // them: test 5's misaligned Yes and No)
        if (p + 3 < end) {
            *p++ = MESSAGE_NEWLINE;
            *p++ = MESSAGE_NEWLINE;
            *p++ = MESSAGE_TWO_CHOICE;
        }
        p = Journal_PutColor(p, end, TEXT_COLOR_ADJUSTABLE);
        p = Msg_PutString(p, end, "Yes");
        if (p < end) {
            *p++ = MESSAGE_NEWLINE;
        }
        p = Msg_PutString(p, end, "No");
        p = Journal_PutColor(p, end, TEXT_COLOR_DEFAULT);
    } else {
        p = Msg_PutString(p, end, "No empty bottle,");
        if (p < end) {
            *p++ = MESSAGE_NEWLINE;
        }
        p = Msg_PutString(p, end, "so you let it go.");
    }
    *p++ = MESSAGE_END;
    sCatchMsgLength = p - sCatchMsg;
    Message_StartTextbox(play, FISHMOD_CATCH_TEXT_ID, NULL);
}

/**
 * Called when message `textId` has just been loaded into `buf` (`*length` bytes, room for `size`). After the
 * vanilla "Link's Records" page, adds the fishing journal: kinds caught, then for each species how many and the
 * biggest, or ??? until one is caught.
 */
void Fishmod_ExtendMessage(u16 textId, u8* buf, u16* length, s32 size) {
    u8 species[FISHMOD_SPECIES_MAX];
    s32 numSpecies;
    s32 caughtKinds = 0;
    s32 i;
    u8* p;
    u8* end = buf + size - 2;
    s32 last;

    if ((textId == 0x7A) && sLarvaCaughtText) {
        // the bugs message, for a chunk of beehive scooped into a bottle
        static const char sLarvaText[] = "You got a chunk of beehive\nwith " "\x05\x41" "bee larvae" "\x05\x40"
                                         " in it!\nUse it with the fishing rod\nto bait the hook.";
        s32 n = 0;
        const char* c;

        for (c = sLarvaText; (*c != '\0') && (n < size - 1); c++) {
            buf[n++] = (*c == '\n') ? MESSAGE_NEWLINE : (u8)*c;
        }
        buf[n++] = MESSAGE_END;
        *length = n;
        sLarvaCaughtText = false;
        return;
    }
    if ((textId == FISHMOD_CATCH_TEXT_ID) && (sCatchMsgLength != 0)) {
        // a catch textbox, once; the Fishing Pond's own use of this message is left alone
        if (sCatchMsgLength <= size) {
            memcpy(buf, sCatchMsg, sCatchMsgLength);
            *length = sCatchMsgLength;
        }
        sCatchMsgLength = 0;
        return;
    }
    if (textId != 0x22A) {
        return;
    }
    // messages are padded after their end code
    last = *length - 1;
    while ((last > 0) && (buf[last] == 0)) {
        last--;
    }
    if ((last <= 0) || (buf[last] != MESSAGE_END)) {
        return;
    }
    *length = last + 1;

    numSpecies = Journal_Species(species);
    for (i = 0; i < numSpecies; i++) {
        if (FISHMOD_SAVE->records[species[i]].count != 0) {
            caughtKinds++;
        }
    }

    // replace the end of the vanilla page with a box break and keep going
    p = buf + *length - 1;
    *p++ = MESSAGE_BOX_BREAK;
    *p++ = MESSAGE_QUICKTEXT_ENABLE;
    p = Journal_PutColor(p, end, TEXT_COLOR_RED);
    p = Journal_Put(p, end, "Fishing Journal");
    p = Journal_PutColor(p, end, TEXT_COLOR_DEFAULT);
    *p++ = MESSAGE_NEWLINE;
    p = Journal_Put(p, end, "Kinds of fish caught: ");
    p = Journal_PutNum(p, end, caughtKinds);
    p = Journal_Put(p, end, " of ");
    p = Journal_PutNum(p, end, numSpecies);

    // two species per box
    for (i = 0; i < numSpecies; i++) {
        const FishmodSpecies* sp = &gFishmodSpecies[species[i]];
        FishmodSpeciesRecord* record = &FISHMOD_SAVE->records[species[i]];

        if (p + 64 > end) {
            break;
        }
        *p++ = ((i % 2) == 0) ? MESSAGE_BOX_BREAK : MESSAGE_NEWLINE;
        if (record->count == 0) {
            p = Journal_PutColor(p, end, TEXT_COLOR_BLUE);
            p = Journal_Put(p, end, "???");
            p = Journal_PutColor(p, end, TEXT_COLOR_DEFAULT);
            *p++ = MESSAGE_NEWLINE;
            p = Journal_Put(p, end, "  Not caught yet");
        } else {
            f32 inches = record->bestTenths * 0.1f;
            f32 pounds = sp->weightCoef * inches * inches * inches * 0.000001f;

            p = Journal_PutColor(p, end, TEXT_COLOR_YELLOW);
            p = Journal_Put(p, end, sp->name);
            p = Journal_PutColor(p, end, TEXT_COLOR_DEFAULT);
            *p++ = MESSAGE_NEWLINE;
            p = Journal_Put(p, end, "  Caught ");
            p = Journal_PutNum(p, end, record->count);
            p = Journal_Put(p, end, ", best ");
            p = Journal_PutNum(p, end, record->bestTenths / 10);
            p = Journal_Put(p, end, ".");
            p = Journal_PutNum(p, end, record->bestTenths % 10);
            p = Journal_Put(p, end, " in, ");
            if (pounds < 1.0f) {
                p = Journal_PutNum(p, end, (u32)(pounds * 16.0f + 0.5f));
                p = Journal_Put(p, end, " oz");
            } else {
                p = Journal_PutNum(p, end, (u32)pounds);
                p = Journal_Put(p, end, ".");
                p = Journal_PutNum(p, end, (u32)(pounds * 10.0f) % 10);
                p = Journal_Put(p, end, " lbs");
            }
        }
    }
    *p++ = MESSAGE_QUICKTEXT_DISABLE;
    *p++ = MESSAGE_END;
    *length = p - buf;
}

#if FISHMOD_TESTMODE
#define OWN_EQUIP(type, inv) ((1 << (inv)) << ((type) * 4))

/**
 * Test mode: Link's age for where he's going (the next scene loads the right Link). Adult: Master Sword, Hylian
 * Shield, Zora Tunic (no drowning while testing under water) and the Iron Boots on (take them off in the
 * equipment screen to swim); child: the Kokiri Sword, Deku Shield and his own tunic and boots.
 */
static void Fishmod_TestSetAge(s32 adult) {
    u8 sword;

    gSaveContext.save.linkAge = adult ? LINK_AGE_ADULT : LINK_AGE_CHILD;
    if (adult) {
        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
        Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_HYLIAN);
        Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_ZORA);
        Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_IRON);
        sword = ITEM_SWORD_MASTER;
    } else {
        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
        Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_DEKU);
        Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_KOKIRI);
        Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_KOKIRI);
        sword = ITEM_SWORD_KOKIRI;
    }
    gSaveContext.save.info.equips.buttonItems[0] = sword;
}

void Fishmod_InitTestSave(void) {
    Inventory* inv = &gSaveContext.save.info.inventory;
    s32 i;

    gSaveContext.save.cutsceneIndex = CS_INDEX_NONE; // no Deku Tree / Navi intro

    // Saria and Mido already met, as on the debug save
    gSaveContext.save.info.infTable[INFTABLE_INDEX_0] |= INFTABLE_MASK(INFTABLE_SARIA_GREETED_LINK) |
                                                         INFTABLE_MASK(INFTABLE_SARIA_WAS_TOLD_ABOUT_MIDO) |
                                                         INFTABLE_MASK(INFTABLE_0C) | INFTABLE_MASK(INFTABLE_0E);

    // bait to test with in all but one bottle (bee larvae and worms, three uses each, and bugs),
    // the last empty for a catch; bottle 4's bugs are on C-Right
    Fishmod_InitSaveExtras();
    for (i = 0; i < FISHMOD_BOTTLES; i++) {
        INV_SLOT(FISHMOD_BOTTLE_SLOT(i)) = ITEM_BOTTLE_EMPTY;
    }
    INV_SLOT(FISHMOD_BOTTLE_SLOT(0)) = ITEM_BOTTLE_LARVA_3;
    INV_SLOT(FISHMOD_BOTTLE_SLOT(1)) = ITEM_BOTTLE_LARVA_3;
    INV_SLOT(FISHMOD_BOTTLE_SLOT(2)) = ITEM_BOTTLE_WORM;
    FISHMOD_SAVE->bottleFish[2].lengthTenths = FISHMOD_WORMS_PER_BOTTLE;
    INV_SLOT(FISHMOD_BOTTLE_SLOT(3)) = ITEM_BOTTLE_BUG;
    INV_SLOT(FISHMOD_BOTTLE_SLOT(4)) = ITEM_BOTTLE_LARVA_3;
    // the rod, and the tackle (the pond lure, the sinking lure, the sinker)
    INV_SLOT(SLOT_FISHING_ROD) = ITEM_FISHING_POLE;
    INV_SLOT(SLOT_LURE_1) = ITEM_LURE_POND;
    INV_SLOT(SLOT_LURE_2) = ITEM_LURE_SINKING;
    INV_SLOT(SLOT_LURE_3) = ITEM_SINKER;

    // the Lens of Truth (it sees through the lake's water: Fishmod_WaterFogSetup) on D-pad Up at both ages, and
    // double magic, full
    inv->items[SLOT_LENS_OF_TRUTH] = ITEM_LENS_OF_TRUTH;
    FISHMOD_SAVE->dpad[0][0] = SLOT_LENS_OF_TRUTH;
    FISHMOD_SAVE->dpad[1][0] = SLOT_LENS_OF_TRUTH;
    gSaveContext.save.info.playerData.isMagicAcquired = true;
    gSaveContext.save.info.playerData.isDoubleMagicAcquired = true;
    gSaveContext.save.info.playerData.magic = MAGIC_DOUBLE_METER;

    // the slingshot knocks down beehives for larvae
    inv->items[SLOT_SLINGSHOT] = ITEM_SLINGSHOT;
    Inventory_ChangeUpgrade(UPG_BULLET_BAG, 1);
    inv->ammo[SLOT_SLINGSHOT] = 30;
    inv->items[SLOT_DEKU_NUT] = ITEM_DEKU_NUT;
    inv->ammo[SLOT_DEKU_NUT] = 20;
    Inventory_ChangeUpgrade(UPG_DEKU_NUTS, 1);
    inv->items[SLOT_DEKU_STICK] = ITEM_DEKU_STICK;
    inv->ammo[SLOT_DEKU_STICK] = 10;
    Inventory_ChangeUpgrade(UPG_DEKU_STICKS, 1);

    // the Goron's Bracelet lifts rocks for bugs (and lengthens the cast)
    Inventory_ChangeUpgrade(UPG_STRENGTH, 1);
    Inventory_ChangeUpgrade(UPG_WALLET, 1);
    gSaveContext.save.info.playerData.rupees = 200;

    // Kokiri Sword and Deku Shield, equipped
    inv->equipment |= OWN_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_KOKIRI) |
                      OWN_EQUIP(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_DEKU);
    // for Lake Hylia as adult (test mode warps there grown up): the lake full, and his gear
    inv->equipment |= OWN_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER) |
                      OWN_EQUIP(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_HYLIAN) |
                      OWN_EQUIP(EQUIP_TYPE_TUNIC, EQUIP_INV_TUNIC_ZORA) | OWN_EQUIP(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_IRON);
    SET_EVENTCHKINF(EVENTCHKINF_RESTORED_LAKE_HYLIA);
    // and the Water Temple beaten, as it is when the lake's full (vanilla's rain over the drained lake goes with it:
    // without it, walking toward the full lake clouded the sky over and brought the rain)
    SET_EVENTCHKINF(EVENTCHKINF_4A);
    SET_EVENTCHKINF(EVENTCHKINF_B1); // Lake Hylia's intro cutscene seen (it took the camera on the first warp there)
    gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_KOKIRI;
    Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
    Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_DEKU);
    gSaveContext.save.info.equips.buttonItems[1] = ITEM_SLINGSHOT;
    gSaveContext.save.info.equips.cButtonSlots[0] = SLOT_SLINGSHOT;
    // the rod and the bugs ready on C-Down and C-Right: cast straight away (L + D-pad Right calls a fish to the bait)
    gSaveContext.save.info.equips.buttonItems[2] = ITEM_FISHING_POLE;
    gSaveContext.save.info.equips.cButtonSlots[1] = SLOT_FISHING_ROD;
    gSaveContext.save.info.equips.buttonItems[3] = ITEM_BOTTLE_BUG;
    gSaveContext.save.info.equips.cButtonSlots[2] = SLOT_BOTTLE_4;
    Fishmod_TestStart();
}

/** Test mode's starting place (FISHMOD_TEST_START): a new test file, and a test file loaded (Sram_OpenSave). */
void Fishmod_TestStart(void) {
#if FISHMOD_TEST_START == 1
    gSaveContext.save.entranceIndex = ENTR_KOKIRI_FOREST_0;
    gSaveContext.save.linkAge = LINK_AGE_CHILD;
    Fishmod_TestSetAge(false);
#elif FISHMOD_TEST_START == 2
    gSaveContext.save.entranceIndex = ENTR_LAKE_HYLIA_0;
    gSaveContext.save.linkAge = LINK_AGE_ADULT;
    Fishmod_TestSetAge(true);
    // (a test file from an earlier build too: the lake's full, so the Water Temple's beaten, its rain gone)
    SET_EVENTCHKINF(EVENTCHKINF_RESTORED_LAKE_HYLIA);
    SET_EVENTCHKINF(EVENTCHKINF_4A);
#elif FISHMOD_TEST_START == 3
    // Lake Hylia as child (the lake is full for him whatever the flag)
    gSaveContext.save.entranceIndex = ENTR_LAKE_HYLIA_0;
    gSaveContext.save.linkAge = LINK_AGE_CHILD;
    Fishmod_TestSetAge(false);
#elif FISHMOD_TEST_START == 4
    // Lake Hylia drained: adult, the Water Temple not cleared (Fishmod_LakeDrained)
    gSaveContext.save.entranceIndex = ENTR_LAKE_HYLIA_0;
    gSaveContext.save.linkAge = LINK_AGE_ADULT;
    Fishmod_TestSetAge(true);
    CLEAR_EVENTCHKINF(EVENTCHKINF_RESTORED_LAKE_HYLIA);
    CLEAR_EVENTCHKINF(EVENTCHKINF_4A); // (vanilla's rain over the drained lake, as it should be)
#endif
}
#endif

#if FISHMOD_TESTMODE || FISHMOD_DEBUG
FishmodRdpCounters gFishmodRdp;

/**
 * The RDP's own counters since the last call (cycles at the RDP's clock, 62.5 MHz): all cycles, cycles its command
 * buffer was busy (fetching commands from RDRAM, or held up), its pipeline was drawing, and TMEM was being loaded.
 * Only the console has them; emulators read 0 or guesses. Once a frame.
 */
void Fishmod_SampleRdp(void) {
    gFishmodRdp.clock = IO_READ(DPC_CLOCK_REG) & 0xFFFFFF;
    gFishmodRdp.buf = IO_READ(DPC_BUFBUSY_REG) & 0xFFFFFF;
    gFishmodRdp.pipe = IO_READ(DPC_PIPEBUSY_REG) & 0xFFFFFF;
    gFishmodRdp.tmem = IO_READ(DPC_TMEM_REG) & 0xFFFFFF;
#if defined(F3DEX3_ALL)
    {
        extern u32 gFishmodRspCountersSet;

        if (gFishmodRspCountersSet == 0) {
            IO_WRITE(DPC_STATUS_REG, DPC_CLR_CLOCK_CTR | DPC_CLR_CMD_CTR | DPC_CLR_PIPE_CTR | DPC_CLR_TMEM_CTR);
        }
    }
#elif !defined(F3DEX3_PROF)
    IO_WRITE(DPC_STATUS_REG, DPC_CLR_CLOCK_CTR | DPC_CLR_CMD_CTR | DPC_CLR_PIPE_CTR | DPC_CLR_TMEM_CTR);
#endif
    // (F3DEX3's profiling builds time themselves with the clock counter: sched.c clears them all as each graphics
    // task starts instead, so these are the task's)
}
#endif

#if FISHMOD_TESTMODE
void Fishmod_DrawFps(Gfx** gfxP) {
    // average of the last 20 frame periods (the time between graph updates), updated every 20 frames
    static u32 sSumUsec;
    static s32 sCount;
    static s32 sFpsTenths;
    static u32 sRdpSum[4];
    static s32 sRdpPct[3];
    GfxPrint printer;
    s32 i;

    Fishmod_SampleRdp();
    sRdpSum[0] += gFishmodRdp.clock;
    sRdpSum[1] += gFishmodRdp.buf;
    sRdpSum[2] += gFishmodRdp.pipe;
    sRdpSum[3] += gFishmodRdp.tmem;

    // nothing extra in the picture the pause menu takes of the screen (it comes out as garbage)
    if (R_PAUSE_BG_PRERENDER_STATE != PAUSE_BG_PRERENDER_OFF) {
        return;
    }
    sSumUsec += (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    if (++sCount >= 20) {
        sFpsTenths = (sSumUsec != 0) ? (s32)(20u * 10000000u / sSumUsec) : 0;
        sSumUsec = 0;
        sCount = 0;
        for (i = 0; i < 3; i++) {
            // (in 1/1024ths first: the sums run to tens of millions)
            sRdpPct[i] = (sRdpSum[0] >> 10) ? (s32)((sRdpSum[1 + i] >> 10) * 100 / (sRdpSum[0] >> 10)) : 0;
        }
        bzero(sRdpSum, sizeof(sRdpSum));
    }
    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, *gfxP);
    GfxPrint_SetColor(&printer, 255, 255, 255, 200);
    GfxPrint_SetPos(&printer, 17, 27); // bottom middle: clear of the hearts, buttons, map and rupees
    GfxPrint_Printf(&printer, "%2d.%dfps", sFpsTenths / 10, sFpsTenths % 10);
    if (sFpsDetail) {
        // RDP busy fetching/held up (buf), drawing (pipe), loading textures (tmem), % of its cycles
        GfxPrint_SetPos(&printer, 10, 26);
        GfxPrint_Printf(&printer, "b%2d p%2d t%2d", sRdpPct[0], sRdpPct[1], sRdpPct[2]);
    }
    *gfxP = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);
}
#endif

/** Link can cast: the rod's controller is there (any scene but the Fishing Pond, whose own actor runs the rod). */
/**
 * How much faster Link pushes and pulls blocks with his strength upgrade: none 1, Goron's Bracelet 1.35, Silver
 * Gauntlets 1.7, Golden Gauntlets 2 (his pace and the block's; first numbers, to be tuned by feel).
 */
f32 Fishmod_PushSpeedScale(void) {
    static const f32 sScale[4] = { 1.0f, 1.35f, 1.7f, 2.0f };

    return sScale[CLAMP(CUR_UPG_VALUE(UPG_STRENGTH), 0, 3)];
}

/**
 * How far casts reach in a scene, against Lake Hylia's: a small pond gets a shorter cast (the strength upgrades still
 * scale it; first numbers, to be tuned by feel).
 */
f32 Fishmod_CastSceneScale(s16 sceneId) {
    switch (sceneId) {
        case SCENE_KOKIRI_FOREST:
            return 0.6f;
        case SCENE_LAKE_HYLIA:
            return 1.0f;
        default:
            return 0.8f;
    }
}

s32 Fishmod_CanUseRod(PlayState* play) {
    return gFishmodRod != NULL;
}

/* Baked water floor maps (fishmod/tools/gen_watermap.py) */

#include "dma.h"
#include "segment_symbols.h"
#include "zelda_arena.h"
#include "tha.h"
#include "fishmod_collision.h"

// the save block fits in the unused scene flags, past every scene of any build (including the old block's place)
static_assert(FISHMOD_SAVE_OLD_SCENE >= SCENE_ID_MAX, "a scene of this build would write over the old save block");
static_assert(FISHMOD_SAVE_SCENE >= SCENE_ID_MAX, "a scene would write over the save block");
static_assert((FISHMOD_SAVE_SCENE - FISHMOD_SAVE_OLD_SCENE) * sizeof(SavedSceneFlags) >= sizeof(FishmodSaveData),
              "the old and new save blocks overlap (the move would need memmove)");
static_assert(FISHMOD_SAVE_SCENE * sizeof(SavedSceneFlags) + sizeof(FishmodSaveData) <=
                  sizeof(gSaveContext.save.info.sceneFlags),
              "the save block doesn't fit in the scene flags");
static_assert(sizeof(FishmodSaveData) == 0xB4, "the save block's layout changed: bump FISHMOD_SAVE_VERSION");
static_assert(FISHMOD_SPECIES_MAX <= FISHMOD_SAVE_SPECIES_MAX, "more species than the save keeps records for");

/**
 * Bring a loaded (or new) file's save block up to this build: moved from where old builds kept it, then the
 * item page's six new slots set up empty (a new file, or one from before them).
 */
void Fishmod_InitSaveExtras(void) {
    FishmodSaveData* save = FISHMOD_SAVE;
    s32 i;

    if (save->magic != FISHMOD_SAVE_MAGIC) {
        // a save from before the move, or a new file (whose old place is zeros too): no scene of this build has
        // flags at the old place, so whatever is there is fishmod's
        FishmodSaveData* old = (FishmodSaveData*)&gSaveContext.save.info.sceneFlags[FISHMOD_SAVE_OLD_SCENE];

        bcopy(old, save, sizeof(FishmodSaveData));
        bzero(old, sizeof(FishmodSaveData));
        save->magic = FISHMOD_SAVE_MAGIC;
        save->version = FISHMOD_SAVE_VERSION;
    }

    if (!(save->flags & FISHMOD_SAVE_FLAG_ITEMS)) {
        for (i = 0; i < ARRAY_COUNT(FISHMOD_SAVE->items); i++) {
            FISHMOD_SAVE->items[i] = ITEM_NONE;
        }
        FISHMOD_SAVE->flags |= FISHMOD_SAVE_FLAG_ITEMS;
    }
    if (!(save->flags & FISHMOD_SAVE_FLAG_DPAD)) {
        // the D-pad's buttons, empty (a new file, or one from before them)
        memset(FISHMOD_SAVE->dpad, SLOT_NONE, sizeof(FISHMOD_SAVE->dpad));
        FISHMOD_SAVE->flags |= FISHMOD_SAVE_FLAG_DPAD;
    }
    save->version = FISHMOD_SAVE_VERSION;
}

#define FISHMOD_WATERMAP(sceneId, segment) DECLARE_ROM_SEGMENT(segment)
#include "fishmod_watermap_scenes.h"
#undef FISHMOD_WATERMAP

typedef struct FishmodWaterMapRom {
    s16 sceneId;
    uintptr_t romStart;
    uintptr_t romEnd;
} FishmodWaterMapRom;

static const FishmodWaterMapRom sWaterMaps[] = {
#define FISHMOD_WATERMAP(sceneId, segment) \
    { sceneId, (uintptr_t)_##segment##SegmentRomStart, (uintptr_t)_##segment##SegmentRomEnd },
#include "fishmod_watermap_scenes.h"
#undef FISHMOD_WATERMAP
};

#define FISHMOD_SCATTER(sceneId, segment) DECLARE_ROM_SEGMENT(segment)
#include "fishmod_scatter_scenes.h"
#undef FISHMOD_SCATTER

static const FishmodWaterMapRom sScatters[] = {
#define FISHMOD_SCATTER(sceneId, segment) \
    { sceneId, (uintptr_t)_##segment##SegmentRomStart, (uintptr_t)_##segment##SegmentRomEnd },
#include "fishmod_scatter_scenes.h"
#undef FISHMOD_SCATTER
};

#define FISHMOD_BEDCARD(sceneId, segment) DECLARE_ROM_SEGMENT(segment)
#include "fishmod_bedcard_scenes.h"
#undef FISHMOD_BEDCARD

static const FishmodWaterMapRom sBedCards[] = {
#define FISHMOD_BEDCARD(sceneId, segment) \
    { sceneId, (uintptr_t)_##segment##SegmentRomStart, (uintptr_t)_##segment##SegmentRomEnd },
#include "fishmod_bedcard_scenes.h"
#undef FISHMOD_BEDCARD
};

s32 Fishmod_BedCardRom(s16 sceneId, uintptr_t* romStart, uintptr_t* romEnd) {
    s32 i;

    if (!FISHMOD_PROFILE_BED_CARDS) {
        return false; // (off in the profile: they cost more than they saved on the console)
    }

    for (i = 0; i < ARRAY_COUNT(sBedCards); i++) {
        if (sBedCards[i].sceneId == sceneId) {
            *romStart = sBedCards[i].romStart;
            *romEnd = sBedCards[i].romEnd;
            return true;
        }
    }
    return false;
}

#define FISHMOD_COLLISION(sceneId, segment, extraMem, gridCells) DECLARE_ROM_SEGMENT(segment)
#include "fishmod_collision_scenes.h"
#undef FISHMOD_COLLISION

static const FishmodWaterMapRom sPropCollisions[] = {
#define FISHMOD_COLLISION(sceneId, segment, extraMem, gridCells) \
    { sceneId, (uintptr_t)_##segment##SegmentRomStart, (uintptr_t)_##segment##SegmentRomEnd },
#include "fishmod_collision_scenes.h"
#undef FISHMOD_COLLISION
};

typedef struct FishmodCollisionSize {
    s16 sceneId;
    u8 gridCells;
    u32 extraMem;
} FishmodCollisionSize;

static const FishmodCollisionSize sPropCollisionSizes[] = {
#define FISHMOD_COLLISION(sceneId, segment, extraMem, gridCells) { sceneId, gridCells, extraMem },
#include "fishmod_collision_scenes.h"
#undef FISHMOD_COLLISION
};

/**
 * BgCheck's sizes for a scene with prop collision (fishmod_collision_scenes.h): more memory for the props' polygons,
 * and a finer lookup grid across x and z. Called once BgCheck has chosen the scene's own.
 */
void Fishmod_SizeCollision(s16 sceneId, u32* memSize, Vec3i* subdivAmount) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sPropCollisionSizes); i++) {
        if (sPropCollisionSizes[i].sceneId == sceneId) {
            *memSize += sPropCollisionSizes[i].extraMem;
            if (sPropCollisionSizes[i].gridCells != 0) {
                subdivAmount->x = sPropCollisionSizes[i].gridCells;
                subdivAmount->z = sPropCollisionSizes[i].gridCells;
            }
            return;
        }
    }
}

/**
 * Adds the scene's prop collision (its rocks, as chunky hulls) to the scene's own: new vertex and polygon lists,
 * vanilla's followed by the props', in the game state's memory (like BgCheck's own tables), so they last the scene.
 * Called as the scene's collision header is read, before BgCheck builds its lookup from it.
 */
void Fishmod_AddPropCollision(PlayState* play, CollisionHeader* colHeader) {
    s32 i;

#if FISHMOD_DEBUG
    if (gFishmodNoPropCol) {
        return; // (a bench without the rocks' collision, to see what it costs)
    }
#endif

    for (i = 0; i < ARRAY_COUNT(sPropCollisions); i++) {
        if (sPropCollisions[i].sceneId == play->sceneId) {
            size_t size = sPropCollisions[i].romEnd - sPropCollisions[i].romStart;
            FishmodColHeader* col = THA_AllocTailAlign16(&play->state.tha, size);
            Vec3s* extraVtx;
            FishmodColPoly* extraPoly;
            Vec3s* vtx;
            CollisionPoly* poly;
            s32 n;

            if (col == NULL) {
                return;
            }
            DmaMgr_RequestSync(col, sPropCollisions[i].romStart, size);
            if ((col->magic != FISHMOD_COL_MAGIC) ||
                (colHeader->numVertices + col->numVertices > COLPOLY_VTX_INDEX(0xFFFF)) ||
                (colHeader->numPolygons + col->numPolygons > 0x7FFF)) {
                return;
            }
            extraVtx = (Vec3s*)(col + 1);
            extraPoly = (FishmodColPoly*)(extraVtx + col->numVertices);
            vtx = THA_AllocTailAlign16(&play->state.tha, (colHeader->numVertices + col->numVertices) * sizeof(Vec3s));
            poly = THA_AllocTailAlign16(&play->state.tha,
                                        (colHeader->numPolygons + col->numPolygons) * sizeof(CollisionPoly));
            if ((vtx == NULL) || (poly == NULL)) {
                return;
            }
            bcopy(colHeader->vtxList, vtx, colHeader->numVertices * sizeof(Vec3s));
            bcopy(extraVtx, vtx + colHeader->numVertices, col->numVertices * sizeof(Vec3s));
            bcopy(colHeader->polyList, poly, colHeader->numPolygons * sizeof(CollisionPoly));
            for (n = 0; n < col->numPolygons; n++) {
                CollisionPoly* p = &poly[colHeader->numPolygons + n];
                FishmodColPoly* e = &extraPoly[n];

                // the surface (footsteps, sound): picked at build time
                p->type = e->type;
                p->vtxData[0] = COLPOLY_VTX(colHeader->numVertices + e->vtx[0], 0);
                p->vtxData[1] = COLPOLY_VTX(colHeader->numVertices + e->vtx[1], 0);
                p->vtxData[2] = colHeader->numVertices + e->vtx[2];
                p->normal = e->normal;
                p->dist = e->dist;
            }
            colHeader->vtxList = vtx;
            colHeader->polyList = poly;
            colHeader->numVertices += col->numVertices;
            colHeader->numPolygons += col->numPolygons;
            return;
        }
    }
}

FishmodScatterHeader* Fishmod_LoadScatter(s16 sceneId) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sScatters); i++) {
        if (sScatters[i].sceneId == sceneId) {
            size_t size = sScatters[i].romEnd - sScatters[i].romStart;
            FishmodScatterHeader* sc = ZeldaArena_Malloc(size);

            if (sc == NULL) {
                return NULL;
            }
            DmaMgr_RequestSync(sc, sScatters[i].romStart, size);
            if (sc->magic != FISHMOD_SCATTER_MAGIC) {
                ZeldaArena_Free(sc);
                return NULL;
            }
            return sc;
        }
    }
    return NULL;
}

FishmodWaterMapHeader* Fishmod_LoadWaterMap(s16 sceneId) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sWaterMaps); i++) {
        if (sWaterMaps[i].sceneId == sceneId) {
            size_t size = sWaterMaps[i].romEnd - sWaterMaps[i].romStart;
            FishmodWaterMapHeader* map = ZeldaArena_Malloc(size);

            if (map == NULL) {
                return NULL;
            }
            DmaMgr_RequestSync(map, sWaterMaps[i].romStart, size);
            if (map->magic != FISHMOD_WATERMAP_MAGIC) {
                ZeldaArena_Free(map);
                return NULL;
            }
            return map;
        }
    }
    return NULL;
}

/*
 * Time (fishmod/docs/v5-plan.md, phase 2)
 *
 * - A 20 minute day: about 12 minutes from 6:30 to 18:00 and 8 minutes of night, at the field speed (10). Each frame's
 *   step is scaled with a fraction carried over between frames (the engine's speed is a whole number). Sun's Song
 *   (speed 400 and up) isn't scaled. The Fishing Pond (speed 1) runs on the same clock as the field.
 * - Places where vanilla stops the clock (speed 0: Kokiri Forest, towns, indoors) keep a hidden clock running: the
 *   time owed. At each scene change the visible time catches up by at most an hour, but never past the next barrier
 *   (day to dusk at 16:00, night to dawn at 4:00), so frozen places get later in the day or the night but never turn
 *   to dusk or dawn. Sunsets and sunrises only play where time flows; once one does, nothing is owed any more.
 */

#define TIME_DAY_SCALE 56    // per 256: 31403 units of day in 12 minutes at speed 10
#define TIME_NIGHT_SCALE 91  // per 256: 34133 units of night in 8 minutes at speed 10 (vanilla: twice the day's)
#define TIME_FIELD_SPEED 10
#define TIME_STEP_MAX CLOCK_TIME(1, 0)
#define TIME_BARRIER_MARGIN CLOCK_TIME(0, 2)

static u32 sTimeFrac;     // 1/256 units carried to the next frame
static u32 sHiddenFrac;
static u32 sTimeOwed;     // time that passed while the clock was stopped
static u16 sTimeLast;     // the visible time last frame (to see a barrier pass)

/** The time to add this frame, in 1/256 units. */
static u32 Fishmod_TimeStep(u32 speed, s32 day) {
    return speed * (day ? TIME_DAY_SCALE : TIME_NIGHT_SCALE);
}

/** True if going from `from` forward to `to` passes 16:00 or 4:00. */
static s32 Fishmod_TimeCrossesBarrier(u16 from, u16 to) {
    u16 span = to - from;

    return ((u16)(CLOCK_TIME(16, 0) - from) < span) || ((u16)(CLOCK_TIME(4, 0) - from) < span) ||
           (to == CLOCK_TIME(16, 0)) || (to == CLOCK_TIME(4, 0));
}

void Fishmod_AdvanceTime(PlayState* play) {
    u16 speed = gTimeSpeed;
    s32 day = IS_DAY;

    if (gSaveContext.gameMode != GAMEMODE_NORMAL) {
        // the title screen and the credits: vanilla's clock (their scenes are timed to it)
        gSaveContext.save.dayTime += gTimeSpeed;
        return;
    }

    if (play->sceneId == SCENE_FISHING_POND) {
        speed = TIME_FIELD_SPEED;
    }
    if (speed >= 400) {
        // Sun's Song: as vanilla
        gSaveContext.save.dayTime += speed;
    } else if (speed != 0) {
        sTimeFrac += Fishmod_TimeStep(speed, day);
        gSaveContext.save.dayTime += sTimeFrac >> 8;
        sTimeFrac &= 0xFF;
    } else {
        // the clock is stopped here: the hidden clock runs at the field's speed
        sHiddenFrac += Fishmod_TimeStep(TIME_FIELD_SPEED, day);
        sTimeOwed += sHiddenFrac >> 8;
        sHiddenFrac &= 0xFF;
        if (sTimeOwed > 0x10000) {
            sTimeOwed = 0x10000;
        }
    }
}

/** Once a frame: a sunset or sunrise that played out (or Sun's Song) means nothing is owed any more. */
/*
 * Underwater haze: with the camera under the surface, the fog comes in closer than vanilla's underwater fog (the
 * water's own lights still give its colour), easing in and out over a few frames. Only the fog: the far plane
 * (and what the scene draws) stays vanilla's. gFishmodHaze: 0 above water, 1 under.
 */
#define HAZE_FOG_NEAR 945 // the fog's start (of 1000, with vanilla's far plane; vanilla's water is about 990).
                          // 953 before the frame-rate pass: a little murkier, so the tall plants' 900 cut hides
f32 gFishmodHaze;

static void Fishmod_UpdateHaze(PlayState* play) {
    Vec3f eye = play->view.eye;
    WaterBox* box;
    f32 surface;
    s32 under = false;
    EnvironmentContext* env = &play->envCtx;

    if ((eye.x == eye.x) && (eye.z == eye.z) && (fabsf(eye.x) < 30000.0f) && (fabsf(eye.z) < 30000.0f)) {
        under = BgCheck_GetWaterSurface(play, &play->colCtx, eye.x, eye.z, &surface, &box) && (eye.y < surface);
    }
#if FISHMOD_DEBUG
    if (gFishmodPop.haze >= 0.0f) {
        // (the pop-in test: the haze held where the camera crosses the surface)
        gFishmodHaze = gFishmodPop.haze;
    } else
#endif
    {
        if (!under && (gFishmodHaze <= 0.0f)) {
            return;
        }
        Math_StepToF(&gFishmodHaze, under ? 1.0f : 0.0f, 0.1f);
    }
    env->adjFogNear = (env->lightSettings.fogNear > HAZE_FOG_NEAR)
                          ? (s16)((HAZE_FOG_NEAR - env->lightSettings.fogNear) * gFishmodHaze)
                          : 0;
}

void Fishmod_TimeUpdate(PlayState* play) {
    u16 now = gSaveContext.save.dayTime;

    Fishmod_UpdateHaze(play);

    if ((now != sTimeLast) && Fishmod_TimeCrossesBarrier(sTimeLast, now)) {
        sTimeOwed = 0;
    }
    sTimeLast = now;
}

/** At every scene change: the visible time catches up a little with the time owed, up to the next barrier. */
void Fishmod_TimeOnSceneExit(void) {
    u16 now = gSaveContext.save.dayTime;
    u16 toBarrier;
    u32 step;

    if (sTimeOwed == 0) {
        return;
    }
    // the next barrier ahead of now
    toBarrier = (u16)(CLOCK_TIME(16, 0) - now);
    if ((u16)(CLOCK_TIME(4, 0) - now) < toBarrier) {
        toBarrier = (u16)(CLOCK_TIME(4, 0) - now);
    }
    if (toBarrier <= TIME_BARRIER_MARGIN) {
        return; // waiting at a barrier: only flowing time takes it past
    }
    step = MIN(sTimeOwed, TIME_STEP_MAX);
    step = MIN(step, (u32)(toBarrier - TIME_BARRIER_MARGIN));
    gSaveContext.save.dayTime = now + step;
    sTimeOwed -= step;
    sTimeLast = gSaveContext.save.dayTime;
}

/*
 * Weather: a roll per day (the same all day, and the same everywhere under the open sky: it was a roll per fishing
 * spot, so walking from Hyrule Field, always clear, into Lake Hylia could turn the sky on the spot): mostly clear,
 * sometimes overcast, rarely rain. Set up the way the engine's own rainy weather is (darker sky and light, rain
 * falling). The Song of Storms still brings rain on top of it.
 */
#define WEATHER_RAIN_CHANCE 10     // percent of days
#define WEATHER_OVERCAST_CHANCE 20

static u8 sWeather; // FishmodWeather, for the scene being played

s32 Fishmod_WeatherRoll(s16 sceneId, s32 day) {
    // a small hash of the spot and the day (the game's random numbers stay untouched)
    u32 h = (u32)sceneId * 0x9E3779B1u + (u32)day * 0x85EBCA77u;
    u32 roll;

    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    roll = h % 100;
    if (roll < WEATHER_RAIN_CHANCE) {
        return FISHMOD_WEATHER_RAIN;
    }
    if (roll < WEATHER_RAIN_CHANCE + WEATHER_OVERCAST_CHANCE) {
        return FISHMOD_WEATHER_OVERCAST;
    }
    return FISHMOD_WEATHER_CLEAR;
}

/**
 * Two weathers: the day's (Fishmod_WeatherDay: one roll per day, a function of the day's number, so there's nothing to
 * keep and nothing rolled again from scene to scene) and the scene's (sWeather: what's over Link's head and what the
 * fish see). The scene's is the day's wherever the scene can show it, clear where it can't, or the scene's own where
 * the game gives one to all of it (the drained lake's rain: Fishmod_WeatherSetScene), and goes back to the day's on
 * leaving.
 */
static u8 sWeatherSceneOwned; // a whole-scene weather of the game's is up (it set gWeatherMode): it ends with the scene

s32 Fishmod_WeatherDay(void) {
    if (FISHMOD_SAVE->clearDay == (u16)(gSaveContext.save.totalDays + 1)) {
        return FISHMOD_WEATHER_CLEAR; // (the day the lake came back)
    }
    return Fishmod_WeatherRoll(0, gSaveContext.save.totalDays);
}

/**
 * Whether this scene, as it's set up now, can show the day's weather: the engine's cloudy sky and its darker light
 * are light set 2 of a scene lit by the time of day (the sets its day, dusk, night and dawn come from: 8 to 11 of its
 * list), so only a scene under the open sky, lit that way, with those sets. Not a cutscene's set-up of a scene (they
 * have their own, shorter light lists, and their own mood: vanilla doesn't carry its weather into them either), not
 * a scene with one fixed light (the last fight, the castle's courtyards), not the title screen or the credits.
 */
#define WEATHER_LIGHT_SETS_NEEDED 12

static s32 Fishmod_WeatherSceneTakesDay(PlayState* play, s16 skyboxId) {
    return (skyboxId == SKYBOX_NORMAL_SKY) && (play->envCtx.lightMode == LIGHT_MODE_TIME) &&
           (play->envCtx.numLightSettings >= WEATHER_LIGHT_SETS_NEEDED) && !IS_CUTSCENE_LAYER &&
           (gSaveContext.gameMode == GAMEMODE_NORMAL);
}

/**
 * The weather of ours this scene gets: the day's, where it can be shown and no vanilla weather is over it. Asked by
 * the skybox too, before it loads its textures (Skybox_Setup): a cloudy day's sky loaded as such, not loaded clear
 * and then streamed over while it's drawn (the noise in the sky after a scene change).
 */
s32 Fishmod_WeatherToday(PlayState* play, s16 skyboxId, s32 beforeEnvInit) {
    // (a vanilla weather carried into the scene: Environment_Init keeps gWeatherMode only if it's retained, so the
    // skybox, asked before that, reads it as it will be)
    if (!Fishmod_WeatherSceneTakesDay(play, skyboxId) ||
        ((gWeatherMode != WEATHER_MODE_CLEAR) && (!beforeEnvInit || gSaveContext.retainWeatherMode))) {
        return FISHMOD_WEATHER_CLEAR;
    }
    return Fishmod_WeatherDay();
}

/**
 * A scene's being loaded (before its sky and light are set up): a weather the game gave the whole of the last scene
 * stays there. Vanilla's lake rain was a zone that had cleared by the exits; over the whole scene it would be carried
 * out on foot (gWeatherMode kept across a walked exit) and never clear.
 */
static void Fishmod_WeatherLeaveScene(void) {
    if (sWeatherSceneOwned) {
        sWeatherSceneOwned = false;
        gWeatherMode = WEATHER_MODE_CLEAR;
        // (its rain's sound: the day's own rain, if any, starts it again once the scene runs)
        Audio_SetNatureAmbienceChannelIO(NATURE_CHANNEL_RAIN, CHANNEL_IO_PORT_1, 0);
        Audio_SetNatureAmbienceChannelIO(NATURE_CHANNEL_LIGHTNING, CHANNEL_IO_PORT_1, 0);
    }
    sWeather = FISHMOD_WEATHER_CLEAR;
}

void Fishmod_OnEnvironmentInit(PlayState* play) {
    EnvironmentContext* envCtx = &play->envCtx;

    sRainSoundPending = false;
    sWeather = Fishmod_WeatherToday(play, play->skyboxId, false);
    if (sWeather == FISHMOD_WEATHER_CLEAR) {
        return;
    }
    // the cloudy sky and the darker light set (the scene has it: Fishmod_WeatherSceneTakesDay)
    envCtx->skyboxConfig = 1;
    envCtx->changeSkyboxNextConfig = 1;
    envCtx->lightConfig = 2;
    envCtx->changeLightNextConfig = 2;
    if (sWeather == FISHMOD_WEATHER_RAIN) {
        envCtx->precipitation[PRECIP_RAIN_MAX] = 20;
        envCtx->precipitation[PRECIP_RAIN_CUR] = 20;
        envCtx->stormState = STORM_STATE_ON;
        sRainSoundPending = true;
    }
}

/**
 * Where the game's own weather (a weather tag, z_en_weather_tag.c, by its type) covers the whole scene rather than a
 * zone in it: the drained Lake Hylia's rain, the frozen Zora's Domain's and Fountain's snow, Lon Lon Ranch's clouds
 * (the ranch itself; Hyrule Field's are a zone by the ranch). Its own conditions (the Water Temple, Epona) are the
 * tag's, as the game has them.
 */
s32 Fishmod_WeatherWholeScene(s16 sceneId, s32 tagType) {
    static const struct {
        s16 sceneId;
        u8 tagType;
    } sWhole[] = {
        { SCENE_LAKE_HYLIA, 3 },     // EN_WEATHER_TAG_TYPE_RAIN_LAKE_HYLIA
        { SCENE_ZORAS_DOMAIN, 2 },   // EN_WEATHER_TAG_TYPE_SNOW_ZORAS_DOMAIN
        { SCENE_ZORAS_FOUNTAIN, 2 },
        { SCENE_LON_LON_RANCH, 1 },  // EN_WEATHER_TAG_TYPE_CLOUDY_LON_LON_RANCH
    };
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sWhole); i++) {
        if ((sWhole[i].sceneId == sceneId) && (sWhole[i].tagType == tagType)) {
            return true;
        }
    }
    return false;
}

/** The scene's weather is the game's own, over all of it (a whole-scene weather tag): what the fish see. */
void Fishmod_WeatherSetScene(PlayState* play, s32 weather) {
    (void)play;
    sWeather = weather;
    sWeatherSceneOwned = true;
}

/** The rest of today clear, everywhere (the Water Temple beaten: vanilla brings the morning and clears the sky). */
void Fishmod_WeatherClearToday(void) {
    FISHMOD_SAVE->clearDay = (u16)(gSaveContext.save.totalDays + 1);
    sWeather = FISHMOD_WEATHER_CLEAR;
}

/** The day's weather as this scene shows it (the Song of Storms not counted): vanilla's weather zones go back to it. */
s32 Fishmod_WeatherRolled(void) {
    return sWeather;
}

s32 Fishmod_Weather(PlayState* play) {
    // the Song of Storms (or any rain) counts as rain while it lasts
    if (play->envCtx.precipitation[PRECIP_RAIN_CUR] > 0) {
        return FISHMOD_WEATHER_RAIN;
    }
    return sWeather;
}

/**
 * How bright it is for the fish, 0 (dark night) to 1 (full day): dawn from 4:00 to 8:00, dusk from 16:00 to 19:00
 * (the engine's light table), dimmer under clouds and rain.
 */
f32 Fishmod_Light(PlayState* play) {
    s32 t = gSaveContext.save.dayTime;
    f32 light;

    if ((t >= CLOCK_TIME(8, 0)) && (t < CLOCK_TIME(16, 0))) {
        light = 1.0f;
    } else if ((t >= CLOCK_TIME(4, 0)) && (t < CLOCK_TIME(8, 0))) {
        light = 0.15f + 0.85f * (t - CLOCK_TIME(4, 0)) / (f32)(CLOCK_TIME(8, 0) - CLOCK_TIME(4, 0));
    } else if ((t >= CLOCK_TIME(16, 0)) && (t < CLOCK_TIME(19, 0))) {
        light = 1.0f - 0.85f * (t - CLOCK_TIME(16, 0)) / (f32)(CLOCK_TIME(19, 0) - CLOCK_TIME(16, 0));
    } else {
        light = 0.15f;
    }
    switch (Fishmod_Weather(play)) {
        case FISHMOD_WEATHER_OVERCAST:
            light *= 0.75f;
            break;
        case FISHMOD_WEATHER_RAIN:
            light *= 0.6f;
            break;
    }
    return light;
}

// (GCC's 64-bit helpers, strcpy: src/gcc_fix/missing_gcc_functions.c, in the boot segment: GCC's code may call
// them before the code segment is loaded)

/**
 * The display lists' next cache lines claimed before they're written (frames11): a store to a line that isn't in the
 * data cache first reads it from RDRAM, for nothing when the whole line is about to be written. The VR4300's Create
 * Dirty Exclusive (cache op 0x0D) makes the line without the read, as the matrices (sys_matrix.c) and the audio's
 * command list (synthesis.c) already do. Called before each big writer (an actor's draw, a room, the fish, the props).
 *
 * Only lines strictly ahead of each list's write pointer (none of their bytes written yet) and wholly below its arena's
 * tail (where GRAPH_ALLOC'd matrices and vertices live, already written): a claimed line holds garbage until written,
 * and would put it back over anything real in RDRAM. CLAIM_AHEAD bytes at a time, each list's watermark kept so a line
 * is claimed once.
 */
#define CLAIM_AHEAD 256

static void Fishmod_ClaimArena(TwoHeadGfxArena* arena, u32* claimed) {
#if defined(__GNUC__) && !defined(FISHMOD_NO_CLAIM)
    u32 head = ((u32)arena->p + 15) & ~15; // (the first line none of whose bytes is written)
    u32 tail = (u32)arena->d & ~15;         // (lines from here up hold allocations)
    u32 end = head + CLAIM_AHEAD;
    u32 p = *claimed;

    if ((p < head) || (p > tail)) {
        p = head; // (a new frame, another buffer, or the tail came down past it)
    }
    if (end > tail) {
        end = tail;
    }
    for (; p < end; p += 16) {
        __asm__ volatile("cache 0x0D, 0(%0)" : : "r"(p) : "memory");
    }
    *claimed = p; // (only ever moves on within a frame's list, or starts again at its head)
#endif
}

void Fishmod_ClaimDisplayLists(GraphicsContext* gfxCtx) {
    static u32 sClaimed[3];

#ifdef FISHMOD_NO_DLCLAIM // (console bisection: frames11's display-list claims left out)
    return;
#endif

    Fishmod_ClaimArena(&gfxCtx->polyOpa, &sClaimed[0]);
    Fishmod_ClaimArena(&gfxCtx->polyXlu, &sClaimed[1]);
    Fishmod_ClaimArena(&gfxCtx->overlay, &sClaimed[2]);
}

/*
 * The lake's terrain lit by the CPU (tools/meshpack.py relights the room: its lit pieces load their vertices through
 * segment FISHMOD_RELIT_SEG, with the RSP's lighting off). The room has only the sky's lights (an ambient colour and
 * the directional lights of the hour: Lights_BindAll binds no point light without a position), so a vertex's colour
 * is the RSP's formula, ambient + each light's colour times the cosine between the vertex's normal and the light
 * (none when it faces away), from the lights the game has just set for this frame: the sun moves and the hours change
 * as before. It's computed again whenever those lights change (a channel or a direction), into one of three buffers
 * in turn (the RSP may still be reading the last two frames' while the CPU is ahead), and the segment points at the
 * newest. The colours can differ from the RSP's by a step in a channel (its rounding against these integers).
 */
#include "light.h"

#define FISHMOD_RELIT_SEG 0x0C
#define FISHMOD_RELIT_BUFS 3
#define FISHMOD_RELIT_MIN_FRAMES 10 // a small change in the lights waits this long (half a second at 20 fps)
#define FISHMOD_RELIT_JUMP 4        // a change bigger than this in any byte is lit at once
static s32 sRelitAge;               // frames since the terrain was last lit

extern void* spot06_room_0_fishmodRelit[] __attribute__((weak)); // (meshpack builds only: else its address is 0)

static Vtx* sRelitBuf[FISHMOD_RELIT_BUFS];
static const u32* sRelitSrcs; // each vertex's source in the room (its segment address): its normal is read there
static s8* sRelitNormal;      // the sources' normals gathered in one array (on load, and when they change)
static u8* sRelitRoom;        // (the room's segment, for the sources)
u32 gFishmodRelitCount;       // (the times the terrain was lit again: a check that it isn't every frame)
s32 gFishmodRelitDirty;       // set when a room vertex's normal changes (fishmod_scatter.c's bed shading): light again
static const Vtx* sRelitSrc;
static s32 sRelitCount;
FishmodWaterFog gFishmodWaterFog; // the water fog seen from above, this frame (Fishmod_WaterFogSetup)
static u8* sRelitDepth;        // each vertex's share of the light left by its depth under the surface (Fishmod_DepthShade)
static s32 sRelitDrained = -1; // the lake drained when they were made (no water: no darkening)
static s32 sRelitFront = -1;
static u8 sRelitKey[64]; // the lights last used (to see when they change)
static s32 sRelitKeyLen = -1;

static s32 Fishmod_RelitSame(const u8* a, const void* b, s32 len) {
    const u8* c = b;
    s32 i;

    for (i = 0; i < len; i++) {
        if (a[i] != c[i]) {
            return false;
        }
    }
    return true;
}

static void Fishmod_RelitFree(void) {
    s32 i;

    for (i = 0; i < FISHMOD_RELIT_BUFS; i++) {
        if (sRelitBuf[i] != NULL) {
            ZELDA_ARENA_FREE(sRelitBuf[i], "../fishmod.c", __LINE__);
            sRelitBuf[i] = NULL;
        }
    }
    if (sRelitDepth != NULL) {
        ZELDA_ARENA_FREE(sRelitDepth, "../fishmod.c", __LINE__);
        sRelitDepth = NULL;
    }
    sRelitDrained = -1;
    if (sRelitNormal != NULL) {
        ZELDA_ARENA_FREE(sRelitNormal, "../fishmod.c", __LINE__);
        sRelitNormal = NULL;
    }
    sRelitSrcs = NULL;
    sRelitSrc = NULL;
    sRelitFront = -1;
    sRelitKeyLen = -1;
}

static void Fishmod_RelitCompute(Vtx* out, Lights* lights) {
    s32 n = lights->numLights;
    s32 ar = lights->l.a.l.col[0], ag = lights->l.a.l.col[1], ab = lights->l.a.l.col[2];
    s32 dx[9], dy[9], dz[9], cr[9], cg[9], cb[9];
    s32 i, k;

    if (n > 9) {
        n = 9;
    }
    for (k = 0; k < n; k++) {
        Light_t* l = &lights->l.l[k].l;
        f32 len = sqrtf(SQ((f32)l->dir[0]) + SQ((f32)l->dir[1]) + SQ((f32)l->dir[2]));
        f32 s = (len > 0.0f) ? (127.0f / len) : 0.0f; // (made unit length as the RSP stores it: in a byte, x127)

        dx[k] = (s32)(l->dir[0] * s);
        dy[k] = (s32)(l->dir[1] * s);
        dz[k] = (s32)(l->dir[2] * s);
        cr[k] = l->col[0];
        cg[k] = l->col[1];
        cb[k] = l->col[2];
    }
    for (i = 0; i < sRelitCount; i++) {
        const s8* nm = &sRelitNormal[i * 3];
        s32 r = ar * (128 * 128), g = ag * (128 * 128), b = ab * (128 * 128); // (the dot's scale: both /128, as F3DEX3)
        Vtx* v = &out[i];

        for (k = 0; k < n; k++) {
            s32 d = nm[0] * dx[k] + nm[1] * dy[k] + nm[2] * dz[k];

            if (d > 0) {
                r += cr[k] * d;
                g += cg[k] * d;
                b += cb[k] * d;
            }
        }
        r = r / (128 * 128);
        g = g / (128 * 128);
        b = b / (128 * 128);
        if (sRelitDepth != NULL) {
            // (deep water darker: the light that reaches the bed through it)
            r = (r * sRelitDepth[i]) >> 8;
            g = (g * sRelitDepth[i]) >> 8;
            b = (b * sRelitDepth[i]) >> 8;
        }
        v->v.cn[0] = (r > 255) ? 255 : r;
        v->v.cn[1] = (g > 255) ? 255 : g;
        v->v.cn[2] = (b > 255) ? 255 : b;
    }
}

/*
 * The terrain anti-aliased without reading the picture back: each loaded room's opaque render modes with the colour
 * read (IM_RD) left out, as libultra's "RA" modes and the props' own. The coverage is still worked out and written
 * (the VI smooths the edges with it); what's gone is the RDP reading each pixel's colour before writing it, which only
 * the edge pixels' blend in the RDP used. The console (terrain sweep, 18 laps): 2.9 ms of RDP a frame, every leg; in
 * ares only pixels on polygons' edges change. Found once per room by walking its lists (the room's own: calls into
 * other segments, the scene's animated materials, aren't followed), the bit cleared in place.
 * FISHMOD_BENCH_TERRAIN_AA: the read back, as vanilla (an A/B).
 */
#define TERRAIN_MODES_MAX 512
typedef struct TerrainModes {
    void* room;             // the room's segment they were found in
    s16 scene;              // and which room it held (rooms load into the same buffers in turn)
    s16 num;
    s32 count;
    s32 ra;                 // the bit is out now
    u32* modes[TERRAIN_MODES_MAX];
} TerrainModes;
static TerrainModes sTerrainModes[2]; // (the current room and the previous one, while both are loaded)
static s16 sTerrainScene;             // (the scene the rooms are this frame's)
s32 gFishmodSkyDepthTest; // (fishmod.h: FISHMOD_SKY_DEPTH_TEST)

static void Fishmod_TerrainModesWalk(TerrainModes* tm, void* seg, u32* dl, s32 depth) {
    s32 n;

    for (n = 0; n < 4096; n++, dl += 2) {
        u32 op = dl[0] >> 24;

        if (op == G_ENDDL) {
            return;
        }
        if ((op == G_DL) && (depth < 8)) {
            u32 a = dl[1];

            if (((a >> 24) == 0x03) && ((a & 0xFFFFFF) < 0x100000)) {
                Fishmod_TerrainModesWalk(tm, seg, (u32*)((u8*)seg + (a & 0xFFFFFF)), depth + 1);
            }
            if ((dl[0] >> 16) & 0xFF) {
                return; // (a branch: the list ends there)
            }
        } else if ((dl[0] == 0xE200001C) && (tm->count < TERRAIN_MODES_MAX)) {
            // the render mode: opaque (ZMODE_OPA), anti-aliased, not forced to blend
            u32 m = dl[1];

            if ((m & AA_EN) && ((m & ZMODE_XLU) == ZMODE_OPA) && !(m & FORCE_BL)) {
                s32 i;

                for (i = 0; i < tm->count; i++) {
                    if (tm->modes[i] == &dl[1]) {
                        break;
                    }
                }
                if (i == tm->count) {
                    tm->modes[tm->count++] = &dl[1];
                }
            }
        }
    }
}

static void Fishmod_TerrainRARoom(TerrainModes* tm, Room* room, s32 want) {
    s32 i;

    if ((room->segment == NULL) || (room->roomShape == NULL)) {
        tm->room = NULL;
        tm->count = 0;
        tm->ra = false;
        return;
    }
    if ((tm->room == room->segment) && ((tm->scene != sTerrainScene) || (tm->num != room->num) ||
                                         ((tm->count > 0) && tm->ra && (*tm->modes[0] & IM_RD)))) {
        tm->room = NULL; // (another room in the same buffer, or this one loaded again: its bits as loaded)
    }
    if (tm->room != room->segment) {
        // a room loaded here: its modes as loaded (the bit in). The room's addresses are segment 3's: found from
        // the room's own place, whatever the segment table holds now
        u8* seg = room->segment;
        s32 type = room->roomShape->base.type;

        tm->room = seg;
        tm->scene = sTerrainScene;
        tm->num = room->num;
        tm->count = 0;
        tm->ra = false;
        if (type == ROOM_SHAPE_TYPE_CULLABLE) {
            RoomShapeCullable* shape = &room->roomShape->cullable;
            RoomShapeCullableEntry* e = (RoomShapeCullableEntry*)(seg + ((u32)shape->entries & 0xFFFFFF));

            for (i = 0; i < shape->numEntries; i++) {
                if ((e[i].opa != NULL) && (((u32)e[i].opa >> 24) == 0x03)) {
                    Fishmod_TerrainModesWalk(tm, seg, (u32*)(seg + ((u32)e[i].opa & 0xFFFFFF)), 0);
                }
            }
        } else if (type == ROOM_SHAPE_TYPE_NORMAL) {
            RoomShapeNormal* shape = &room->roomShape->normal;
            RoomShapeDListsEntry* e = (RoomShapeDListsEntry*)(seg + ((u32)shape->entries & 0xFFFFFF));

            for (i = 0; i < shape->numEntries; i++) {
                if ((e[i].opa != NULL) && (((u32)e[i].opa >> 24) == 0x03)) {
                    Fishmod_TerrainModesWalk(tm, seg, (u32*)(seg + ((u32)e[i].opa & 0xFFFFFF)), 0);
                }
            }
        }
    }
    if (want != tm->ra) {
        for (i = 0; i < tm->count; i++) {
            if (want) {
                *tm->modes[i] &= ~IM_RD;
            } else {
                *tm->modes[i] |= IM_RD;
            }
        }
        tm->ra = want;
    }
}

void Fishmod_TerrainRA(PlayState* play) {
    s32 want = !FISHMOD_BENCH_SKIP(FISHMOD_BENCH_TERRAIN_AA);

    sTerrainScene = play->sceneId;
    Room* rooms[2];
    s32 r;

    rooms[0] = &play->roomCtx.curRoom;
    rooms[1] = &play->roomCtx.prevRoom;
    for (r = 0; r < 2; r++) {
        // (each room keeps the slot it was found in while it stays loaded: the current one may become the previous)
        TerrainModes* tm = (sTerrainModes[0].room == rooms[r]->segment) ? &sTerrainModes[0]
                           : (sTerrainModes[1].room == rooms[r]->segment) ? &sTerrainModes[1]
                           : NULL;

        if (tm == NULL) {
            void* other = rooms[r ^ 1]->segment;

            tm = ((sTerrainModes[0].room == NULL) || (sTerrainModes[0].room != other)) ? &sTerrainModes[0]
                                                                                       : &sTerrainModes[1];
            tm->room = NULL; // (made afresh below)
        }
        Fishmod_TerrainRARoom(tm, rooms[r], want);
    }
}

/*
 * Depth darkening: deep water lets less of the light through, so the lake bed, and what stands on it, darkens with
 * its depth under the surface, from FISHMOD_DEPTH_FROM down to FISHMOD_DEPTH_TO, to FISHMOD_DEPTH_MIN of its light.
 * Seen from above the water the surface's tint then lies over a darker bed: the deep water reads as deep, its bottom
 * hard to make out. Baked: the bed's vertices once (their lighting is the CPU's already), the props by where they
 * stand. Nothing when the lake is drained.
 */
s32 Fishmod_DepthShade(f32 depth) {
    f32 f;

    if (depth <= FISHMOD_DEPTH_FROM) {
        return 256;
    }
    f = (depth - FISHMOD_DEPTH_FROM) / (FISHMOD_DEPTH_TO - FISHMOD_DEPTH_FROM);
    if (f > 1.0f) {
        f = 1.0f;
    }
    f = f * (2.0f - f); // (eased: the most change near the top, settling to the floor)
    return (s32)(256.0f * (1.0f - (1.0f - FISHMOD_DEPTH_MIN) * f));
}

s32 Fishmod_LakeDrained(PlayState* play) {
    return (play->sceneId == SCENE_LAKE_HYLIA) && LINK_IS_ADULT && !GET_EVENTCHKINF(EVENTCHKINF_RESTORED_LAKE_HYLIA);
}

/*
 * Water fog: with the camera above the water, what's under the surface is fogged as it would be from under it, by the
 * RSP's own fog, per pixel as the game's is. The camera-under-water fog (Fishmod_UpdateHaze's: its start no further
 * than HAZE_FOG_NEAR, its end FISHMOD_HAZE_FOG_FAR, the water's own light setting's colour) gives a strength by
 * distance; seen from above, the distance that counts is the part of the view's ray inside the water: its distance
 * times the share of it under the surface, depth / (the eye's height over the surface + depth). For a draw whose depth
 * is about the same throughout, that is the RSP's own fog with its two numbers scaled by the share (the RSP works
 * fog out from z / w, which is zA - zB / d at view depth d: the water's at s d is (z / w) fm / s + zA fm (1 - 1 / s)
 * + fo): set before the draw, per pixel by distance, nothing more for the CPU than a few multiplies a draw. The lake
 * bed's pieces are cut into bands by depth (tools/meshpack.py), each band's lists after a list of segment 0x0A's
 * setting its fog (Fishmod_WaterFogBands, from Scene_DrawConfigLakeHylia); each prop under the water sets its own
 * (fishmod_scatter.c). Then the scene's fog again. Nothing above the water changes. Off with the camera under water
 * (the game's own fog is the water's then), with the lake drained, and for the bench's A/B (FISHMOD_BENCH_NOWATERFOG).
 */
static const s16 sWaterBandMid[FISHMOD_WATER_BANDS] = { 10, 35, 75, 140, 242, 360, 470, 620 }; // (meshpack's bands)

void Fishmod_WaterFogSetup(PlayState* play) {
    static u8 sWater[3] = { 30, 70, 70 };
    static s32 sWaterNear = HAZE_FOG_NEAR;
    Vec3f eye = play->view.eye;
    EnvironmentContext* env = &play->envCtx;
    WaterBox* box;
    f32 surface;
    f32 n = play->view.zNear;
    f32 f = play->view.zFar;
    s32 c;

    gFishmodWaterFog.on = (gFishmodHaze <= 0.0f) && (eye.y > FISHMOD_LAKE_SURFACE) &&
                          (play->sceneId == SCENE_LAKE_HYLIA) && !Fishmod_LakeDrained(play) &&
                          !play->actorCtx.lensActive && // (the Lens of Truth sees through the water)
                          !FISHMOD_BENCH_SKIP(FISHMOD_BENCH_NOWATERFOG);
    if (!gFishmodWaterFog.on) {
        return;
    }
    if (BgCheck_GetWaterSurface(play, &play->colCtx, eye.x, eye.z, &surface, &box)) {
        u32 li = WaterBox_GetLightIndex(&play->colCtx, box);

        if ((env->lightSettingsList != NULL) && (li != WATERBOX_LIGHT_INDEX_NONE)) {
            for (c = 0; c < 3; c++) {
                sWater[c] = env->lightSettingsList[li].fogColor[c];
            }
            sWaterNear = env->lightSettingsList[li].blendRateAndFogNear & 0x3FF;
        }
    }
    sWaterNear = MIN(sWaterNear, HAZE_FOG_NEAR);
    for (c = 0; c < 3; c++) {
        gFishmodWaterFog.water[c] = sWater[c];
    }
    Fishmod_WaterTint(gFishmodWaterFog.water, 256);
    gFishmodWaterFog.zA = (f + n) / (f - n);
    gFishmodWaterFog.fm = 128000.0f / (FISHMOD_HAZE_FOG_FAR - sWaterNear);
    gFishmodWaterFog.fo = (500 - sWaterNear) * 256.0f / (FISHMOD_HAZE_FOG_FAR - sWaterNear);
    gFishmodWaterFog.h = eye.y - FISHMOD_LAKE_SURFACE;
}

#if FISHMOD_DEBUG
s16 gFishmodWaterTune[3] = { FISHMOD_WATER_ALPHA, FISHMOD_WATER_ALPHA_OUT, FISHMOD_WATER_TINT };
#endif

/** A fog colour taken toward vanilla's water's colour: FISHMOD_WATER_TINT of 256, times amount of 256. */
void Fishmod_WaterTint(u8* rgb, s32 amount) {
#if FISHMOD_WATER_SURFACE
    static const u8 sMean[3] = FISHMOD_WATER_MEAN;
    s32 k = (FISHMOD_WATER_TUNE(FISHMOD_WT_TINT, FISHMOD_WATER_TINT) * amount) >> 8;
    s32 c;

    for (c = 0; c < 3; c++) {
        rgb[c] += ((sMean[c] - rgb[c]) * k) >> 8;
    }
#endif
}

/** Under water, the scene's fog colour (the fog, the frame's clear) tinted as the water's seen from above, by how far
 * the haze has come in: after Environment_Update, which makes it afresh each frame. */
void Fishmod_HazeTint(PlayState* play) {
    if ((gFishmodHaze > 0.0f) && (play->sceneId == SCENE_LAKE_HYLIA)) {
        Fishmod_WaterTint(play->lightCtx.fogColor, (s32)(gFishmodHaze * 256.0f));
    }
}

/** The RSP's fog numbers for something this deep under the surface, seen from above (false: next to none: the
 * scene's fog is left as it is). */
s32 Fishmod_WaterFogFactor(f32 depth, s16* fm, s16* fo) {
    f32 sh;
    f32 m;
    f32 o;

    if (!gFishmodWaterFog.on || (depth <= 0.0f)) {
        return false;
    }
    sh = depth / (gFishmodWaterFog.h + depth);
    if (sh < 0.09f) {
        return false; // (barely under, seen from high up: its share of the water too small to show; and fm / s fits)
    }
    m = gFishmodWaterFog.fm / sh;
    o = gFishmodWaterFog.zA * gFishmodWaterFog.fm * (1.0f - 1.0f / sh) + gFishmodWaterFog.fo;
    *fm = (s16)CLAMP(m, -32768.0f, 32767.0f);
    *fo = (s16)CLAMP(o, -32768.0f, 32767.0f);
    return true;
}

/** The scene's fog again (Play_SetFog's), after a pipe sync. */
Gfx* Fishmod_WaterFogOff(PlayState* play, Gfx* gfx) {
    gDPPipeSync(gfx++);
    return Play_SetFog(play, gfx);
}

/** The lake bed's band lists, FISHMOD_WATER_BANDS of them and the one setting the scene's fog back, 4 commands each
 * (empty, an end, with the water fog off). */
void Fishmod_WaterFogBands(PlayState* play, Gfx* out) {
    s32 k;

    Fishmod_WaterFogSetup(play);
    for (k = 0; k < FISHMOD_WATER_BANDS; k++) {
        Gfx* g = &out[k * 4];
        s16 fm;
        s16 fo;

        if (Fishmod_WaterFogFactor(sWaterBandMid[k], &fm, &fo)) {
            gDPPipeSync(g++);
            gDPSetFogColor(g++, gFishmodWaterFog.water[0], gFishmodWaterFog.water[1], gFishmodWaterFog.water[2], 0);
            gSPFogFactor(g++, fm, fo);
        }
        gSPEndDisplayList(g++);
    }
    {
        Gfx* g = &out[FISHMOD_WATER_BANDS * 4];

        if (gFishmodWaterFog.on) {
            g = Fishmod_WaterFogOff(play, g); // (three commands: the sync, the colour, the numbers)
        }
        gSPEndDisplayList(g++);
    }
}

Gfx* Fishmod_Relight(PlayState* play, Lights* lights, Gfx* gfx) {
    void** table;
    const Vtx* src;
    s32 count;
    s32 i;

    if ((spot06_room_0_fishmodRelit == NULL) || (play->sceneId != SCENE_LAKE_HYLIA) ||
        (play->roomCtx.curRoom.num != 0) || (play->roomCtx.curRoom.segment == NULL)) {
        if (sRelitSrc != NULL) {
            Fishmod_RelitFree();
        }
        return gfx;
    }
    table = (void**)((u8*)play->roomCtx.curRoom.segment + ((u32)spot06_room_0_fishmodRelit & 0xFFFFFF));
    count = (s32)table[0];
    src = (const Vtx*)((u8*)play->roomCtx.curRoom.segment + ((u32)table[1] & 0xFFFFFF));
    if (src != sRelitSrc) {
        // the room (re)loaded: the buffers made from its table (positions, texture coordinates, alpha)
        Fishmod_RelitFree();
        for (i = 0; i < FISHMOD_RELIT_BUFS; i++) {
            sRelitBuf[i] = ZELDA_ARENA_MALLOC(count * sizeof(Vtx), "../fishmod.c", __LINE__);
        }
        sRelitNormal = ZELDA_ARENA_MALLOC(count * 3, "../fishmod.c", __LINE__);
        if ((sRelitBuf[FISHMOD_RELIT_BUFS - 1] == NULL) || (sRelitBuf[0] == NULL) || (sRelitBuf[1] == NULL) ||
            (sRelitNormal == NULL)) {
            Fishmod_RelitFree();
            return gfx; // (no memory: the lit pieces would read garbage; the segment is left unset)
        }
        sRelitRoom = play->roomCtx.curRoom.segment;
        sRelitSrcs = (const u32*)((u8*)play->roomCtx.curRoom.segment + ((u32)table[2] & 0xFFFFFF));
        for (i = 0; i < FISHMOD_RELIT_BUFS; i++) {
            bcopy(src, sRelitBuf[i], count * sizeof(Vtx)); // (positions, texture coordinates, alpha)
        }
        sRelitSrc = src;
        sRelitCount = count;
        gFishmodRelitDirty = true; // (the normals gathered below)
    }
    {
        // each vertex's darkening by its depth under the surface (none with the lake drained): made again when that
        // changes, and then the bed relit at once
        s32 drained = !FISHMOD_DEPTH_SHADE || Fishmod_LakeDrained(play) || FISHMOD_BENCH_SKIP(FISHMOD_BENCH_NODEPTHSHADE);

        if (drained != sRelitDrained) {
            if (sRelitDepth == NULL) {
                sRelitDepth = ZELDA_ARENA_MALLOC(sRelitCount, "../fishmod.c", __LINE__);
            }
            if (sRelitDepth != NULL) {
                for (i = 0; i < sRelitCount; i++) {
                    sRelitDepth[i] = drained ? 255 : CLAMP_MAX(Fishmod_DepthShade(FISHMOD_LAKE_SURFACE - sRelitSrc[i].v.ob[1]), 255);
                }
            }
            sRelitDrained = drained;
            gFishmodRelitDirty = true;
        }
    }
    {
        // the lights this frame (their count, the ambient colour, each light's colour and direction: the bytes that
        // light a vertex; the structure's padding isn't cleared each frame, so it isn't compared): when they're what
        // the newest buffer was lit with, it stays
        u8 key[sizeof(sRelitKey)];
        s32 len = 0;
        s32 k;

        key[len++] = lights->numLights;
        key[len++] = lights->l.a.l.col[0];
        key[len++] = lights->l.a.l.col[1];
        key[len++] = lights->l.a.l.col[2];
        for (k = 0; (k < lights->numLights) && (len + 6 <= (s32)sizeof(key)); k++) {
            Light_t* l = &lights->l.l[k].l;

            key[len++] = l->col[0];
            key[len++] = l->col[1];
            key[len++] = l->col[2];
            key[len++] = l->dir[0];
            key[len++] = l->dir[1];
            key[len++] = l->dir[2];
        }
        if (gFishmodRelitDirty) {
            // the normals as the room holds them now (the game may have shortened some to shade the lake bed)
            for (i = 0; i < sRelitCount; i++) {
                const s8* n = (const s8*)((const Vtx*)(sRelitRoom + (sRelitSrcs[i] & 0xFFFFFF)))->n.n;

                sRelitNormal[i * 3 + 0] = n[0];
                sRelitNormal[i * 3 + 1] = n[1];
                sRelitNormal[i * 3 + 2] = n[2];
            }
        }
        s32 big = (sRelitFront < 0) || gFishmodRelitDirty || (len != sRelitKeyLen);
        s32 changed = big || !Fishmod_RelitSame(sRelitKey, key, len);

        // (a small change waits until half a second has passed since the last: the sun moves a fraction of a degree
        // a second, a day being 20 minutes; a jump of more than a few steps, a lightning flash or the lights of a
        // new scene, is lit at once)
        sRelitAge++;
        if (changed && !big) {
            for (k = 0; k < len; k++) {
                s32 d = key[(k)] - sRelitKey[k];

                if ((d > FISHMOD_RELIT_JUMP) || (d < -FISHMOD_RELIT_JUMP)) {
                    big = true;
                    break;
                }
            }
        }
        if (changed && (big || (sRelitAge >= FISHMOD_RELIT_MIN_FRAMES))) {
            s32 next = (sRelitFront + 1) % FISHMOD_RELIT_BUFS;

            Fishmod_RelitCompute(sRelitBuf[next], lights);
            sRelitFront = next;
            bcopy(key, sRelitKey, len);
            sRelitKeyLen = len;
            gFishmodRelitDirty = false;
            gFishmodRelitCount++;
            sRelitAge = 0;
        }
    }
    gSPSegment(gfx++, FISHMOD_RELIT_SEG, sRelitBuf[sRelitFront]);
    return gfx;
}

/*
 * A build that needs the Expansion Pak (fishmod/build.sh --expak), started without one: says so and stops there,
 * before anything is set up that counts on the 8 MB (called from SysCfb_Init, code/sys_cfb.c, on the flag below).
 * Drawn with the crash screen's own text, into a frame buffer at the top of the 4 MB that are there.
 */
#include "fault.h"
const u8 gFishmodExpakRequired = FISHMOD_EXPAK;

void Fishmod_NeedsExpansionPak(void) {
    u16* fb = (u16*)(0x80400000 - (SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16)));

    Fault_SetFrameBuffer(fb, SCREEN_WIDTH, SCREEN_HEIGHT);
    Fault_DrawRecBlack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    Fault_DrawText(56, 96, "THIS VERSION NEEDS THE");
    Fault_DrawText(56, 108, "EXPANSION PAK.");
    Fault_DrawText(56, 132, "TURN THE CONSOLE OFF AND");
    Fault_DrawText(56, 144, "PUT ONE IN.");
    osWritebackDCacheAll();
    osViSwapBuffer(fb);
    osViBlack(false);
    for (;;) {}
}

#include "fishmod_feel.inc.c" // the fishing's feel tuned from the desktop (test builds)
