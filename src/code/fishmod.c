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

s32 Fishmod_CanUseRod(PlayState* play) {
    return (gFishmodRod != NULL) && (Fishmod_GetSceneSpawns(play->sceneId) != NULL);
}
