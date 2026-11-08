// Part of fishmod_debug.c (#included there, sharing its statics): the PC-sampling profiler (FISHMOD_PCPROF).
/*
 * fish-oot's glue for n64prof (n64prof/, a game-independent sampler: see its README.md): libultra's timer interrupt
 * calls a hook (src/libultra/os/timerintr.c, debug builds) that gives the sampler the interrupted thread's PC; once a
 * frame the overlay tables say where each overlay is; every 30 seconds the hottest buckets go to SRAM (over the save
 * files: blank-save ROMs only). Read with n64prof/tools/n64prof.py (fishmod/tools/pcprof.sh finds the dump in a save).
 *
 * It runs the same on the console and in ares (the timer is the CPU's own Count/Compare), so the two can be compared.
 */
#ifdef FISHMOD_PCPROF
#include "z_game_dlftbls.h"
#include "kaleido_manager.h"
#include "effect.h"
#include "ultra64/osint.h"
#define N64PROF_BUCKET_SHIFT 5 // (32-byte buckets: pcprof1 lost 12.8% of its samples to the save's room)
#include "n64prof/n64prof.c"

#define PCPROF_INTERVAL 11719 // count cycles between samples on average (46.875 MHz: about 4000 a second)
#define PCPROF_HASH 8192      // buckets in RAM (a power of two)
#define PCPROF_SRAM_SIZE (FISHMOD_SRAM_CRUMB - FISHMOD_SRAM_PCPROF)
#define PCPROF_SAVE_SECONDS 30

extern void (*gFishmodTimerHook)(OSTimer* timer);
extern u32 gFishmodCounterPc; // where the counter interrupt came in (exceptasm.s)

static N64ProfEntry* sPcTable;
static u8* sPcOut;
static OSTimer sPcTimer;

/**
 * The timer hook: one sample. libultra runs its timers in the VI manager's thread after the counter interrupt has
 * posted a message, so the PC is the one the exception handler saw (exceptasm.s keeps it), not the running thread's.
 */
static void PcProf_Hook(OSTimer* timer) {
    if (timer == &sPcTimer) {
        n64prof_sample(gFishmodCounterPc);
        timer->interval = n64prof_next_interval();
    }
}

/** Where each loaded overlay is (once a frame, from the game thread). */
static void PcProf_Ranges(void) {
    N64ProfRange* r = n64prof_ranges_begin();
    s32 n = 0;
    s32 i;

#define PCPROF_ADD(ram, vs, ve)                                                          \
    if (((ram) != NULL) && ((vs) != NULL) && (n < N64PROF_MAX_RANGES)) {                  \
        r[n].ramStart = (u32)(ram);                                                       \
        r[n].ramEnd = (u32)(ram) + ((u32)(ve) - (u32)(vs));                               \
        r[n].vramStart = (u32)(vs);                                                       \
        n++;                                                                              \
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
    n64prof_ranges_end(n);
}

/** Once a play frame: start the sampler, keep the overlay ranges current, save every 30 seconds. */
static void PcProf_Update(void) {
    static OSTime sLastSave;
    OSTime now = osGetTime();

    if (sPcTable == NULL) {
        sPcTable = SYSTEM_ARENA_MALLOC(PCPROF_HASH * sizeof(N64ProfEntry), "../fishmod_debug.c", __LINE__);
        sPcOut = SYSTEM_ARENA_MALLOC(PCPROF_SRAM_SIZE, "../fishmod_debug.c", __LINE__);
        if ((sPcTable == NULL) || (sPcOut == NULL)) {
            sPcTable = NULL;
            Fishmod_DebugPrintf(5, "PCPROF: no memory");
            return;
        }
        n64prof_init(sPcTable, PCPROF_HASH, PCPROF_INTERVAL, (u32)_codeSegmentEnd);
        PcProf_Ranges();
        gFishmodTimerHook = PcProf_Hook;
        osSetTimer(&sPcTimer, PCPROF_INTERVAL, PCPROF_INTERVAL, NULL, NULL);
        sLastSave = now;
        return;
    }
    // (every 8 frames: every frame cost 0.8% in pcprof1; an overlay loaded since is unmapped for a few frames)
    if ((sPlayFrames & 7) == 0) {
        PcProf_Ranges();
    }
    if (OS_CYCLES_TO_USEC(now - sLastSave) > PCPROF_SAVE_SECONDS * 1000000ULL) {
        u32 user[6] = { 0x464D4F44 /* 'FMOD' */, 0, 0, 0, 0, 0 };
        u32 bytes;

        user[1] = sPlayFrames;
        user[2] = FISHMOD_PROFILE_HW;
        sLastSave = now;
        bytes = n64prof_serialize(sPcOut, PCPROF_SRAM_SIZE, user);
        if (bytes != 0) {
            N64ProfHeader* h = (N64ProfHeader*)sPcOut;

            SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_PCPROF), sPcOut, bytes, OS_WRITE);
            // (once a save: the debug text's formatting every frame cost about 2% in pcprof1)
            Fishmod_DebugPrintf(5, "pcprof dump %u: %u samples, %u buckets", h->dumps, h->total, h->entries);
        }
    }
}
#endif
