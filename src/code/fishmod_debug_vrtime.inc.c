// Part of fishmod_debug.c (#included there, sharing its statics): the VR4300 timing tests (FISHMOD_VRTIME).
/*
 * fish-oot's glue for vrtime (n64prof/vrtime/: game-independent timing tests for the N64's CPU, for a compiler's cost
 * model). A second after the game starts it runs every test (a few seconds, the picture stopping meanwhile: each test
 * has interrupts off), saves the results over the save files (blank-save ROMs only) and says so on screen. Read with
 * n64prof/tools/vrtime.py SAVE.
 */
#ifdef FISHMOD_VRTIME
#include "alignment.h"
#include "n64prof/vrtime/vrtime.c"

static u32 Vrtime_IrqOff(void) {
    return (u32)__osDisableInt();
}

static void Vrtime_IrqOn(u32 mask) {
    __osRestoreInt((s32)mask);
}

static void Vrtime_Update(void) {
    static s32 sDone;
    static s32 sCount;

    if (!sDone && (sPlayFrames > 20)) {
        static struct {
            VrtimeHeader header;
            VrtimeResult results[VRTIME_MAX_TESTS];
        } sOut;
        void* raw = SYSTEM_ARENA_MALLOC(0x10000 + 16, "../fishmod_debug.c", __LINE__);

        sDone = true;
        if (raw == NULL) {
            sCount = -1;
            return;
        }
        sCount = vrtime_run(sOut.results, VRTIME_MAX_TESTS, (void*)ALIGN16((u32)raw), Vrtime_IrqOff, Vrtime_IrqOn);
        bzero(&sOut.header, sizeof(sOut.header));
        sOut.header.magic = VRTIME_MAGIC;
        sOut.header.count = sCount;
        sOut.header.runs = RUNS;
        SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_BENCH), &sOut,
                         ALIGN16(sizeof(VrtimeHeader) + sCount * sizeof(VrtimeResult)), OS_WRITE);
    }
    if (sDone) {
        Fishmod_DebugPrintf(5, (sCount < 0) ? "vrtime: no memory" : "vrtime: %d tests done and saved", sCount);
    }
}
#endif
