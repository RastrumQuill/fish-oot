#include "ultra64.h"
#include "sys_ucode.h"

#if ENABLE_F3DEX3
extern u64 gspF3DEX3_fifoTextStart[];
extern u64 gspF3DEX3_fifoDataStart[];

u64* sDefaultGSPUCodeText = gspF3DEX3_fifoTextStart;
u64* sDefaultGSPUCodeData = gspF3DEX3_fifoDataStart;
#elif !defined(F3DEX_GBI_PL)
u64* sDefaultGSPUCodeText = gspF3DZEX2_NoN_fifoTextStart;
u64* sDefaultGSPUCodeData = gspF3DZEX2_NoN_fifoDataStart;
#else
u64* sDefaultGSPUCodeText = gspF3DZEX2_NoN_PosLight_fifoTextStart;
u64* sDefaultGSPUCodeData = gspF3DZEX2_NoN_PosLight_fifoDataStart;
#endif

u64* SysUcode_GetUCodeBoot(void) {
    return rspbootTextStart;
}

size_t SysUcode_GetUCodeBootSize(void) {
    return (size_t)((u8*)rspbootTextEnd - (u8*)rspbootTextStart);
}

u64* SysUcode_GetUCode(void) {
    return sDefaultGSPUCodeText;
}

u64* SysUcode_GetUCodeData(void) {
    return sDefaultGSPUCodeData;
}

#if defined(F3DEX3_ALL) && ENABLE_F3DEX3
extern u64 gspF3DEX3_PA_fifoTextStart[];
extern u64 gspF3DEX3_PA_fifoDataStart[];
extern u64 gspF3DEX3_PB_fifoTextStart[];
extern u64 gspF3DEX3_PB_fifoDataStart[];
extern u64 gspF3DEX3_PC_fifoTextStart[];
extern u64 gspF3DEX3_PC_fifoDataStart[];

/**
 * fish-oot: which of F3DEX3's builds draws the frames from now on (0 the plain one, 1-3 its profiling builds A-C),
 * for the bench to measure every set of its counters in one run. Called at the start of a frame's update, so the
 * frame's display lists and its task use the same one.
 */
void Fishmod_SetUcodeSet(s32 set) {
    static u64* const sText[4] = { gspF3DEX3_fifoTextStart, gspF3DEX3_PA_fifoTextStart, gspF3DEX3_PB_fifoTextStart,
                                   gspF3DEX3_PC_fifoTextStart };
    static u64* const sData[4] = { gspF3DEX3_fifoDataStart, gspF3DEX3_PA_fifoDataStart, gspF3DEX3_PB_fifoDataStart,
                                   gspF3DEX3_PC_fifoDataStart };

    sDefaultGSPUCodeText = sText[set & 3];
    sDefaultGSPUCodeData = sData[set & 3];
}

/** Which set a task's microcode is (0-3), from its text's address. */
s32 Fishmod_UcodeSetOf(void* ucode) {
    return (ucode == gspF3DEX3_PA_fifoTextStart)   ? 1
           : (ucode == gspF3DEX3_PB_fifoTextStart) ? 2
           : (ucode == gspF3DEX3_PC_fifoTextStart) ? 3
                                                   : 0;
}
#endif
