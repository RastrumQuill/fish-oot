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
