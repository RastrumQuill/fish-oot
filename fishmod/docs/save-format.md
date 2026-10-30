# The save file: format, the mod's additions, and the original game's save bugs

Research only: nothing here is changed yet. It maps the save (NTSC 1.2, from the decompilation), what the mod
already stores in it, where there's room, and the original game's flag and save bugs worth fixing, with a plan for
fixing them without breaking existing saves.

## 1. SRAM (32 KB battery save)

| Offset | Size | What |
|---|---|---|
| `0x0000` | `0x10` | Header: sound mode, Z-target mode, language, a magic byte that marks the SRAM as formatted |
| `0x0010` | `0x10` | (padding) |
| `0x0020` | `0x1450` | File 1 |
| `0x1470` | `0x1450` | File 2 |
| `0x28C0` | `0x1450` | File 3 |
| `0x3D10` | `0x1450` | File 1 backup |
| `0x5160` | `0x1450` | File 2 backup |
| `0x65B0` | `0x1450` | File 3 backup |
| `0x7A00` | `0x600` | Unused by the game |

- **A slot is the whole `SaveContext` plus 0x28 bytes** (`SLOT_SIZE`), but only the first `0x1354` bytes (`Save`)
  are read back; the rest is runtime state written for nothing.
- **Checksum:** a plain 16-bit sum of the `Save` words, stored in the save (`checksum`). Saving writes the file and
  then its backup with the same contents. On boot (`Sram_VerifyAndLoadAllSaves`) a file whose sum is wrong is
  replaced by its backup if the backup's sum is right, else it becomes a new file.
- **The mod's debug builds use the unused tail** (fingerprints, play log, stall, fault, collision, profile and stats
  records), all listed in one map, `include/fishmod_sram.h` (the Python tools read it through
  `fishmod/tools/sram_map.py`). The benches and the LOD jobs write their results over the saves, from `0x0000`: those
  ROMs must never be run on a cart with a real save on it (they're debug-only).

## 2. The save (`Save`, 0x1354 bytes)

| Offset | Size | Field | Notes |
|---|---|---|---|
| `0x0000` | 0x1C | entrance, age, cutscene, time of day, night, days | |
| `0x001C` | 0x4C | `playerData` | "ZELDAZ" marker, deaths, name, hearts, magic, rupees, BGS durability, child/adult equips, saved scene; 0x0E + 4 + 1 unused bytes |
| `0x0068` | 0x0A | `equips` | B/C items, C-button slots, equipment worn |
| `0x0074` | 0x5E | `inventory` | 24 item slots, 16 ammo counts, equipment owned, upgrades, quest items, dungeon items/keys, double defence hearts, Gold Skulltula count |
| `0x00D4` | 124 × 0x1C | `sceneFlags` | per scene: chest, switch, clear, collectible, `unk`, map rooms, map floors |
| `0x0E64` | 0x28 | `fw` | Farore's Wind point |
| `0x0E8C` | 0x10 | (unused) | |
| `0x0E9C` | 0x18 | `gsFlags` | Gold Skulltulas found, per area |
| `0x0EB4` | 4 | (unused) | |
| `0x0EB8` | 0x1C | `highScores` | horseback archery, Big Poe points, fishing (several values packed), horse race, marathon, one unused, Dampé race |
| `0x0ED4` | 0x1C | `eventChkInf` | 224 story/event flags |
| `0x0EF0` | 0x08 | `itemGetInf` | 64 "got this item from this NPC" flags |
| `0x0EF8` | 0x3C | `infTable` | 480 NPC dialogue/state flags |
| `0x0F34` | 4 | (unused) | |
| `0x0F38` | 4 | `worldMapAreaData` | areas visited (map) |
| `0x0F3C` | 4 | (unused) | |
| `0x0F40` | 0x361 | scarecrow's long song | |
| `0x12A1` | 0x24 | (unused) | |
| `0x12C5` | 0x81 | scarecrow's spawn song | |
| `0x1346` | 2 | (unused) | |
| `0x1348` | 0x0A | `horseData` | where Epona was left |
| `0x1352` | 2 | `checksum` | |

**Not saved:** `eventInf` (minigame and temporary event state), the per-scene *temporary* switch and collectible
flags (0x20-0x3F, reset when you leave the scene), the respawn points (except Farore's Wind), magic level (reset to
0 on load and set again from `isMagicAcquired`).

### What loading a file changes (`Sram_OpenSave`)

- Where you start: dungeons (and their boss rooms) put you at the dungeon's entrance; anywhere else, Link's house
  (child) or the Temple of Time (adult).
- Health at least 3 hearts.
- Zelda's Letter taken back (and the meeting with Zelda undone) if Zelda's Lullaby wasn't learned yet.
- An adult without the Master Sword gets it back (1.1 and later also equip it).
- Spoiling trade items (Odd Mushroom, Odd Potion, the Eyeball Frog, Eye Drops... `gSpoilingItems`) go back a step.

## 3. What the mod stores, and where there's room

The mod's block, `FishmodSaveData` (`include/fishmod.h`, 0xAA bytes), sits in the scene-flag entries past every
scene: `sceneFlags[110]` on, a fixed place whatever the build (this NTSC 1.2 build has 101 scenes, the debug ROM's
test maps make 110), 14 entries = 392 bytes that nothing in the game reads or writes (checked: every vanilla index
into `sceneFlags` is a scene id or a dungeon map index). Builds before the refactor kept it at `sceneFlags[101]`
(`SCENE_ID_MAX` of this build); on load, a block without its mark ('FM') is moved from there once
(`Fishmod_InitSaveExtras`). It's inside `Save`, so the checksum and the backup cover it. It holds: the mark, a
version, flags (bit 0: the item page's new slots are set up), the species records (by each species' permanent
`saveId` in `fishmod/data/species.json`), the bait pouch, six bottled fish (also by `saveId`), and the item page's six
new slots.

Room left, all inside the checksummed `Save`:

| Where | Bytes | Notes |
|---|---|---|
| `sceneFlags[110..123]` after the mod's block | 222 | the easiest: already the mod's |
| `sceneFlags[101..109]` (the old place, cleared by the move) | 252 | free again |
| `sceneFlags[*].unk` | 110 × 4 = 440 | one unused word per scene, never read or written (good for per-scene extras) |
| the named gaps (`0x0E8C`, `0x0EB4`, `0x0F34`, `0x0F3C`, `0x12A1`, `0x1346`, `playerData` padding) | 16+4+4+4+36+2+19 = 85 | |

A bigger save (the pictograph box's photos, roadmap item 5) needs a different save type (MM's 128 KB flash): 32 KB
SRAM has only `0x600` bytes free after the six slots.

## 4. The original game's save and flag bugs

How sure: **code** = seen in the decompilation here; **SoH/Rando** = Ship of Harkinian or the OoT Randomizer fix
it (their source read); **wiki** = from wiki or forum summaries only (TCRF, ZeldaSpeedRuns and Cloudmodding were
blocked from this session, so worth reading directly before acting).

### 4a. A flag shared by two things

| Bug | What the player sees | Flag / place | Sure | Fix elsewhere |
|---|---|---|---|---|
| **Poacher's Saw takes the Deku Nut upgrade** | After getting the Saw, the Deku Theater (Lost Woods stage) never gives the 40-nut upgrade | `Item_Give(ITEM_POACHERS_SAW)` sets `ITEMGETINF_FOREST_STAGE_NUT_UPGRADE` (`z_parameter.c`, marked `@bug`) | code | SoH `Fixes/DekuNutUpgradeFix.cpp` (don't set it; for saves where it's set already, clear it if the nut upgrade isn't owned); Rando `Patches.py` |
| Gerudo "caught before" is reused for the tower escape | Nothing: Zelda clears it in the collapse, and you can't save after Ganon | `EVENTCHKINF_GERUDO_CAUGHT_TOWER_FALL` | code | (none needed) |
| Kakariko gate opens for child once the Master Sword is pulled | The guard lets child through without Zelda's Letter | `z_bg_gate_shutter.c`: `INFTABLE_76` **or** `EVENTCHKINF_OBTAINED_MASTER_SWORD` | code | a broad check, not a shared bit; Rando calls it shared |
| Windmill "fast" state takes a saved bit | Nothing (cleared on entering the scene) | `EVENTCHKINF_65` | code | |

Nothing else shared turned up: a scan of every saved flag (where each is set and read) found the others are the same
event set from two places (the Goron pair, the carpenters), not two things on one bit. MQ-only and
randomizer-only ones (Water Temple MQ lock, Shadow redeads, Dodongo's gossip fairy) don't touch this game.

### 4b. State lost, repeatable, or checked wrong

| Bug | What the player sees | Field / place | Sure | Fix elsewhere |
|---|---|---|---|---|
| **Gravedigging Tour heart piece lost** | Leave (or the game reloads) before picking up the piece Dampé dug up, and it's gone for good | `ITEMGETINF_1C` set when he **digs**, not when it's collected (`z_en_tk.c`) | code | SoH `GravediggingTourFix` |
| **Gold Skulltula token counted twice** | Leave before the "you got a token" box closes (e.g. a void), and the same spider gives another token | `Item_Give(ITEM_SKULL_TOKEN)` on touch; `SET_GS_FLAGS` only when the box closes (`z_en_si.c`); `gsTokens` has no cap | code | none found |
| **Broken Giant's Knife never cleared** | Owning Biggoron's Sword still counts the broken knife: shop and pause-menu logic see both | the `EQUIP_INV_SWORD_BROKENGIANTKNIFE` bit stays set | code | SoH `Fixes/BrokenGiantsKnife.cpp` |
| Goron City doors stay shut | Beat the Fire Temple without talking to Link the Goron: the shop doors stay closed | `z_bg_spot18_shutter.c` checks only `INFTABLE_109` | code (SoH) | SoH `Fixes/GoronCityDoorFix.cpp` |
| Kokiri lines don't match the story | Get Zelda's Letter before the Kokiri Emerald: Kokiri talk and act as if the Deku Tree were alive | flags unknown | SoH | SoH `Fixes/FixKokiriForestQuestState.cpp` |
| Zora hint after King Zora moved | The Zora still talks about Ruto's Letter | | SoH | SoH `FixZoraHintDialogue` |
| A world-map hint flag nothing sets | Kokiri Forest is never highlighted by that rule (map only) | `EVENTCHKINF_6E` read in `z_kaleido_scope.c`, never set | code | |
| Half the scene's switch and collectible flags are temporary | Things reset on leaving a scene; each case needs checking (most are meant to) | flags 0x20-0x3F (`tempSwch`, `tempCollect`) aren't saved | code | case by case |
| Swordless state and B button | After a sword is lost (Ganon's knock-away), save and reload can leave B empty | `infTable[INFTABLE_INDEX_1DX]`; B's enabled state isn't in the save | code + wiki | |

### 4c. The save itself

| Issue | Effect | Sure |
|---|---|---|
| Checksum is a plain 16-bit sum | weak: swapped or offsetting errors pass | code |
| Both copies bad → the file silently becomes a new file | a damaged save is lost without a word | code |
| `dayTime` (u16) cleared as 32 bits (`z_sram.c`, marked `@bug`) | harmless now, **breaks if the save layout moves** | code |
| `gsTokens` uncapped (see the token bug) | the count can pass 100 | code |
| Beans bought stored in the Hammer's ammo slot (`BEANS_BOUGHT = AMMO(ITEM_MAGIC_BEAN + 1)`) | a quirk, no bug; matters to anything that changes ammo slots (the item page kept all ammo where it was) | code |
| Big Poe points in a high score (+100 each, reward at 1000, capped 1100); fishing packs two records and seven flags into one value | quirks, no bug found | code |
| Ruto's Letter / bottle duplication, trade item loss, saving in cutscenes | only with glitches (Bottle Adventure, Get-Item Manipulation), not in normal play | wiki |

## 5. Fixing them later: the plan (nothing done yet)

- **Never move the vanilla layout.** Existing saves and the checksum keep working; everything new goes in the mod's
  block or the spare space above.
- **A save version.** `FishmodSaveData` has a version byte, and its flags mark what's been set up (bit 0, the item page). Each fix that
  must repair an old save gets a bit: on load (`Fishmod_InitSaveExtras`, called from `Sram_OpenSave`), a save
  without the bit is repaired once and marked. E.g. the nut upgrade: clear the stage flag if the upgrade isn't owned
  (as SoH does).
- **Moving a flag:** for the shared-bit cases, the second user gets a new flag in the mod's block, read and written
  through one function, so the old bit is left to its first owner.
- **Order bugs** (heart piece, token): set the flag when the thing is actually received, not when it appears.
- **One table of fixes**, each with its name, what it changes, and its save bit, so the refactor's "one framework"
  covers them and a build can list what it fixes. Fixes that change what a player sees in normal play (the Kakariko
  gate, the Kokiri lines) are decisions, not bugs: ask before including them.
- **Order of work:** the clear bugs first (nut upgrade, heart piece, token, the knife, Goron doors), then read TCRF's
  bug page and the ZeldaSpeedRuns pages directly for anything missing here, then the save's own weak spots (a
  stronger checksum would make vanilla files read as corrupt, so it would need its own marker).

### References
- The decompilation here: `include/save.h` (structs, every flag table), `src/code/z_sram.c` (slots, checksum,
  loading).
- Ship of Harkinian's fixes: https://github.com/HarbourMasters/Shipwright/tree/develop/soh/soh/Enhancements/Fixes
- OoT Randomizer's patches: https://github.com/OoTRandomizer/OoT-Randomizer/blob/Dev/Patches.py
- TCRF bug list: https://tcrf.net/Bugs:The_Legend_of_Zelda:_Ocarina_of_Time
- Cloudmodding save format: https://wiki.cloudmodding.com/oot/Save_Format
