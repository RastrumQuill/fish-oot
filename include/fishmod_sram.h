#ifndef FISHMOD_SRAM_H
#define FISHMOD_SRAM_H

/*
 * Where fishmod's debug and bench builds write in SRAM (32 KB). The one map: the C code takes the addresses from
 * here, and the Python tools read this file (fishmod/tools/sram_map.py), so the two can't drift apart.
 *
 * 0x0000-0x79FF holds the game's six save slots (three files and their backups). Records past that are safe for a
 * real save; records inside it are written only by ROMs that must run on a blank save (the benches, the LOD jobs),
 * and say so. The ones written by every script build (the fingerprints, the stall and fault records, the timing
 * totals) are all past the saves. A job that needs a big area owns it, and the records that would land in it are
 * left out of that build (noted per record).
 *
 * The save block (fishmod's catch records, items, bottled fish) isn't here: it's inside each save file, in the
 * unused scene flags (FISHMOD_SAVE in fishmod.h; fishmod/docs/save-format.md).
 */

#define FISHMOD_SRAM_SAVES_END 0x7A00 // the end of the six save slots (fishmod/docs/save-format.md)

// Over the saves: blank-save ROMs only
#define FISHMOD_SRAM_BENCH 0x0000     // the benches' results (play and area benches), and the LOD jobs' sweeps
#define FISHMOD_SRAM_LOD_DIAG 0x1000  // the LOD jobs' diagnosis frames (to 0x7000)
#define FISHMOD_SRAM_CRUMB 0x7000     // 'FBCR': where the bench was, as each mode starts (a crash says where)
#define FISHMOD_SRAM_LOD_ROTATE 0x7640 // the LOD rotation check's results

// Past the saves
#define FISHMOD_SRAM_HASHES 0x7A00    // 'FHSH': the state fingerprints (every script, 0x40 bytes)
#define FISHMOD_SRAM_BIG 0x7A40       // the LOD calibration's results / the play log ('FPLG', to 0x7FC0): these own
                                      // 0x7A40 up, and leave out the records below that would land in them
#define FISHMOD_SRAM_STALL 0x7B00     // 'FSTL': a stall (no frame for 10 s)
#define FISHMOD_SRAM_FAULT 0x7C00     // 'FFLT': a crash; also the LOD jobs' part number on input ('FMPT')
#define FISHMOD_SRAM_COLLISION 0x7D00 // 'FMCO': collision use per scene
#define FISHMOD_SRAM_PROFILE 0x7E00   // the fish profile timers (not with the play log)
#define FISHMOD_SRAM_STATS 0x7F00     // 'FMST': whole-run averages (not with the play log)

#define FISHMOD_SRAM_ADDR(offset) (OS_K1_TO_PHYSICAL(0xA8000000) + (offset))

#endif
