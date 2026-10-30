/*
 * fishmod's items: one table of what the mod adds (the rod's slot, the lures, the bait bottles) and everything the
 * game asks about an item (its icon, name graphic, slot, what Link does with it, whether it counts as a bottle),
 * and the item page's grid (6 x 5: fishmod.h has the layout). The game's own items go on being looked up the game's
 * way; these functions answer only for the mod's.
 */
#include "fishmod.h"
#include "fishmod_item_page.h"
#include "array_count.h"
#include "dma.h"
#include "inventory.h"
#include "item.h"
#include "play_state.h"
#include "player.h"
#include "segment_symbols.h"
#include "stddef.h"

#include "fishmod_bait_icons.inc.c"

DECLARE_ROM_SEGMENT(fishmod_item_page)

#define NO_PAGE_INDEX 0xFF

typedef struct FishmodItemDef {
    u8 item;
    s8 action;      // what Link does with it from a button (PLAYER_IA_*); using them is fishmod's (Fishmod_UseItem)
    u8 isBottle;    // counts as a bottle (button restrictions, the age swap)
    u8 lureIcon;    // its icon in the item page's graphics (lureIcons[]), or NO_PAGE_INDEX
    u8 name;        // its name graphic in the item page's graphics (names[]), or NO_PAGE_INDEX: the game's (nameAs)
    u8 nameAs;      // ...or the game's item whose name it goes by
    u64* ramIcon;   // its icon in RAM (the bait bottles), or NULL
} FishmodItemDef;

static const FishmodItemDef sItems[] = {
    // item                  action                     bottle  lure icon      name           name as             RAM icon
    { ITEM_FISHING_POLE, PLAYER_IA_FISHING_POLE, false, NO_PAGE_INDEX, 0, ITEM_FISHING_POLE, NULL },
    { ITEM_LURE_CRANKBAIT, PLAYER_IA_NONE, false, 0, 1, ITEM_LURE_CRANKBAIT, NULL },
    { ITEM_LURE_FROG, PLAYER_IA_NONE, false, 1, 2, ITEM_LURE_FROG, NULL },
    { ITEM_LURE_SINKING, PLAYER_IA_NONE, false, 2, 3, ITEM_LURE_SINKING, NULL },
    // bait bottles: Link holds them as bottles (using one is fishmod's); until they have name graphics of their own,
    // they go by the bottle's name
    { ITEM_BOTTLE_LARVA_3, PLAYER_IA_BOTTLE, true, NO_PAGE_INDEX, NO_PAGE_INDEX, ITEM_BOTTLE_EMPTY,
      gFishmodBottleLarva3Tex },
    { ITEM_BOTTLE_LARVA_2, PLAYER_IA_BOTTLE, true, NO_PAGE_INDEX, NO_PAGE_INDEX, ITEM_BOTTLE_EMPTY,
      gFishmodBottleLarva2Tex },
    { ITEM_BOTTLE_LARVA_1, PLAYER_IA_BOTTLE, true, NO_PAGE_INDEX, NO_PAGE_INDEX, ITEM_BOTTLE_EMPTY,
      gFishmodBottleLarva1Tex },
};

static const FishmodItemDef* Fishmod_ItemDef(s32 item) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sItems); i++) {
        if (sItems[i].item == item) {
            return &sItems[i];
        }
    }
    return NULL;
}

/** What Link does with one of the mod's items from a button (Player_ItemToItemAction), PLAYER_IA_NONE if unknown. */
s32 Fishmod_ItemAction(s32 item) {
    const FishmodItemDef* def = Fishmod_ItemDef(item);

    return (def != NULL) ? def->action : PLAYER_IA_NONE;
}

/** True for anything that's a bottle: the game's bottles and the mod's bait bottles. */
s32 Fishmod_IsBottleItem(s32 item) {
    const FishmodItemDef* def;

    if ((item >= ITEM_BOTTLE_EMPTY) && (item <= ITEM_BOTTLE_POE)) {
        return true;
    }
    def = Fishmod_ItemDef(item);
    return (def != NULL) && def->isBottle;
}

/** The icon of one of the mod's items that lives in RAM (the bait bottles), or NULL. */
void* Fishmod_ItemIcon(s32 item) {
    const FishmodItemDef* def = Fishmod_ItemDef(item);

    return (def != NULL) ? def->ramIcon : NULL;
}

/** The game item whose name graphic the mod's item goes by (the item itself if it has its own, or isn't the mod's). */
s32 Fishmod_ItemNameIndex(s32 item) {
    const FishmodItemDef* def = Fishmod_ItemDef(item);

    return (def != NULL) ? def->nameAs : item;
}

/* The item page's own graphics (fishmod/tools/gen_item_page.py, include/fishmod_item_page.h) */

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
    DMA_REQUEST_SYNC(dst, (uintptr_t)_fishmod_item_pageSegmentRomStart, Fishmod_ItemPageSize(), "fishmod_items.c",
                     __LINE__);
}

/** An item's icon while paused (the item page, the icon flying to a C button). */
void* Fishmod_PauseItemIcon(s32 item) {
    const FishmodItemDef* def = Fishmod_ItemDef(item);

    if (def != NULL) {
        if (def->ramIcon != NULL) {
            return def->ramIcon;
        }
        if ((def->lureIcon != NO_PAGE_INDEX) && (gFishmodItemPageData != NULL)) {
            return gFishmodItemPageData->lureIcons[def->lureIcon];
        }
    }
    return gItemIcons[item];
}

/** Where the C buttons load one of the mod's icons from in ROM (the lures), or 0. */
uintptr_t Fishmod_ItemIconVrom(s32 item) {
    const FishmodItemDef* def = Fishmod_ItemDef(item);

    if ((def == NULL) || (def->lureIcon == NO_PAGE_INDEX)) {
        return 0;
    }
    return (uintptr_t)_fishmod_item_pageSegmentRomStart + offsetof(FishmodItemPage, lureIcons) +
           def->lureIcon * sizeof(gFishmodItemPageData->lureIcons[0]);
}

/** Where the pause menu loads one of the mod's name graphics from in ROM, or 0 (the game's own). */
uintptr_t Fishmod_ItemNameVrom(s32 item) {
    const FishmodItemDef* def = Fishmod_ItemDef(item);

    if ((def == NULL) || (def->name == NO_PAGE_INDEX)) {
        return 0;
    }
    return (uintptr_t)_fishmod_item_pageSegmentRomStart + offsetof(FishmodItemPage, names) +
           def->name * sizeof(gFishmodItemPageData->names[0]);
}

/*
 * The item page's grid, six columns by five rows (fishmod.h). The cursor moves over grid places (cursorPoint,
 * cursorX, cursorY); each place holds an inventory slot (cursorSlot), and each slot's quad sits at its place.
 */
u8 gFishmodItemGridSlot[INVENTORY_SLOTS] = {
    SLOT_DEKU_STICK,  SLOT_DEKU_NUT,      SLOT_BOMB,       SLOT_BOW,         SLOT_ARROW_FIRE,  SLOT_DINS_FIRE,
    SLOT_SLINGSHOT,   SLOT_OCARINA,       SLOT_BOMBCHU,    SLOT_HOOKSHOT,    SLOT_ARROW_ICE,   SLOT_FARORES_WIND,
    SLOT_BOOMERANG,   SLOT_LENS_OF_TRUTH, SLOT_MAGIC_BEAN, SLOT_HAMMER,      SLOT_ARROW_LIGHT, SLOT_NAYRUS_LOVE,
    SLOT_FISHING_ROD, SLOT_LURE_1,        SLOT_LURE_2,     SLOT_LURE_3,      SLOT_TRADE_ADULT, SLOT_TRADE_CHILD,
    SLOT_BOTTLE_1,    SLOT_BOTTLE_2,      SLOT_BOTTLE_3,   SLOT_BOTTLE_4,    SLOT_BOTTLE_5,    SLOT_BOTTLE_6,
};
// and back: each slot's place
u8 gFishmodItemSlotGrid[INVENTORY_SLOTS] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    24, 25, 26, 27, // bottles 1-4
    22, 23,         // trade items
    18, 19, 20, 21, // rod, lures
    28, 29,         // bottles 5-6
};

/** Where an inventory slot's item is kept: the vanilla 24 in the inventory, the item page's six new ones in the mod's
 * save block (INV_CONTENT goes through here; INV_SLOT is the same as a macro). */
u8* Fishmod_InvSlot(s32 slot) {
    return &INV_SLOT(slot);
}
