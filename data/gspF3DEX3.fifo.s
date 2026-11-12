.include "macro.inc"

/* F3DEX3 (BrW: OoT's Z-buffer and tri setup), patched from the ROM's F3DZEX2 at build time: see fishmod/f3dex3.
   With the occlusion plane where the build profile asks for it (F3DEX3_OCCLUSION), else the NOC variant without; or one
   of its profiling builds (F3DEX3_PROF 1-3: PA, PB, PC, with the occlusion plane), for its counters. */

.section .rodata

.balign 16

glabel gspF3DEX3_fifoTextStart
#if defined(F3DEX3_PROF) && F3DEX3_PROF == 1
    .incbin "incbin/F3DEX3_BrW_PA_Text"
#elif defined(F3DEX3_PROF) && F3DEX3_PROF == 2
    .incbin "incbin/F3DEX3_BrW_PB_Text"
#elif defined(F3DEX3_PROF) && F3DEX3_PROF == 3
    .incbin "incbin/F3DEX3_BrW_PC_Text"
#elif F3DEX3_OCCLUSION
    .incbin "incbin/F3DEX3_BrW_Text"
#else
    .incbin "incbin/F3DEX3_BrW_NOC_Text"
#endif
glabel gspF3DEX3_fifoTextEnd

.balign 16

glabel gspF3DEX3_fifoDataStart
#if defined(F3DEX3_PROF) && F3DEX3_PROF == 1
    .incbin "incbin/F3DEX3_BrW_PA_Data"
#elif defined(F3DEX3_PROF) && F3DEX3_PROF == 2
    .incbin "incbin/F3DEX3_BrW_PB_Data"
#elif defined(F3DEX3_PROF) && F3DEX3_PROF == 3
    .incbin "incbin/F3DEX3_BrW_PC_Data"
#elif F3DEX3_OCCLUSION
    .incbin "incbin/F3DEX3_BrW_Data"
#else
    .incbin "incbin/F3DEX3_BrW_NOC_Data"
#endif
glabel gspF3DEX3_fifoDataEnd
