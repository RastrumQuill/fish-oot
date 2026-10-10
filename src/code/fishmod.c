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

s32 Object_LoadPersistent(ObjectContext* objectCtx, s16 objectId);

Actor* gFishmodRod = NULL;

// The rod rides on B while fishing (like in the Fishing Pond); L puts it there and back.
static u8 sRodOnB = false;
static u8 sSavedBItem = ITEM_NONE;

static void Fishmod_RestoreB(PlayState* play) {
    if (gSaveContext.save.info.equips.buttonItems[0] == ITEM_FISHING_POLE) {
        u8 item = sSavedBItem;

        if ((item == ITEM_NONE) || (item == ITEM_FISHING_POLE)) {
            // lost track (e.g. after a reset): give back the equipped sword
            switch (CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD)) {
                case EQUIP_VALUE_SWORD_KOKIRI:
                    item = ITEM_SWORD_KOKIRI;
                    break;
                case EQUIP_VALUE_SWORD_MASTER:
                    item = ITEM_SWORD_MASTER;
                    break;
                case EQUIP_VALUE_SWORD_BIGGORON:
                    item = ITEM_SWORD_BIGGORON;
                    break;
                default:
                    item = ITEM_NONE;
                    break;
            }
        }
        gSaveContext.save.info.equips.buttonItems[0] = item;
        if (play != NULL && item != ITEM_NONE) {
            Interface_LoadItemIcon1(play, 0);
        }
    }
    sRodOnB = false;
}
FishmodPhoto gFishmodPhoto;
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
void Fishmod_OnSceneLoad(PlayState* play) {
    gFishmodRod = NULL;

    // never carry the rod-on-B swap into another scene
    if (play->sceneId != SCENE_FISHING_POND) {
        Fishmod_RestoreB(NULL);
    }

    if (Fishmod_GetSceneSpawns(play->sceneId) == NULL) {
        return;
    }

    if (Object_GetSlot(&play->objectCtx, OBJECT_FISH) < 0) {
        Object_LoadPersistent(&play->objectCtx, OBJECT_FISH);
    }
}

/**
 * Called every Play_Update. Spawns the fishing actor once the scene is running.
 */
void Fishmod_PlayUpdate(PlayState* play) {
    Actor* rod;

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
    }
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
    if (p < end) {
        *p++ = MESSAGE_NEWLINE;
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
        p = Msg_PutString(p, end, "No, let it go");
        p = Journal_PutColor(p, end, TEXT_COLOR_DEFAULT);
    } else {
        p = Msg_PutString(p, end, "No empty bottle, so you let it go.");
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

s32 Fishmod_CanUseRod(PlayState* play) {
    return (gFishmodRod != NULL) && (Fishmod_GetSceneSpawns(play->sceneId) != NULL);
}
