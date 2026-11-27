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
#include "save.h"
#include "player.h"
#include "segment_symbols.h"
#include "stddef.h"

#include "src/fishmod_data/fishmod_bait_icons.inc.c" // (generated at build: gen_bait_icons.py)

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
    { ITEM_LURE_POND, PLAYER_IA_NONE, false, 0, 1, ITEM_LURE_POND, NULL },
    { ITEM_LURE_SINKING, PLAYER_IA_NONE, false, 1, 2, ITEM_LURE_SINKING, NULL },
    { ITEM_SINKER, PLAYER_IA_NONE, false, 2, 3, ITEM_SINKER, NULL },
    // bait bottles: Link holds them as bottles (using one is fishmod's); their names in the item page's graphics
    { ITEM_BOTTLE_LARVA_3, PLAYER_IA_BOTTLE, true, NO_PAGE_INDEX, 5, ITEM_BOTTLE_EMPTY, gFishmodBottleLarva3Tex },
    { ITEM_BOTTLE_LARVA_2, PLAYER_IA_BOTTLE, true, NO_PAGE_INDEX, 5, ITEM_BOTTLE_EMPTY, gFishmodBottleLarva2Tex },
    { ITEM_BOTTLE_LARVA_1, PLAYER_IA_BOTTLE, true, NO_PAGE_INDEX, 5, ITEM_BOTTLE_EMPTY, gFishmodBottleLarva1Tex },
    { ITEM_BOTTLE_WORM, PLAYER_IA_BOTTLE, true, NO_PAGE_INDEX, 6, ITEM_BOTTLE_EMPTY, gFishmodBottleWormTex },
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

// What's loaded of them while paused (NULL: not loaded, it didn't fit): the tackle's icons, then the page's tiles
#define LURE_ICONS_LOADED 3 // the pond lure, the sinking lure, and the sinker item's as it shows now (Fishmod_TackleShown)
#define TACKLE_BOBBER_ICON_ROM 3 // (lureIcons[3]: the bobber, shown for the sinker item while the sinker's on)
#define LURE_ICON_SIZE sizeof(((FishmodItemPage*)0)->lureIcons[0])
static u64 (*sLureIconsRam)[32 * 32 * 4 / 8];
u64 (*gFishmodItemPageTiles)[80 * 32 / 8];
s32 gFishmodItemPageMargin; // debug: the bytes left over after them in the object space at the last pause (< 0: short)

u32 Fishmod_ItemPageSize(void) {
    return LURE_ICONS_LOADED * LURE_ICON_SIZE + sizeof(((FishmodItemPage*)0)->pageTex);
}

static void Fishmod_LoadLureIcon(void* dst, s32 index) {
    DMA_REQUEST_SYNC(dst,
                     (uintptr_t)_fishmod_item_pageSegmentRomStart + offsetof(FishmodItemPage, lureIcons) +
                         index * LURE_ICON_SIZE,
                     LURE_ICON_SIZE, "fishmod_items.c", __LINE__);
}

/**
 * Loads the page's graphics at `dst`, after the pause menu's own. The pause menu fills the scene's object space from
 * its start (the objects are loaded again when it closes), and nearly fills it. How much is left depends only on the
 * space's size (1000 KB at least), Link's age and whether the scene is a dungeon, not on what the scene had loaded.
 * The tightest case is adult Link outside a dungeon in a scene with the 1000 KB space: measured at Lake Hylia, 41,712
 * bytes past the Link preview's coverage, of which these take 35,328 (6,384 left: gFishmodItemPageMargin, debug
 * builds). A child has 43 KB more (his object is smaller), a dungeon 40 KB more (its map graphics are smaller).
 * In two parts, so that running short never
 * shows a wrong texture: the tackle's icons first (12 KB; without them the tackle shows the rod's icon), then the
 * page's redrawn tiles (23 KB; without them the game's own page is drawn under the same grid). Only three of the four
 * tackle icons: the sinker item shows the sinker or the bobber by the rig on the line, which doesn't change while
 * paused.
 */
void Fishmod_LoadItemPage(PlayState* play, void* dst) {
    uintptr_t end = (uintptr_t)play->objectCtx.spaceEnd;
    uintptr_t at = (uintptr_t)dst;
    s32 i;

    gFishmodItemPageMargin = (s32)(end - (at + Fishmod_ItemPageSize()));
    sLureIconsRam = NULL;
    gFishmodItemPageTiles = NULL;
    if (at + LURE_ICONS_LOADED * LURE_ICON_SIZE <= end) {
        sLureIconsRam = (void*)at;
        for (i = 0; i < LURE_ICONS_LOADED; i++) {
            // (the last: the sinker item's, as it shows with this rig on)
            Fishmod_LoadLureIcon(sLureIconsRam[i],
                                 ((i == LURE_ICONS_LOADED - 1) && (FISHMOD_RIG() == FISHMOD_RIG_SINKER)) ? TACKLE_BOBBER_ICON_ROM : i);
        }
        at += LURE_ICONS_LOADED * LURE_ICON_SIZE;
        if (at + sizeof(((FishmodItemPage*)0)->pageTex) <= end) {
            gFishmodItemPageTiles = (void*)at;
            DMA_REQUEST_SYNC((void*)at, (uintptr_t)_fishmod_item_pageSegmentRomStart + offsetof(FishmodItemPage, pageTex),
                             sizeof(((FishmodItemPage*)0)->pageTex), "fishmod_items.c", __LINE__);
        }
    }
#if FISHMOD_DEBUG
    if (gFishmodItemPageMargin < 0) {
        osSyncPrintf("fishmod: the item page's graphics don't fit (%d bytes short): %s\n", -gFishmodItemPageMargin,
                     (sLureIconsRam == NULL) ? "nothing loaded" : "the tackle's icons only");
    }
#endif
}

/*
 * The sinker item shows what pressing it does: the sinker while the bobber's on (or a lure), the bobber while the
 * sinker's on (it takes the sinker off). Its icon and name in the item page's graphics, by the rig on the line.
 */
#define TACKLE_BOBBER_ICON 3
#define TACKLE_BOBBER_NAME 4
static s32 Fishmod_TackleShown(const FishmodItemDef* def, s32 item, s32 name) {
    if ((item == ITEM_SINKER) && (FISHMOD_RIG() == FISHMOD_RIG_SINKER)) {
        return name ? TACKLE_BOBBER_NAME : TACKLE_BOBBER_ICON;
    }
    return name ? def->name : def->lureIcon;
}

/** An item's icon while paused (the item page, the icon flying to a C button). */
void* Fishmod_PauseItemIcon(s32 item) {
    const FishmodItemDef* def = Fishmod_ItemDef(item);

    if (def != NULL) {
        if (def->ramIcon != NULL) {
            return def->ramIcon;
        }
        if (def->lureIcon != NO_PAGE_INDEX) {
            // (the tackle's icons as loaded for this pause: the third is the sinker item's, whichever it shows; if
            // they didn't fit, the rod's own icon, which is the game's and always there)
            return (sLureIconsRam != NULL) ? (void*)sLureIconsRam[MIN(def->lureIcon, LURE_ICONS_LOADED - 1)]
                                           : gItemIcons[ITEM_FISHING_POLE];
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
           Fishmod_TackleShown(def, item, false) * LURE_ICON_SIZE;
}

/** Where the pause menu loads one of the mod's name graphics from in ROM, or 0 (the game's own). */
uintptr_t Fishmod_ItemNameVrom(s32 item) {
    const FishmodItemDef* def = Fishmod_ItemDef(item);

    if ((def == NULL) || (def->name == NO_PAGE_INDEX)) {
        return 0;
    }
    return (uintptr_t)_fishmod_item_pageSegmentRomStart + offsetof(FishmodItemPage, names) +
           Fishmod_TackleShown(def, item, true) * sizeof(((FishmodItemPage*)0)->names[0]);
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
    static u8 sSpare;

    if ((u32)slot < INVENTORY_SLOTS_VANILLA) {
        return &gSaveContext.save.info.inventory.items[slot];
    }
    if ((u32)slot < INVENTORY_SLOTS) {
        return &FISHMOD_SAVE->items[slot - INVENTORY_SLOTS_VANILLA];
    }
    // past the inventory (no slot, or an equipment button's mark): a spare byte reading as no item
    sSpare = ITEM_NONE;
    return &sSpare;
}

/*
 * The D-pad as four more item buttons (4-7: up, down, left, right), set for each age as the C buttons are: in the
 * pause menu, press the D-pad on an item (again to take it off). Equipment (swords, shields, tunics, boots) can go on
 * the D-pad or the C buttons from the equipment page; pressing it equips it, and a tunic or boots already worn go
 * back to the Kokiri ones.
 */
#include "gfx.h"
#include "gfx_setupdl.h"
#include "interface.h"
#include "sfx.h"
#include "controller.h"
#include "z_lib.h"
#include "string.h"
#include "cutscene.h"
#include "pause.h"
#include "message.h"

#define DPAD_AGE() (((void)0, gSaveContext.save.linkAge) == LINK_AGE_CHILD)

static u16 sDpadButtons[4] = { BTN_DUP, BTN_DDOWN, BTN_DLEFT, BTN_DRIGHT };
static const u8 sArrowSlots[3] = { SLOT_ARROW_FIRE, SLOT_ARROW_ICE, SLOT_ARROW_LIGHT };

u8 Fishmod_ButtonSlot(s32 button) {
    if ((button >= 1) && (button <= 3)) {
        return gSaveContext.save.info.equips.cButtonSlots[button - 1];
    }
    if ((button >= FISHMOD_BUTTON_DPAD) && (button < FISHMOD_BUTTON_MAX)) {
        return FISHMOD_SAVE->dpad[DPAD_AGE()][button - FISHMOD_BUTTON_DPAD];
    }
    return SLOT_NONE;
}

/** The item a button's slot holds now (an equipment mark's equipment), or ITEM_NONE. */
u8 Fishmod_SlotItem(s32 slot) {
    if (FISHMOD_IS_EQUIP_SLOT(slot)) {
        s32 type = (slot - FISHMOD_SLOT_EQUIP) / 4;
        s32 value = (slot - FISHMOD_SLOT_EQUIP) % 4;

        return ((type < EQUIP_TYPE_MAX) && (value >= 1)) ? ITEM_SWORD_KOKIRI + type * 3 + value - 1 : ITEM_NONE;
    }
    if (FISHMOD_IS_BOW_ARROW_SLOT(slot)) {
        // the bow with an arrow set on it (plain once the arrow's gone; nothing without the bow)
        if (INV_SLOT(SLOT_BOW) != ITEM_BOW) {
            return ITEM_NONE;
        }
        return (INV_SLOT(sArrowSlots[slot - FISHMOD_SLOT_BOW_ARROW]) != ITEM_NONE)
                   ? ITEM_BOW_FIRE + (slot - FISHMOD_SLOT_BOW_ARROW)
                   : ITEM_BOW;
    }
    return ((u32)slot < INVENTORY_SLOTS) ? INV_SLOT(slot) : ITEM_NONE;
}

s32 Fishmod_IsEquipItem(s32 item) {
    return (item >= ITEM_SWORD_KOKIRI) && (item <= ITEM_BOOTS_HOVER);
}

/*
 * Whether a D-pad button works now: a status each, as the C buttons have (gSaveContext.buttonStatus), set by the
 * game's own code wherever it sets theirs. The rules by what's on a button are one function for all seven
 * (Interface_ItemButtonRule, z_parameter.c); where the game sets the C buttons' as one (riding, minigames, the pause
 * menu, a new file) it sets these with them (Fishmod_DpadSetStatus).
 */
u8 gFishmodDpadStatus[4]; // BTN_ENABLED or BTN_DISABLED, as gSaveContext.buttonStatus is for the C buttons

/** All four at once: wherever the game sets the three C buttons' as one (riding, minigames, the pause menu...). */
void Fishmod_DpadSetStatus(u8 status) {
    gFishmodDpadStatus[0] = gFishmodDpadStatus[1] = gFishmodDpadStatus[2] = gFishmodDpadStatus[3] = status;
}

/** The pause menu keeps the buttons' statuses while it's open and puts them back (as it does the C buttons'). */
void Fishmod_DpadSaveStatus(s32 save) {
    static u8 sSaved[4];
    s32 d;

    for (d = 0; d < 4; d++) {
        if (save) {
            sSaved[d] = gFishmodDpadStatus[d];
        } else {
            gFishmodDpadStatus[d] = sSaved[d];
        }
    }
}

static s32 Fishmod_DpadOn(PlayState* play, s32 d) {
#if FISHMOD_TESTMODE
    if (CHECK_BTN_ALL(play->state.input[0].cur.button, BTN_L)) {
        return false; // (L + D-pad: the test build's warps)
    }
#endif
    return gFishmodDpadStatus[d] != BTN_DISABLED;
}

/** What D-pad button (4-7) gives now (ITEM_NONE: nothing on it, or off). */
s32 Fishmod_DpadItem(PlayState* play, s32 button) {
    s32 item = Fishmod_SlotItem(Fishmod_ButtonSlot(button));

    // (a trade item sold out: the game takes it off a C button)
    if ((item == ITEM_NONE) || (item == ITEM_SOLD_OUT) || !Fishmod_DpadOn(play, button - FISHMOD_BUTTON_DPAD)) {
        return ITEM_NONE;
    }
    return item;
}

/** Whether a D-pad button holds this item (lit or not: the lens's magic, z_parameter.c, turns the lens off once no
 * C button holds it, whatever the buttons' state). */
s32 Fishmod_DpadHasItem(PlayState* play, s32 item) {
    s32 b;

    for (b = FISHMOD_BUTTON_DPAD; b < FISHMOD_BUTTON_MAX; b++) {
        if (Fishmod_SlotItem(Fishmod_ButtonSlot(b)) == item) {
            return true;
        }
    }
    return false;
}

// Who can wear what (the pause menu's gEquipAgeReqs, which lives in its overlay): 0 either age, else the LINK_AGE_*
// plus one
static const u8 sEquipAge[EQUIP_TYPE_MAX][4] = {
    { 0, LINK_AGE_CHILD + 1, LINK_AGE_ADULT + 1, LINK_AGE_ADULT + 1 }, // swords: Kokiri, Master, Biggoron
    { 0, LINK_AGE_CHILD + 1, 0, LINK_AGE_ADULT + 1 },                  // shields: Deku, Hylian, Mirror
    { 0, 0, LINK_AGE_ADULT + 1, LINK_AGE_ADULT + 1 },                  // tunics: Kokiri, Goron, Zora
    { 0, 0, LINK_AGE_ADULT + 1, LINK_AGE_ADULT + 1 },                  // boots: Kokiri, Iron, Hover
};

/** The equipment a type goes back to when what's worn is pressed again: the Kokiri tunic and boots; the Kokiri Sword
 * and Deku Shield as a child, the Master Sword and Hylian Shield as an adult. */
static s32 Fishmod_EquipDefault(s32 type) {
    if ((type == EQUIP_TYPE_SWORD) || (type == EQUIP_TYPE_SHIELD)) {
        return LINK_IS_ADULT ? 2 : 1;
    }
    return 1;
}

/** Equipment pressed on a button: equip it; pressed while it's already worn, back to the default (Fishmod_EquipDefault).
 * Returns true if anything changed. */
s32 Fishmod_EquipItem(PlayState* play, s32 item) {
    Player* player = GET_PLAYER(play);
    s32 type = (item - ITEM_SWORD_KOKIRI) / 3;
    s32 value = (item - ITEM_SWORD_KOKIRI) % 3 + 1;
    s32 owned;

    if (CUR_EQUIP_VALUE(type) == value) {
        value = Fishmod_EquipDefault(type); // (worn already: back to the default)
    }
    owned = (value == 1) && ((type == EQUIP_TYPE_TUNIC) || (type == EQUIP_TYPE_BOOTS)) // (always owned)
                ? true
                : CHECK_OWNED_EQUIP(type, value - 1);
    if ((type == EQUIP_TYPE_SWORD) && (value == EQUIP_VALUE_SWORD_BIGGORON)) {
        owned = owned || CHECK_OWNED_EQUIP_ALT(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BROKENGIANTKNIFE);
    }
    if ((sEquipAge[type][value] != 0) && (sEquipAge[type][value] != ((void)0, gSaveContext.save.linkAge) + 1)) {
        owned = false; // (not at this age)
    }
    // (not a sword or shield while one's in his hand: the held model would be the old one's)
    if (!owned || (CUR_EQUIP_VALUE(type) == value) ||
        (((type == EQUIP_TYPE_SWORD) || (type == EQUIP_TYPE_SHIELD)) &&
         ((player->heldItemAction != PLAYER_IA_NONE) || (player->stateFlags1 & PLAYER_STATE1_SHIELDING)))) {
        Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
        return false;
    }
    Inventory_ChangeEquipment(type, value);
    if (type == EQUIP_TYPE_SWORD) {
        // as the pause menu does
        gSaveContext.save.info.infTable[INFTABLE_INDEX_1DX] = 0;
        gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_KOKIRI + value - 1;
        if ((value == EQUIP_VALUE_SWORD_BIGGORON) && gSaveContext.save.info.playerData.bgsFlag) {
            gSaveContext.save.info.playerData.swordHealth = 8;
        } else if ((value == EQUIP_VALUE_SWORD_BIGGORON) &&
                   CHECK_OWNED_EQUIP_ALT(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BROKENGIANTKNIFE)) {
            gSaveContext.save.info.equips.buttonItems[0] = ITEM_GIANTS_KNIFE;
        }
        Interface_LoadItemIcon1(play, 0);
    }
    Player_SetEquipmentData(play, player);
    Sfx_PlaySfxCentered(NA_SE_SY_DECIDE);
    return true;
}

/**
 * Equipment on a C or D-pad button, pressed: put on (or, if worn, back to the default) at any time Link can be
 * controlled, even with the C buttons greyed out (swimming, climbing...), as the pause menu allows. Not while paused,
 * in a cutscene, with text up, or at a game over. Called from Player_UpdateCommon every frame; the item buttons' own
 * handling leaves equipment alone.
 */
void Fishmod_EquipButtons(PlayState* play, Input* input) {
    static const u16 sButtons[] = { BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT, BTN_DUP, BTN_DDOWN, BTN_DLEFT, BTN_DRIGHT };
    Player* player = GET_PLAYER(play);
    s32 i;

    if ((input == NULL) || (player == NULL) || (play->pauseCtx.state != PAUSE_STATE_OFF) ||
        (play->csCtx.state != CS_STATE_IDLE) || (Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) ||
        (gSaveContext.gameMode != GAMEMODE_NORMAL) || (gSaveContext.save.info.playerData.health <= 0) ||
        (player->csAction != PLAYER_CSACTION_NONE)) {
        return;
    }
#if FISHMOD_TESTMODE
    if (CHECK_BTN_ALL(input->cur.button, BTN_L)) {
        return; // (L + D-pad: the test build's warps)
    }
#endif
    for (i = 0; i < ARRAY_COUNT(sButtons); i++) {
        if (CHECK_BTN_ALL(input->press.button, sButtons[i])) {
            s32 slot = Fishmod_ButtonSlot(i + 1);

            if (FISHMOD_IS_EQUIP_SLOT(slot) && (Fishmod_SlotItem(slot) != ITEM_NONE)) {
                Fishmod_EquipItem(play, Fishmod_SlotItem(slot));
            }
        }
    }
}

/** A bottle on a D-pad button changed (as Inventory_UpdateBottleItem does for a C button). */
void Fishmod_DpadUpdateBottle(PlayState* play, u8 item, s32 button) {
    s32 slot = Fishmod_ButtonSlot(button);
    s32 i;

    if ((u32)slot >= INVENTORY_SLOTS) {
        return;
    }
    if ((INV_SLOT(slot) == ITEM_BOTTLE_MILK_FULL) && (item == ITEM_BOTTLE_EMPTY)) {
        item = ITEM_BOTTLE_MILK_HALF;
    }
    INV_SLOT(slot) = item;
    // the same bottle on a C button too
    for (i = 1; i <= 3; i++) {
        if (gSaveContext.save.info.equips.cButtonSlots[i - 1] == slot) {
            gSaveContext.save.info.equips.buttonItems[i] = item;
            Interface_LoadItemIcon1(play, i);
        }
    }
    play->pauseCtx.cursorItem[PAUSE_ITEM] = item;
}

/*
 * The pause menu: the D-pad buttons are C buttons, seven in all (0-2 C-Left, C-Down, C-Right; 3-6 D-pad up, down,
 * left, right). A slot (or an equipment mark) is on one button at a time; put on a button while it's on another, the
 * two trade (that other button takes what this one had), as the C buttons do among themselves; put on the button
 * it's on already, it comes off.
 */
static s32 Button_Get(s32 b) {
    if (b < 3) {
        s32 item = gSaveContext.save.info.equips.buttonItems[b + 1];

        // (the game keeps a bow with an arrow set on it as the bow's slot and the pair's item)
        if ((item >= ITEM_BOW_FIRE) && (item <= ITEM_BOW_LIGHT)) {
            return FISHMOD_SLOT_BOW_ARROW + (item - ITEM_BOW_FIRE);
        }
        return gSaveContext.save.info.equips.cButtonSlots[b];
    }
    return FISHMOD_SAVE->dpad[DPAD_AGE()][b - 3];
}

static void Button_Set(PlayState* play, s32 b, s32 slot) {
    if (b < 3) {
        gSaveContext.save.info.equips.cButtonSlots[b] = FISHMOD_IS_BOW_ARROW_SLOT(slot) ? SLOT_BOW : slot;
        gSaveContext.save.info.equips.buttonItems[b + 1] = (slot == SLOT_NONE) ? ITEM_NONE : Fishmod_SlotItem(slot);
        if (gSaveContext.save.info.equips.buttonItems[b + 1] != ITEM_NONE) {
            Interface_LoadItemIcon1(play, b + 1);
        } else {
            gSaveContext.save.info.equips.cButtonSlots[b] = SLOT_NONE;
        }
    } else {
        FISHMOD_SAVE->dpad[DPAD_AGE()][b - 3] = slot;
    }
}

/** What two buttons can't both hold: a slot, and the bow with or without an arrow on it is one thing. */
static s32 Button_Key(s32 slot) {
    return FISHMOD_IS_BOW_ARROW_SLOT(slot) ? SLOT_BOW : slot;
}

void Fishmod_ButtonPut(PlayState* play, s32 b, s32 slot) {
    s32 old = Button_Get(b);
    s32 j;

    if (old == slot) {
        if (!FISHMOD_IS_BOW_ARROW_SLOT(slot)) {
            Button_Set(play, b, SLOT_NONE);
        }
        return; // (an arrow set again on the bow that has it stays, as on a C button)
    }
    if (Button_Key(old) != Button_Key(slot)) {
        for (j = 0; j < 7; j++) {
            if ((j != b) && (Button_Key(Button_Get(j)) == Button_Key(slot))) {
                Button_Set(play, j, old);
            }
        }
    }
    Button_Set(play, b, slot);
}

/**
 * The pause menu: put a slot on the D-pad button pressed (Fishmod_ButtonPut). A fire, ice or light arrow goes on as
 * the bow with it set, as on a C button. Returns false if no button was pressed (or the arrow has no bow).
 */
s32 Fishmod_DpadAssign(PlayState* play, u16 press, s32 slot) {
    s32 d;

    for (d = 0; d < ARRAY_COUNT(sArrowSlots); d++) {
        if (slot == sArrowSlots[d]) {
            if (INV_SLOT(SLOT_BOW) != ITEM_BOW) {
                return false;
            }
            slot = FISHMOD_SLOT_BOW_ARROW + d;
            break;
        }
    }
    for (d = 0; d < 4; d++) {
        if (CHECK_BTN_ALL(press, sDpadButtons[d])) {
            Fishmod_ButtonPut(play, 3 + d, slot);
            return true;
        }
    }
    return false;
}

/**
 * The item page is putting a slot on C button `c` (0-2; vanilla's animation, with its own trades among the C
 * buttons), about to write it: a D-pad button with the slot (the bow's, for a bow with or without an arrow) takes
 * what that C button has.
 */
void Fishmod_DpadTrade(s32 slot, s32 c) {
    u8* dpad = FISHMOD_SAVE->dpad[DPAD_AGE()];
    s32 old = Button_Get(c);
    s32 i;

    if ((slot == SLOT_NONE) || (Button_Key(old) == slot)) {
        return;
    }
    for (i = 0; i < 4; i++) {
        if (Button_Key(dpad[i]) == slot) {
            dpad[i] = old;
        }
    }
}

/** An item is gone from its slot for good (Inventory_DeleteItem clears the C buttons with it): off the D-pad too. */
void Fishmod_DpadDeleteSlot(s32 slot) {
    s32 age;
    s32 i;

    for (age = 0; age < 2; age++) {
        for (i = 0; i < 4; i++) {
            if (FISHMOD_SAVE->dpad[age][i] == slot) {
                FISHMOD_SAVE->dpad[age][i] = SLOT_NONE;
            }
        }
    }
}

/*
 * The HUD: a D-pad under the C buttons, shaded as they are (fishmod/tools/gen_dpad_tex.py: a bevelled cross with an
 * arrow pressed into each arm), its items at 16 x 16 over the arrows with their ammo counts as the C buttons show them
 * (green when full, grey when none). Their icons are read from ROM as they change, into a buffer of their own. Shown
 * with the C buttons once anything is on it; an item dimmed while it can't be used.
 */
#include "src/fishmod_data/fishmod_dpad_tex.inc.c"

#define DPAD_X 282 // the cross's middle
#define DPAD_Y 88
#define DPAD_ARM 17 // from the middle to an item's
#define DPAD_TEX 48
#define DPAD_DIGIT 6 // the ammo digits' size (the C buttons': 8)

#include "assets/textures/parameter_static/parameter_static.h" // (segment 2 while the HUD draws)

Gfx* Gfx_TextureIA8(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight, s16 rectLeft, s16 rectTop,
                    s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy); // (z_parameter.c)

static u64 sDpadIcons[4][ITEM_ICON_SIZE / sizeof(u64)];
static u16 sDpadIconItem[4]; // the item each holds, + 1 (0: none loaded); + 0x100 for the sinker showing the bobber
#define DPAD_ICON_KEY(item) ((item) + 1 + (((item) == ITEM_SINKER) && (FISHMOD_RIG() == FISHMOD_RIG_SINKER) ? 0x100 : 0))

static void Fishmod_DpadLoadIcon(s32 d, s32 item) {
    if (Fishmod_ItemIcon(item) != NULL) {
        bcopy(Fishmod_ItemIcon(item), sDpadIcons[d], ITEM_ICON_SIZE);
    } else {
        DmaMgr_RequestSync(sDpadIcons[d],
                           (Fishmod_ItemIconVrom(item) != 0) ? Fishmod_ItemIconVrom(item) : GET_ITEM_ICON_VROM(item),
                           ITEM_ICON_SIZE);
    }
    sDpadIconItem[d] = DPAD_ICON_KEY(item);
}

/** An item's ammo count as Interface_DrawAmmoCount shows it (-1: none shown); *full set when at its capacity. */
static s32 Fishmod_DpadAmmo(s32 item, s32* full) {
    s32 ammo;

    if ((item >= ITEM_BOW_FIRE) && (item <= ITEM_BOW_LIGHT)) {
        item = ITEM_BOW;
    }
    if (!((item == ITEM_DEKU_STICK) || (item == ITEM_DEKU_NUT) || (item == ITEM_BOMB) || (item == ITEM_BOW) ||
          (item == ITEM_SLINGSHOT) || (item == ITEM_BOMBCHU) || (item == ITEM_MAGIC_BEAN))) {
        return -1;
    }
    ammo = AMMO(item);
    *full = ((item == ITEM_BOW) && (ammo == CUR_CAPACITY(UPG_QUIVER))) ||
            ((item == ITEM_BOMB) && (ammo == CUR_CAPACITY(UPG_BOMB_BAG))) ||
            ((item == ITEM_SLINGSHOT) && (ammo == CUR_CAPACITY(UPG_BULLET_BAG))) ||
            ((item == ITEM_DEKU_STICK) && (ammo == CUR_CAPACITY(UPG_DEKU_STICKS))) ||
            ((item == ITEM_DEKU_NUT) && (ammo == CUR_CAPACITY(UPG_DEKU_NUTS))) ||
            ((item == ITEM_BOMBCHU) && (ammo == 50)) || ((item == ITEM_MAGIC_BEAN) && (ammo == 15));
    return CLAMP(ammo, 0, 99);
}

void Fishmod_DpadDraw(PlayState* play) {
    static const s8 sOffset[4][2] = { { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } };
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    s16 alpha = MAX(MAX(interfaceCtx->cLeftAlpha, interfaceCtx->cDownAlpha), interfaceCtx->cRightAlpha);
    s32 any = false;
    s32 d;

    // (shown with the C buttons once anything is on it: an empty direction then shows its arrow)
    for (d = 0; d < 4; d++) {
        if (Fishmod_SlotItem(FISHMOD_SAVE->dpad[DPAD_AGE()][d]) != ITEM_NONE) {
            any = true;
        }
    }
    if (!any || (alpha == 0)) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx, "fishmod_items.c", __LINE__);

    // the pad (its texture's intensity times the grey, as the C buttons' times their yellow)
    gDPPipeSync(OVERLAY_DISP++);
    gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 143, 143, 150, alpha); // (the N64 controller's D-pad grey on screen)
    gDPLoadTextureBlock(OVERLAY_DISP++, sFishmodDpadTex, G_IM_FMT_IA, G_IM_SIZ_8b, DPAD_TEX, DPAD_TEX, 0,
                        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    gSPTextureRectangle(OVERLAY_DISP++, (DPAD_X - DPAD_TEX / 2) << 2, (DPAD_Y - DPAD_TEX / 2) << 2,
                        (DPAD_X + DPAD_TEX / 2) << 2, (DPAD_Y + DPAD_TEX / 2) << 2, G_TX_RENDERTILE, 0, 0, 1 << 10,
                        1 << 10);

    for (d = 0; d < 4; d++) {
        s32 item = Fishmod_SlotItem(FISHMOD_SAVE->dpad[DPAD_AGE()][d]);
        s32 x = DPAD_X + sOffset[d][0] * DPAD_ARM - 8;
        s32 y = DPAD_Y + sOffset[d][1] * DPAD_ARM - 8;
        s16 a;
        s32 full = false;
        s32 ammo;

        if ((item == ITEM_NONE) || (item >= ITEM_NONE_FE) || (item == ITEM_SOLD_OUT)) {
            continue;
        }
        if (sDpadIconItem[d] != DPAD_ICON_KEY(item)) {
            Fishmod_DpadLoadIcon(d, item);
        }
        a = Fishmod_DpadOn(play, d) ? alpha : MIN(alpha, 70);
        // the item, at half size
        gDPPipeSync(OVERLAY_DISP++);
        gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, a);
        gDPLoadTextureBlock(OVERLAY_DISP++, sDpadIcons[d], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(OVERLAY_DISP++, x << 2, y << 2, (x + 16) << 2, (y + 16) << 2, G_TX_RENDERTILE, 0, 0,
                            2 << 10, 2 << 10);
        // its ammo, at its lower left (the C buttons' digits, smaller)
        ammo = Fishmod_DpadAmmo(item, &full);
        if (ammo >= 0) {
            s32 dx = x;
            s32 dy = y + 16 - DPAD_DIGIT + 1;

            gDPPipeSync(OVERLAY_DISP++);
            gDPSetCombineLERP(OVERLAY_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0,
                              PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
            if (ammo == 0) {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 100, 100, 100, a);
            } else if (full) {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 120, 255, 0, a);
            } else {
                gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, a);
            }
            if (ammo >= 10) {
                OVERLAY_DISP = Gfx_TextureIA8(OVERLAY_DISP, (u8*)gAmmoDigit0Tex + 8 * 8 * (ammo / 10), 8, 8, dx, dy,
                                              DPAD_DIGIT, DPAD_DIGIT, (8 << 10) / DPAD_DIGIT, (8 << 10) / DPAD_DIGIT);
                dx += DPAD_DIGIT - 1;
            }
            OVERLAY_DISP = Gfx_TextureIA8(OVERLAY_DISP, (u8*)gAmmoDigit0Tex + 8 * 8 * (ammo % 10), 8, 8, dx, dy,
                                          DPAD_DIGIT, DPAD_DIGIT, (8 << 10) / DPAD_DIGIT, (8 << 10) / DPAD_DIGIT);
        }
    }
    gDPPipeSync(OVERLAY_DISP++);

    CLOSE_DISPS(play->state.gfxCtx, "fishmod_items.c", __LINE__);
}

/*
 * The clock under the overworld minimap (fishmod/tools/gen_clock_tex.py): an arc from midnight to noon to midnight in
 * the sky's colours, the sun (day) or moon (night) riding it at the time of day. It fades with the minimap. The
 * minimap is raised only as far as the clock needs to end above a CRT's safe area. Hidden where time is frozen.
 */
#include "src/fishmod_data/fishmod_clock_tex.inc.c"
#include "environment.h" // (gTimeSpeed)


#define CLOCK_SAFE_BOTTOM 227 // (a CRT may hide the lines below)

s16 gFishmodMinimapLift;

s32 Fishmod_ClockLift(s32 mapBottom) {
    // the clock: 2 below the map, the arc, and half an icon below the arc's ends
    s32 bottom = mapBottom + 2 + FISHMOD_CLOCK_ARC_H + FISHMOD_CLOCK_ICON / 2 - 3;

    return MAX(0, bottom - CLOCK_SAFE_BOTTOM);
}

void Fishmod_ClockDraw(PlayState* play, s32 centerX, s32 top, s16 alpha) {
    s32 x0 = centerX - FISHMOD_CLOCK_ARC_W / 2;
    s32 t = ((u16)((void)0, gSaveContext.save.dayTime) * (FISHMOD_CLOCK_ARC_W - 1)) >> 16; // the column for now
    s32 day = (gSaveContext.save.dayTime >= CLOCK_TIME(6, 0)) && (gSaveContext.save.dayTime < CLOCK_TIME(18, 0));
    s32 ix = x0 + t - FISHMOD_CLOCK_ICON / 2;
    s32 iy = top + sFishmodClockArcY[t] + 1 - FISHMOD_CLOCK_ICON / 2;

    // not where time stands still (the scene's time speed is 0: the mod's hidden clock runs on underneath, and the
    // shown time would only mislead)
    if ((alpha == 0) || (gTimeSpeed == 0)) {
        return;
    }
    alpha = MIN(255, alpha * 255 / 0xAA); // (the minimap itself is drawn see-through, at 0xAA at most: not the clock)
    OPEN_DISPS(play->state.gfxCtx, "fishmod_items.c", __LINE__);

    gDPPipeSync(OVERLAY_DISP++);
    gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, alpha);
    gDPLoadTextureBlock(OVERLAY_DISP++, sFishmodClockArcTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, FISHMOD_CLOCK_ARC_W,
                        FISHMOD_CLOCK_ARC_H, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK,
                        G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gSPTextureRectangle(OVERLAY_DISP++, x0 << 2, top << 2, (x0 + FISHMOD_CLOCK_ARC_W) << 2,
                        (top + FISHMOD_CLOCK_ARC_H) << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    gDPPipeSync(OVERLAY_DISP++);
    gDPLoadTextureBlock(OVERLAY_DISP++, day ? sFishmodClockSunTex : sFishmodClockMoonTex, G_IM_FMT_RGBA,
                        G_IM_SIZ_32b, FISHMOD_CLOCK_ICON, FISHMOD_CLOCK_ICON, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gSPTextureRectangle(OVERLAY_DISP++, ix << 2, iy << 2, (ix + FISHMOD_CLOCK_ICON) << 2,
                        (iy + FISHMOD_CLOCK_ICON) << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    gDPPipeSync(OVERLAY_DISP++);

    CLOSE_DISPS(play->state.gfxCtx, "fishmod_items.c", __LINE__);
}
