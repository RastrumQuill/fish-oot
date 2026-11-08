// Part of fishmod_debug.c (#included there, sharing its statics): the PC-sampling profiler (FISHMOD_PCPROF).
/*
 * Where the CPU's time goes, for a compiler's sample profile (fishmod/docs/hw-rig.md, "Profiling on the console").
 * A timer interrupt every PCPROF_INTERVAL count cycles (jittered by a quarter either way, so work that repeats with
 * the frame isn't always hit or always missed) records the PC the running thread was interrupted at: time spent
 * waiting on cache misses and RDRAM lands on the instruction that waited. A PC inside a loaded overlay is turned into
 * its link-time address (the overlay tables say where each one is), so every sample matches the ELF. Samples are
 * counted per 16 bytes of code in a hash table in RAM; the hottest buckets go to SRAM every 30 seconds (over the
 * save files: blank-save ROMs only). Read with fishmod/tools/pcprof.py.
 *
 * It runs the same on the console and in ares (the timer is the CPU's own Count/Compare), so the two can be compared.
 */
#ifdef FISHMOD_PCPROF
#include "z_game_dlftbls.h"
#include "kaleido_manager.h"
#include "effect.h"
#include "ultra64/osint.h"
#include "alignment.h"

#define PCPROF_VERSION 1
#define PCPROF_INTERVAL 11719 // count cycles between samples on average (46.875 MHz: about 4000 a second)
#define PCPROF_HASH 8192      // buckets in RAM (a power of two)
#define PCPROF_PROBES 16
#define PCPROF_RANGES 96 // loaded overlays the interrupt can map
#define PCPROF_SRAM_SIZE (FISHMOD_SRAM_CRUMB - FISHMOD_SRAM_PCPROF)
#define PCPROF_SAVE_SECONDS 30

typedef struct PcProfEntry {
    u32 addr;  // the bucket's first address: link-time for code in an overlay, else the PC as it was
    u32 count; // samples in it
} PcProfEntry;

typedef struct PcProfHeader {
    /* 0x00 */ u32 magic; // 'FPCP'
    /* 0x04 */ u32 version;
    /* 0x08 */ u32 total;    // samples taken
    /* 0x0C */ u32 dropped;  // samples not counted: the table was full
    /* 0x10 */ u32 unsaved;  // samples counted in buckets too cold to fit in the save
    /* 0x14 */ u32 entries;  // entries that follow
    /* 0x18 */ u32 interval; // count cycles between samples, on average
    /* 0x1C */ u32 frames;   // play frames when saved
    /* 0x20 */ u32 saves;    // how many times it has been saved
    /* 0x24 */ u32 hw;       // the hardware profile's build
    /* 0x28 */ u32 unmapped; // samples outside the static code and the loaded overlays (counted at their raw PC)
    /* 0x2C */ u32 pad[5];
} PcProfHeader; // size = 0x40

#define PCPROF_SAVE_MAX ((PCPROF_SRAM_SIZE - sizeof(PcProfHeader)) / sizeof(PcProfEntry))

typedef struct PcProfRange {
    u32 ramStart;
    u32 ramEnd;
    u32 vramStart;
} PcProfRange;

extern void (*gFishmodTimerHook)(OSTimer* timer);
extern char _codeSegmentEnd[];

static PcProfEntry* sPcTable;
static u8* sPcOut;
static OSTimer sPcTimer;
static u32 sPcRand = 0x2545F491;
static volatile u32 sPcTotal;
static volatile u32 sPcDropped;
static volatile u32 sPcUnmapped;
// two sets of the loaded overlays' ranges: the game thread fills one while the interrupt reads the other
static PcProfRange sPcRanges[2][PCPROF_RANGES];
static s32 sPcRangeCount[2];
static volatile s32 sPcRangeSet;

/** The timer hook (interrupts off, the interrupted thread's registers saved): one sample. Integer only. */
static void PcProf_Hook(OSTimer* timer) {
    PcProfRange* r;
    PcProfEntry* e;
    u32 pc;
    u32 key;
    u32 h;
    s32 n;
    s32 i;

    if ((timer != &sPcTimer) || (sPcTable == NULL)) {
        return;
    }
    // the next one a quarter early to a quarter late
    sPcRand ^= sPcRand << 13;
    sPcRand ^= sPcRand >> 17;
    sPcRand ^= sPcRand << 5;
    timer->interval = (PCPROF_INTERVAL * 3 / 4) + (sPcRand % (PCPROF_INTERVAL / 2));

    sPcTotal++;
    pc = __osRunningThread->context.pc;
    r = sPcRanges[sPcRangeSet];
    n = sPcRangeCount[sPcRangeSet];
    for (i = 0; i < n; i++, r++) {
        if ((pc - r->ramStart) < (r->ramEnd - r->ramStart)) {
            pc = pc - r->ramStart + r->vramStart;
            break;
        }
    }
    if ((i == n) && (pc >= (u32)_codeSegmentEnd)) {
        sPcUnmapped++;
    }
    key = pc & ~0xF;
    h = ((key >> 4) * 2654435761u) >> (32 - 13);
    for (i = 0; i < PCPROF_PROBES; i++) {
        e = &sPcTable[(h + i) & (PCPROF_HASH - 1)];
        if (e->addr == key) {
            e->count++;
            return;
        }
        if (e->addr == 0) {
            e->addr = key;
            e->count = 1;
            return;
        }
    }
    sPcDropped++;
}

/** Where each loaded overlay is, for the interrupt (once a frame, from the game thread). */
static void PcProf_Ranges(void) {
    s32 set = sPcRangeSet ^ 1;
    PcProfRange* r = sPcRanges[set];
    s32 n = 0;
    s32 i;

#define PCPROF_ADD(ram, vs, ve)                                                  \
    if (((ram) != NULL) && ((vs) != NULL) && (n < PCPROF_RANGES)) {               \
        r[n].ramStart = (u32)(ram);                                               \
        r[n].ramEnd = (u32)(ram) + ((u32)(ve) - (u32)(vs));                       \
        r[n].vramStart = (u32)(vs);                                               \
        n++;                                                                      \
    }
    for (i = 0; i < ACTOR_ID_MAX; i++) {
        PCPROF_ADD(gActorOverlayTable[i].loadedRamAddr, gActorOverlayTable[i].vramStart,
                   gActorOverlayTable[i].vramEnd);
    }
    for (i = 0; i < EFFECT_SS_TYPE_MAX; i++) {
        PCPROF_ADD(gEffectSsOverlayTable[i].loadedRamAddr, gEffectSsOverlayTable[i].vramStart,
                   gEffectSsOverlayTable[i].vramEnd);
    }
    for (i = 0; i < GAMESTATE_ID_MAX; i++) {
        PCPROF_ADD(gGameStateOverlayTable[i].loadedRamAddr, gGameStateOverlayTable[i].vramStart,
                   gGameStateOverlayTable[i].vramEnd);
    }
    for (i = 0; i < KALEIDO_OVL_MAX; i++) {
        PCPROF_ADD(gKaleidoMgrOverlayTable[i].loadedRamAddr, gKaleidoMgrOverlayTable[i].vramStart,
                   gKaleidoMgrOverlayTable[i].vramEnd);
    }
#undef PCPROF_ADD
    sPcRangeCount[set] = n;
    sPcRangeSet = set; // (the interrupt can't run in the middle of this store)
}

/** The hottest buckets to SRAM: as many as fit, hottest first by powers of two. */
static void PcProf_Save(void) {
    PcProfHeader* hdr = (PcProfHeader*)sPcOut;
    PcProfEntry* out = (PcProfEntry*)(sPcOut + sizeof(PcProfHeader));
    u32 hist[32];
    u32 saved = 0;
    u32 counted = 0;
    u32 room = PCPROF_SAVE_MAX;
    s32 floorBit;
    s32 b;
    s32 i;
    static u32 sSaves;

    bzero(hist, sizeof(hist));
    for (i = 0; i < PCPROF_HASH; i++) {
        u32 c = sPcTable[i].count;

        if (c != 0) {
            for (b = 31; !(c & (1u << b)); b--) {
            }
            hist[b]++;
        }
    }
    // the lowest power of two whose buckets (with all hotter ones) still fit; the next one down fills what's left
    for (floorBit = 31; floorBit > 0; floorBit--) {
        if (hist[floorBit] > room) {
            break;
        }
        room -= hist[floorBit];
    }
    room = PCPROF_SAVE_MAX;
    for (b = 31; (b >= floorBit) && (room > 0); b--) {
        for (i = 0; (i < PCPROF_HASH) && (room > 0); i++) {
            u32 c = sPcTable[i].count;

            if ((c >= (1u << b)) && ((b == 31) || (c < (1u << (b + 1))))) {
                out[saved] = sPcTable[i];
                saved++;
                room--;
                counted += c;
            }
        }
    }
    bzero(hdr, sizeof(PcProfHeader));
    hdr->magic = 0x46504350; // 'FPCP'
    hdr->version = PCPROF_VERSION;
    hdr->total = sPcTotal;
    hdr->dropped = sPcDropped;
    hdr->unsaved = sPcTotal - sPcDropped - counted;
    hdr->entries = saved;
    hdr->interval = PCPROF_INTERVAL;
    hdr->frames = sPlayFrames;
    hdr->saves = ++sSaves;
    hdr->hw = FISHMOD_PROFILE_HW;
    hdr->unmapped = sPcUnmapped;
    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_PCPROF), sPcOut,
                     ALIGN16(sizeof(PcProfHeader) + saved * sizeof(PcProfEntry)), OS_WRITE);
}

/** Once a play frame: start the sampler, keep the overlay ranges current, save every 30 seconds. */
static void PcProf_Update(void) {
    static OSTime sLastSave;
    OSTime now = osGetTime();

    if (sPcTable == NULL) {
        sPcTable = SYSTEM_ARENA_MALLOC(PCPROF_HASH * sizeof(PcProfEntry), "../fishmod_debug.c", __LINE__);
        sPcOut = SYSTEM_ARENA_MALLOC(PCPROF_SRAM_SIZE, "../fishmod_debug.c", __LINE__);
        if ((sPcTable == NULL) || (sPcOut == NULL)) {
            sPcTable = NULL;
            Fishmod_DebugPrintf(5, "PCPROF: no memory");
            return;
        }
        bzero(sPcTable, PCPROF_HASH * sizeof(PcProfEntry));
        PcProf_Ranges();
        PcProf_Ranges(); // (both sets)
        gFishmodTimerHook = PcProf_Hook;
        osSetTimer(&sPcTimer, PCPROF_INTERVAL, PCPROF_INTERVAL, NULL, NULL);
        sLastSave = now;
        return;
    }
    PcProf_Ranges();
    if (OS_CYCLES_TO_USEC(now - sLastSave) > PCPROF_SAVE_SECONDS * 1000000ULL) {
        sLastSave = now;
        PcProf_Save();
    }
    Fishmod_DebugPrintf(5, "pcprof %u samples, %u dropped, %u unmapped", sPcTotal, sPcDropped, sPcUnmapped);
}
#endif
