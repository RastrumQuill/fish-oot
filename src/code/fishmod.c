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
#include "message.h"
#include "message_data_fmt.h"
#include "save.h"
#include "sfx.h"
#include "z_lib.h"
#include "string.h"
#include "regs.h"
#include "transition.h"
#include "libc64/sprintf.h"
#include "rand.h"
#include "bgcheck.h"
#include "ultra64/convert.h"
#include "speed_meter.h"
#include "libu64/gfxprint.h"

s32 Object_LoadPersistent(ObjectContext* objectCtx, s16 objectId);

Actor* gFishmodRod = NULL;
Vec3f gFishmodHivePos; // where the scene's added beehive went (debug display)

// The rod rides on B while fishing (like in the Fishing Pond); L puts it there and back.
static u8 sRodOnB = false;
static u8 sSavedBItem = ITEM_NONE;

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
 * Fish kept from each scene's roster today (FISHMOD_OPT 4): they don't come back until the day changes. A few scenes
 * are remembered; the oldest is forgotten first.
 */
typedef struct FishmodRosterCaught {
    s16 sceneId;
    s32 day;
    u32 bits;
} FishmodRosterCaught;

static FishmodRosterCaught sRosterCaught[8];
static s32 sRosterCaughtNext;

static FishmodRosterCaught* Fishmod_FindRosterCaught(s16 sceneId, s32 day, s32 create) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sRosterCaught); i++) {
        if ((sRosterCaught[i].bits != 0) && (sRosterCaught[i].sceneId == sceneId) && (sRosterCaught[i].day == day)) {
            return &sRosterCaught[i];
        }
    }
    if (!create) {
        return NULL;
    }
    i = sRosterCaughtNext;
    sRosterCaughtNext = (sRosterCaughtNext + 1) % ARRAY_COUNT(sRosterCaught);
    sRosterCaught[i].sceneId = sceneId;
    sRosterCaught[i].day = day;
    sRosterCaught[i].bits = 0;
    return &sRosterCaught[i];
}

u32 Fishmod_RosterCaughtBits(s16 sceneId, s32 day) {
    FishmodRosterCaught* entry = Fishmod_FindRosterCaught(sceneId, day, false);

    return (entry != NULL) ? entry->bits : 0;
}

void Fishmod_RosterSetCaught(s16 sceneId, s32 day, s32 index) {
    Fishmod_FindRosterCaught(sceneId, day, true)->bits |= 1 << index;
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

    // Repair files saved with the old rod bug that left a Deku Stick on B (vanilla never puts it there)
    if (gSaveContext.save.info.equips.buttonItems[0] == ITEM_DEKU_STICK) {
        gSaveContext.save.info.equips.buttonItems[0] = Fishmod_SwordBItem();
        if (gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) {
            Interface_LoadItemIcon1(play, 0);
        }
    }

#if FISHMOD_TESTMODE
    // test mode: D-pad Up warps to Kokiri Forest (small water), D-pad Down to Lake Hylia (big water)
    if ((play->pauseCtx.state == 0) && (Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) &&
        !Play_InCsMode(play) && (play->transitionTrigger == TRANS_TRIGGER_OFF)) {
        u16 press = play->state.input[0].press.button;
        s16 entrance = -1;

        if (CHECK_BTN_ALL(press, BTN_DUP) && (play->sceneId != SCENE_KOKIRI_FOREST)) {
            entrance = ENTR_KOKIRI_FOREST_0;
        } else if (CHECK_BTN_ALL(press, BTN_DDOWN) && (play->sceneId != SCENE_LAKE_HYLIA)) {
            entrance = ENTR_LAKE_HYLIA_0;
        }
        if (entrance >= 0) {
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
    } else if ((actorId == ACTOR_EN_FISH) && (slot >= SLOT_BOTTLE_1) && (slot <= SLOT_BOTTLE_4)) {
        // the flopping fish poured out on land: if it was one of fishmod's, it's that fish again
        FISHMOD_SAVE->bottleFish[slot - SLOT_BOTTLE_1] = sFloppingFish;
        sFloppingFish.species = 0;
        sFloppingFish.lengthTenths = 0;
    }
}

/** Notes what just went into a bottle: the first empty one, which is where Item_Give puts a fish. */
void Fishmod_BottleFish(s32 species, f32 lengthInches) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (gSaveContext.save.info.inventory.items[SLOT_BOTTLE_1 + i] == ITEM_BOTTLE_EMPTY) {
            FISHMOD_SAVE->bottleFish[i].species = species + 1;
            FISHMOD_SAVE->bottleFish[i].lengthTenths = (u16)(lengthInches * 10.0f);
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

    if ((spawns == NULL) || (gFishmodRod == NULL) || (slot < SLOT_BOTTLE_1) || (slot > SLOT_BOTTLE_4)) {
        return false;
    }
    bottle = &FISHMOD_SAVE->bottleFish[slot - SLOT_BOTTLE_1];
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
    } else {
        species = spawns->entries[(s32)Rand_ZeroFloat(spawns->count - 0.01f)].species;
        gFishmodSpawnLength = 0.0f;
    }
    bottle->species = 0;
    bottle->lengthTenths = 0;
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

void Fishmod_InitTestSave(void) {
    Inventory* inv = &gSaveContext.save.info.inventory;
    s32 i;

    gSaveContext.save.cutsceneIndex = CS_INDEX_NONE; // no Deku Tree / Navi intro

    // Saria and Mido already met, as on the debug save
    gSaveContext.save.info.infTable[INFTABLE_INDEX_0] |= INFTABLE_MASK(INFTABLE_SARIA_GREETED_LINK) |
                                                         INFTABLE_MASK(INFTABLE_SARIA_WAS_TOLD_ABOUT_MIDO) |
                                                         INFTABLE_MASK(INFTABLE_0C) | INFTABLE_MASK(INFTABLE_0E);

    // three empty bottles and one with bugs, for bait and catches
    for (i = SLOT_BOTTLE_1; i <= SLOT_BOTTLE_4; i++) {
        inv->items[i] = ITEM_BOTTLE_EMPTY;
    }
    inv->items[SLOT_BOTTLE_4] = ITEM_BOTTLE_BUG;

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
    gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_KOKIRI;
    Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
    Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_DEKU);
    gSaveContext.save.info.equips.buttonItems[1] = ITEM_SLINGSHOT;
    gSaveContext.save.info.equips.cButtonSlots[0] = SLOT_SLINGSHOT;
}
#endif

#if FISHMOD_TESTMODE
void Fishmod_DrawFps(Gfx** gfxP) {
    // average of the last 20 frame periods (the time between graph updates), updated every 20 frames
    static u32 sSumUsec;
    static s32 sCount;
    static s32 sFpsTenths;
    GfxPrint printer;

    // nothing extra in the picture the pause menu takes of the screen (it comes out as garbage)
    if (R_PAUSE_BG_PRERENDER_STATE != PAUSE_BG_PRERENDER_OFF) {
        return;
    }
    sSumUsec += (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    if (++sCount >= 20) {
        sFpsTenths = (sSumUsec != 0) ? (s32)(20u * 10000000u / sSumUsec) : 0;
        sSumUsec = 0;
        sCount = 0;
    }
    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, *gfxP);
    GfxPrint_SetColor(&printer, 255, 255, 255, 200);
    GfxPrint_SetPos(&printer, 17, 27); // bottom middle: clear of the hearts, buttons, map and rupees
    GfxPrint_Printf(&printer, "%2d.%dfps", sFpsTenths / 10, sFpsTenths % 10);
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
