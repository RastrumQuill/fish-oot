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

s32 Fishmod_CanUseRod(PlayState* play) {
    return (gFishmodRod != NULL) && (Fishmod_GetSceneSpawns(play->sceneId) != NULL);
}
