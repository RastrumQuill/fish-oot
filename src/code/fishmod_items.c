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

#define DPAD_AGE() (((void)0, gSaveContext.save.linkAge) == LINK_AGE_CHILD)

static u16 sDpadButtons[4] = { BTN_DUP, BTN_DDOWN, BTN_DLEFT, BTN_DRIGHT };

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
    return ((u32)slot < INVENTORY_SLOTS) ? INV_SLOT(slot) : ITEM_NONE;
}

s32 Fishmod_IsEquipItem(s32 item) {
    return (item >= ITEM_SWORD_KOKIRI) && (item <= ITEM_BOOTS_HOVER);
}

/** Whether the D-pad works now: as the C buttons (off when they all are: cutscenes, minigames, riding...), not in
 * test builds while L is held (L + D-pad warps), and an item also on a C button only when that button works. */
static s32 Fishmod_DpadOn(PlayState* play, s32 item) {
    s32 i;

    if ((gSaveContext.buttonStatus[1] == BTN_DISABLED) && (gSaveContext.buttonStatus[2] == BTN_DISABLED) &&
        (gSaveContext.buttonStatus[3] == BTN_DISABLED)) {
        return false;
    }
#if FISHMOD_TESTMODE
    if (CHECK_BTN_ALL(play->state.input[0].cur.button, BTN_L)) {
        return false;
    }
#endif
    for (i = 1; i <= 3; i++) {
        if ((gSaveContext.save.info.equips.buttonItems[i] == item) && (gSaveContext.buttonStatus[i] == BTN_DISABLED)) {
            return false;
        }
    }
    return true;
}

/** What D-pad button (4-7) gives now (ITEM_NONE: nothing on it, or off). */
s32 Fishmod_DpadItem(PlayState* play, s32 button) {
    s32 item = Fishmod_SlotItem(Fishmod_ButtonSlot(button));

    if ((item == ITEM_NONE) || !Fishmod_DpadOn(play, item)) {
        return ITEM_NONE;
    }
    return item;
}

/** Equipment pressed on a button: equip it (a tunic or boots already worn: back to the Kokiri ones). Returns true if
 * anything changed. */
s32 Fishmod_EquipItem(PlayState* play, s32 item) {
    Player* player = GET_PLAYER(play);
    s32 type = (item - ITEM_SWORD_KOKIRI) / 3;
    s32 value = (item - ITEM_SWORD_KOKIRI) % 3 + 1;
    s32 owned = CHECK_OWNED_EQUIP(type, value - 1);

    if ((type == EQUIP_TYPE_SWORD) && (value == EQUIP_VALUE_SWORD_BIGGORON)) {
        owned = owned || CHECK_OWNED_EQUIP_ALT(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BROKENGIANTKNIFE);
    }
    if (((type == EQUIP_TYPE_TUNIC) || (type == EQUIP_TYPE_BOOTS)) && (CUR_EQUIP_VALUE(type) == value)) {
        value = 1; // (the Kokiri tunic or boots: always owned)
        owned = true;
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

/** The pause menu: put a slot (or an equipment mark) on the D-pad button pressed; on it already, take it off. A slot
 * goes on one D-pad button at a time. Returns false if no D-pad button was pressed. */
s32 Fishmod_DpadAssign(u16 press, s32 slot) {
    u8* dpad = FISHMOD_SAVE->dpad[DPAD_AGE()];
    s32 d;
    s32 i;

    for (d = 0; d < 4; d++) {
        if (CHECK_BTN_ALL(press, sDpadButtons[d])) {
            break;
        }
    }
    if (d == 4) {
        return false;
    }
    if (dpad[d] == slot) {
        dpad[d] = SLOT_NONE;
        return true;
    }
    for (i = 0; i < 4; i++) {
        if (dpad[i] == slot) {
            dpad[i] = SLOT_NONE;
        }
    }
    dpad[d] = slot;
    return true;
}

/*
 * The HUD: a D-pad under the C buttons, shaded as they are (fishmod/tools/gen_dpad_tex.py: a bevelled cross with an
 * arrow pressed into each arm), its items at 16 x 16 over the arrows with their ammo counts as the C buttons show them
 * (green when full, grey when none). Their icons are read from ROM as they change, into a buffer of their own. Shown
 * with the C buttons, empty or not; an item dimmed while it can't be used.
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
static u16 sDpadIconItem[4]; // the item each holds, + 1 (0: none loaded)

static void Fishmod_DpadLoadIcon(s32 d, s32 item) {
    if (Fishmod_ItemIcon(item) != NULL) {
        bcopy(Fishmod_ItemIcon(item), sDpadIcons[d], ITEM_ICON_SIZE);
    } else {
        DmaMgr_RequestSync(sDpadIcons[d],
                           (Fishmod_ItemIconVrom(item) != 0) ? Fishmod_ItemIconVrom(item) : GET_ITEM_ICON_VROM(item),
                           ITEM_ICON_SIZE);
    }
    sDpadIconItem[d] = item + 1;
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
    s32 d;

    // (always shown with the C buttons, empty or not: an empty direction shows its arrow)
    if (alpha == 0) {
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

        if ((item == ITEM_NONE) || (item >= ITEM_NONE_FE)) {
            continue;
        }
        if (sDpadIconItem[d] != item + 1) {
            Fishmod_DpadLoadIcon(d, item);
        }
        a = Fishmod_DpadOn(play, item) ? alpha : MIN(alpha, 70);
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
