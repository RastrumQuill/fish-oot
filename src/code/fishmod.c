/*
 * File: fishmod.c
 * Description: fishmod core. Species data, which scenes have fishing, and the hooks that load the fishing
 * object and spawn the fishing actor in those scenes. Always part of the code segment.
 */

#include "fishmod.h"
#include "fishmod_species_data.h"

#include "array_count.h"
#include "actor.h"
#include "object.h"
#include "play_state.h"
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
static s32 sFpsDetail = 1; // D-pad Left: the RDP readout on or off
#endif

s32 Object_LoadPersistent(ObjectContext* objectCtx, s16 objectId);

Actor* gFishmodRod = NULL;
static u8 sRainSoundPending; // start the rain's sound once the scene's music has started (see the weather)
Vec3f gFishmodHivePos; // where the scene's added beehive went (debug display)

// The rod rides on B while fishing (like in the Fishing Pond); L puts it there and back.
static u8 sRodOnB = false;
static u8 sSavedBItem = ITEM_NONE;

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

static void Fishmod_RestoreB(PlayState* play) {
    if (gSaveContext.save.info.equips.buttonItems[0] == ITEM_FISHING_POLE) {
        u8 item = sSavedBItem;

        if ((item == ITEM_NONE) || (item == ITEM_FISHING_POLE)) {
            // lost track (e.g. after a reset): give back the equipped sword
            item = Fishmod_SwordBItem();
        }
        gSaveContext.save.info.equips.buttonItems[0] = item;
        if (play != NULL && item != ITEM_NONE) {
            Interface_LoadItemIcon1(play, 0);
        }
    }
    sRodOnB = false;
}
FishmodPhoto gFishmodPhoto;
FishmodStudio gFishmodStudio;
FishmodRodInfo gFishmodRodInfo;

const FishmodSceneSpawns* Fishmod_GetSceneSpawns(s16 sceneId) {
    s32 i;

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

void Fishmod_OnSceneLoad(PlayState* play) {
    gFishmodRod = NULL;

    // never carry the rod-on-B swap into another scene
    if (play->sceneId != SCENE_FISHING_POND) {
        Fishmod_RestoreB(NULL);
    }

    if ((Fishmod_GetSceneSpawns(play->sceneId) == NULL)
#if FISHMOD_DEBUG
        && !Fishmod_DebugWantsFish()
#endif
    ) {
        return;
    }

    if (Object_GetSlot(&play->objectCtx, OBJECT_FISH) < 0) {
        Object_LoadPersistent(&play->objectCtx, OBJECT_FISH);
    }
}

/**
 * Things fishmod adds to a scene besides the fish. Kokiri Forest gets a beehive (for bee larva bait) on a wall in
 * sight of the moat, found by looking for the nearest wall around a spot by the water.
 */
static void Fishmod_SpawnSceneExtras(PlayState* play) {
    if (play->sceneId == SCENE_KOKIRI_FOREST) {
        static Vec3f sHiveLookFrom = { 960.0f, 75.0f, -60.0f };
        Vec3f best;
        f32 bestDist = 400.0f;
        s16 bestYaw = 0;
        s32 i;

        for (i = 0; i < 16; i++) {
            s16 yaw = i * 0x1000;
            Vec3f to;
            Vec3f hit;
            CollisionPoly* poly;
            s32 bgId;

            to.x = sHiveLookFrom.x + Math_SinS(yaw) * 400.0f;
            to.y = sHiveLookFrom.y;
            to.z = sHiveLookFrom.z + Math_CosS(yaw) * 400.0f;
            if (BgCheck_EntityLineTest1(&play->colCtx, &sHiveLookFrom, &to, &hit, &poly, true, false, false, true,
                                        &bgId)) {
                f32 dist = Math_Vec3f_DistXZ(&sHiveLookFrom, &hit);

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

    // the rain's sound (the nature ambience's rain channel, as vanilla's rain weather plays it), once the scene's
    // own music has started
    if (sRainSoundPending && (play->transitionMode == TRANS_MODE_OFF) && (play->state.frames > 30)) {
        sRainSoundPending = false;
        Environment_PlayStormNatureAmbience(play);
    }

    // Repair files saved with the old rod bug that left a Deku Stick on B (vanilla never puts it there)
    if (gSaveContext.save.info.equips.buttonItems[0] == ITEM_DEKU_STICK) {
        gSaveContext.save.info.equips.buttonItems[0] = Fishmod_SwordBItem();
        if (gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) {
            Interface_LoadItemIcon1(play, 0);
        }
    }

#if FISHMOD_TESTMODE
    // test mode: D-pad Up warps to Kokiri Forest (small water) as child Link, D-pad Down to Lake Hylia (big water)
    // as adult Link with the lake full, in the Iron Boots
    if ((play->pauseCtx.state == 0) && (Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) &&
        !Play_InCsMode(play) && (play->transitionTrigger == TRANS_TRIGGER_OFF)) {
        u16 press = play->state.input[0].press.button;
        s16 entrance = -1;

        // D-pad Left: the RDP readout
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

    // L: rod on B / back to the sword
    if ((Fishmod_GetSceneSpawns(play->sceneId) != NULL) && (gFishmodRod != NULL) &&
        CHECK_BTN_ALL(play->state.input[0].press.button, BTN_L) && (play->pauseCtx.state == 0) &&
        (Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) && !Play_InCsMode(play)) {
        if (!sRodOnB) {
            if (gSaveContext.save.info.equips.buttonItems[0] != ITEM_FISHING_POLE) {
                sSavedBItem = gSaveContext.save.info.equips.buttonItems[0];
                gSaveContext.save.info.equips.buttonItems[0] = ITEM_FISHING_POLE;
                Interface_LoadItemIcon1(play, 0);
            }
            sRodOnB = true;
            Sfx_PlaySfxCentered(NA_SE_SY_CAMERA_ZOOM_UP);
        } else {
            Fishmod_RestoreB(play);
            Sfx_PlaySfxCentered(NA_SE_SY_CAMERA_ZOOM_DOWN);
        }
    }

    if ((gFishmodRod != NULL) || (Fishmod_GetSceneSpawns(play->sceneId) == NULL) || (GET_PLAYER(play) == NULL)) {
        return;
    }

    rod = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, 0.0f, 0.0f, 0.0f, 0, 0, 0,
                      FISHMOD_PARAMS(FISHMOD_TYPE_ROD, 0));
    if (rod != NULL) {
        rod->room = -1; // persists through room changes
        gFishmodRod = rod;
        Fishmod_SpawnSceneExtras(play);
    }
}

f32 gFishmodSpawnLength;
u8 gFishmodSpawnLook;
// a fishmod fish poured out on land (the game's flopping fish stands in for it until it's bottled again)
static FishmodBottleFish sFloppingFish;

static const char* sBaitNames[FISHMOD_BAIT_MAX] = {
    "None", "Bee larva", "Bug", "Worm", "Coral earring", "Fish", "Spinner", "Crankbait", "Popper", "Frog lure",
    "Sinking lure",
};

const char* Fishmod_BaitName(s32 bait) {
    return ((bait >= 0) && (bait < FISHMOD_BAIT_MAX)) ? sBaitNames[bait] : "?";
}

#include "fishmod_bait_icons.inc.c"

void* Fishmod_ItemIcon(s32 item) {
    switch (item) {
        case ITEM_BOTTLE_LARVA_3:
            return gFishmodBottleLarva3Tex;
        case ITEM_BOTTLE_LARVA_2:
            return gFishmodBottleLarva2Tex;
        case ITEM_BOTTLE_LARVA_1:
            return gFishmodBottleLarva1Tex;
        default:
            return NULL;
    }
}

s32 Fishmod_ItemNameIndex(s32 item) {
    // until they have name graphics of their own, bait bottles go by the bottle's name
    return FISHMOD_IS_BAIT_ITEM(item) ? ITEM_BOTTLE_EMPTY : item;
}

/**
 * A bottle of bait (or bugs) used from a button while Link holds the rod, before casting: one goes on the hook
 * and the bottle has one less. Anything else about these bottles does nothing for now (there's no pouring
 * larvae out). Returns true if it handled the item.
 */
s32 Fishmod_UseItem(PlayState* play, s32 item) {
    static u8 sNextLarva[] = { ITEM_BOTTLE_LARVA_2, ITEM_BOTTLE_LARVA_1, ITEM_BOTTLE_EMPTY };
    s32 bait;
    s32 next;

    if (FISHMOD_IS_LURE_ITEM(item)) {
        // placeholder: lures can be put on a C button, but don't go on the line yet
        Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
        return true;
    }
    if (FISHMOD_IS_BAIT_ITEM(item)) {
        bait = FISHMOD_BAIT_BEE_LARVA;
        next = sNextLarva[item - ITEM_BOTTLE_LARVA_3];
    } else if ((item == ITEM_BOTTLE_BUG) && (gFishmodRodInfo.state != 0)) {
        bait = FISHMOD_BAIT_BUG;
        next = ITEM_BOTTLE_EMPTY;
    } else {
        return false;
    }
    // only onto a bare hook, with the rod held and not cast
    if ((gFishmodRodInfo.state != 1) || (gFishmodRodInfo.bait != FISHMOD_BAIT_NONE) ||
        (gFishmodRodInfo.baitRequest != FISHMOD_BAIT_NONE)) {
        Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
        return true;
    }
    gFishmodRodInfo.baitRequest = bait;
    Inventory_ReplaceItem(play, item, next);
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
        species = spawns->entries[(s32)Rand_ZeroFloat(spawns->count - 0.01f)].species;
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
        if (p + 2 < end) {
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
    // (with the rod on B, the sword waits where the rod keeps it)
    if (sRodOnB) {
        sSavedBItem = sword;
    } else {
        gSaveContext.save.info.equips.buttonItems[0] = sword;
    }
}

void Fishmod_InitTestSave(void) {
    Inventory* inv = &gSaveContext.save.info.inventory;
    s32 i;

    gSaveContext.save.cutsceneIndex = CS_INDEX_NONE; // no Deku Tree / Navi intro

    // Saria and Mido already met, as on the debug save
    gSaveContext.save.info.infTable[INFTABLE_INDEX_0] |= INFTABLE_MASK(INFTABLE_SARIA_GREETED_LINK) |
                                                         INFTABLE_MASK(INFTABLE_SARIA_WAS_TOLD_ABOUT_MIDO) |
                                                         INFTABLE_MASK(INFTABLE_0C) | INFTABLE_MASK(INFTABLE_0E);

    // five empty bottles and one with bugs, for bait and catches
    Fishmod_InitSaveExtras();
    for (i = 0; i < FISHMOD_BOTTLES; i++) {
        INV_SLOT(FISHMOD_BOTTLE_SLOT(i)) = ITEM_BOTTLE_EMPTY;
    }
    inv->items[SLOT_BOTTLE_4] = ITEM_BOTTLE_BUG;
    // the rod, and the lures (placeholders)
    INV_SLOT(SLOT_FISHING_ROD) = ITEM_FISHING_POLE;
    INV_SLOT(SLOT_LURE_1) = ITEM_LURE_CRANKBAIT;
    INV_SLOT(SLOT_LURE_2) = ITEM_LURE_FROG;
    INV_SLOT(SLOT_LURE_3) = ITEM_LURE_SINKING;

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
    SET_EVENTCHKINF(EVENTCHKINF_B1); // Lake Hylia's intro cutscene seen (it took the camera on the first warp there)
    gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_KOKIRI;
    Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
    Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_DEKU);
    gSaveContext.save.info.equips.buttonItems[1] = ITEM_SLINGSHOT;
    gSaveContext.save.info.equips.cButtonSlots[0] = SLOT_SLINGSHOT;
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
    IO_WRITE(DPC_STATUS_REG, DPC_CLR_CLOCK_CTR | DPC_CLR_CMD_CTR | DPC_CLR_PIPE_CTR | DPC_CLR_TMEM_CTR);
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

s32 Fishmod_CanUseRod(PlayState* play) {
    return (gFishmodRod != NULL) && (Fishmod_GetSceneSpawns(play->sceneId) != NULL);
}

/* Baked water floor maps (fishmod/tools/gen_watermap.py) */

#include "dma.h"
#include "segment_symbols.h"
#include "zelda_arena.h"
#include "tha.h"
#include "fishmod_collision.h"

/* The item page's own graphics (fishmod/tools/gen_item_page.py, include/fishmod_item_page.h) */

#include "fishmod_item_page.h"
#include "stddef.h"

DECLARE_ROM_SEGMENT(fishmod_item_page)

FishmodItemPage* gFishmodItemPageData;

u32 Fishmod_ItemPageSize(void) {
    return offsetof(FishmodItemPage, names); // (the names stay in ROM)
}

/**
 * Loads the page's graphics at `dst`, after the pause menu's own in the scene's object space. That space is nearly
 * full already (adult Link outside a dungeon leaves about 41 KB past the Link preview's coverage; these take 35 KB),
 * so if they wouldn't fit, the page stays vanilla.
 */
void Fishmod_LoadItemPage(PlayState* play, void* dst) {
    if ((uintptr_t)dst + Fishmod_ItemPageSize() > (uintptr_t)play->objectCtx.spaceEnd) {
        gFishmodItemPageData = NULL;
        return;
    }
    gFishmodItemPageData = dst;
    DMA_REQUEST_SYNC(dst, (uintptr_t)_fishmod_item_pageSegmentRomStart, Fishmod_ItemPageSize(), "fishmod.c", __LINE__);
}

/** An item's icon while paused: the lures' come from the item page's graphics (when they're loaded). */
void* Fishmod_PauseItemIcon(s32 item) {
    if (FISHMOD_IS_LURE_ITEM(item) && (gFishmodItemPageData != NULL)) {
        return gFishmodItemPageData->lureIcons[item - ITEM_LURE_CRANKBAIT];
    }
    return gItemIcons[item];
}

uintptr_t Fishmod_ItemIconVrom(s32 item) {
    if (FISHMOD_IS_LURE_ITEM(item)) {
        return (uintptr_t)_fishmod_item_pageSegmentRomStart + offsetof(FishmodItemPage, lureIcons) +
               (item - ITEM_LURE_CRANKBAIT) * sizeof(gFishmodItemPageData->lureIcons[0]);
    }
    return 0;
}

uintptr_t Fishmod_ItemNameVrom(s32 item) {
    s32 index;

    if (item == ITEM_FISHING_POLE) {
        index = 0;
    } else if (FISHMOD_IS_LURE_ITEM(item)) {
        index = 1 + item - ITEM_LURE_CRANKBAIT;
    } else {
        return 0;
    }
    return (uintptr_t)_fishmod_item_pageSegmentRomStart + offsetof(FishmodItemPage, names) +
           index * sizeof(gFishmodItemPageData->names[0]);
}

/** The save block's new parts: the item page's six new slots start empty (a new file, or one from before). */
void Fishmod_InitSaveExtras(void) {
    s32 i;

    if (!(FISHMOD_SAVE->flags & FISHMOD_SAVE_FLAG_ITEMS)) {
        for (i = 0; i < ARRAY_COUNT(FISHMOD_SAVE->items); i++) {
            FISHMOD_SAVE->items[i] = ITEM_NONE;
        }
        FISHMOD_SAVE->flags |= FISHMOD_SAVE_FLAG_ITEMS;
    }
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

#define FISHMOD_COLLISION(sceneId, segment) DECLARE_ROM_SEGMENT(segment)
#include "fishmod_collision_scenes.h"
#undef FISHMOD_COLLISION

static const FishmodWaterMapRom sPropCollisions[] = {
#define FISHMOD_COLLISION(sceneId, segment) \
    { sceneId, (uintptr_t)_##segment##SegmentRomStart, (uintptr_t)_##segment##SegmentRomEnd },
#include "fishmod_collision_scenes.h"
#undef FISHMOD_COLLISION
};

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
    if (!under && (gFishmodHaze <= 0.0f)) {
        return;
    }
    Math_StepToF(&gFishmodHaze, under ? 1.0f : 0.0f, 0.1f);
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
 * Weather: a roll per fishing spot per day (the same all day): mostly clear, sometimes overcast, rarely rain. Set
 * up the way the engine's own rainy weather is (darker sky and light, rain falling). The Song of Storms still
 * brings rain on top of it.
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

void Fishmod_OnEnvironmentInit(PlayState* play) {
    EnvironmentContext* envCtx = &play->envCtx;

    sWeather = FISHMOD_WEATHER_CLEAR;
    sRainSoundPending = false;
    if ((Fishmod_GetSceneSpawns(play->sceneId) == NULL) || (play->skyboxId != SKYBOX_NORMAL_SKY) ||
        (gWeatherMode != WEATHER_MODE_CLEAR)) {
        return;
    }
    sWeather = Fishmod_WeatherRoll(play->sceneId, gSaveContext.save.totalDays);
    if (sWeather == FISHMOD_WEATHER_CLEAR) {
        return;
    }
    // the cloudy sky and the darker light set (scenes have at least three light sets, so set 2 is safe)
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

#ifdef COMPILER_GCC
/*
 * GCC_CODE=1 (the game's code compiled with GCC, the rest with IDO): GCC calls libgcc's helpers for 64-bit division
 * and conversions, which IDO's libultra provides under its own names. o32 passes 64-bit values the same way in both.
 */
unsigned long long __ull_div(unsigned long long left, unsigned long long right);
unsigned long long __ull_rem(unsigned long long left, unsigned long long right);
long long __ll_div(long long left, long long right);
long long __ll_mul(long long left, long long right);
long long __d_to_ll(double d);
long long __f_to_ll(float f);
unsigned long long __d_to_ull(double d);
unsigned long long __f_to_ull(float f);
double __ll_to_d(long long l);
float __ll_to_f(long long l);
double __ull_to_d(unsigned long long l);
float __ull_to_f(unsigned long long l);

unsigned long long __udivdi3(unsigned long long a, unsigned long long b) {
    return __ull_div(a, b);
}
unsigned long long __umoddi3(unsigned long long a, unsigned long long b) {
    return __ull_rem(a, b);
}
long long __divdi3(long long a, long long b) {
    return __ll_div(a, b);
}
long long __moddi3(long long a, long long b) {
    return a - __ll_mul(__ll_div(a, b), b); // (C's %: the sign follows the dividend; __ll_rem divides unsigned)
}
long long __muldi3(long long a, long long b) {
    return __ll_mul(a, b);
}
// 64-bit shifts, on the two 32-bit halves (libultra has no equivalent)
long long __ashldi3(long long a, int b) {
    unsigned int lo = (unsigned int)a;
    unsigned int hi = (unsigned int)((unsigned long long)a >> 32);

    if (b >= 32) {
        hi = lo << (b - 32);
        lo = 0;
    } else if (b > 0) {
        hi = (hi << b) | (lo >> (32 - b));
        lo <<= b;
    }
    return (long long)(((unsigned long long)hi << 32) | lo);
}
unsigned long long __lshrdi3(unsigned long long a, int b) {
    unsigned int lo = (unsigned int)a;
    unsigned int hi = (unsigned int)(a >> 32);

    if (b >= 32) {
        lo = hi >> (b - 32);
        hi = 0;
    } else if (b > 0) {
        lo = (lo >> b) | (hi << (32 - b));
        hi >>= b;
    }
    return ((unsigned long long)hi << 32) | lo;
}
// GCC -Os turns some string formatting into a call to strcpy, which the game's libc doesn't have
char* strcpy(char* dst, const char* src) {
    char* d = dst;

    while ((*d++ = *src++) != '\0') {}
    return dst;
}
long long __fixdfdi(double d) {
    return __d_to_ll(d);
}
long long __fixsfdi(float f) {
    return __f_to_ll(f);
}
unsigned long long __fixunsdfdi(double d) {
    return __d_to_ull(d);
}
unsigned long long __fixunssfdi(float f) {
    return __f_to_ull(f);
}
double __floatdidf(long long l) {
    return __ll_to_d(l);
}
float __floatdisf(long long l) {
    return __ll_to_f(l);
}
double __floatundidf(unsigned long long l) {
    return __ull_to_d(l);
}
float __floatundisf(unsigned long long l) {
    return __ull_to_f(l);
}
#endif
