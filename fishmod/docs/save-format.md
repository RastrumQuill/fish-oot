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
- **The mod's debug builds use the unused tail** (`0x7A40` play log, `0x7B00` stall record, `0x7C00` fault record,
  `0x7D00`-`0x7F00` collision/profile/stats records). The benches write their results at `0x0000`, over the header
  and file 1: bench ROMs must never be run on a cart with a real save on it (they're debug-only).

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

The mod's block, `FishmodSaveData` (`include/fishmod.h`, 0xAA bytes), sits in the scene-flag entries past the last
real scene (`sceneFlags[SCENE_ID_MAX]` on, 14 entries = 392 bytes that nothing in the game reads or writes; checked:
every vanilla index into `sceneFlags` is a scene id or a dungeon map index). It's inside `Save`, so the checksum and
the backup cover it. It holds: flags (bit 0: the item page's new slots are set up), the species records, the bait
pouch, six bottled fish, and the item page's six new slots.

Room left, all inside the checksummed `Save`:

| Where | Bytes | Notes |
|---|---|---|
| `sceneFlags[110..123]` after the mod's block | 222 | the easiest: already the mod's |
| `sceneFlags[*].unk` | 110 × 4 = 440 | one unused word per scene, never read or written (good for per-scene extras) |
| the named gaps (`0x0E8C`, `0x0EB4`, `0x0F34`, `0x0F3C`, `0x12A1`, `0x1346`, `playerData` padding) | 16+4+4+4+36+2+19 = 85 | |

A bigger save (the pictograph box's photos, roadmap item 5) needs a different save type (MM's 128 KB flash): 32 KB
SRAM has only `0x600` bytes free after the six slots.

## 4. The original game's save and flag bugs

(see below)
