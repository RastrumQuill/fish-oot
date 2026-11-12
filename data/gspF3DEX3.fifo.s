.include "macro.inc"

/* F3DEX3 (BrW: OoT's Z-buffer and tri setup), patched from the ROM's F3DZEX2 at build time: see fishmod/f3dex3.
   With the occlusion plane where the build profile asks for it (F3DEX3_OCCLUSION), else the NOC variant without; or one
   of its profiling builds (F3DEX3_PROF 1-3: PA, PB, PC, with the occlusion plane), for its counters. */

.section .rodata

/* the profiling builds: the occlusion plane's variant (BrW_Px) where the profile has it, else BrW_NOC_Px, as the plain
   one, so they measure the microcode the game runs */
#if F3DEX3_OCCLUSION
#define INC_PA_Text .incbin "incbin/F3DEX3_BrW_PA_Text"
#define INC_PA_Data .incbin "incbin/F3DEX3_BrW_PA_Data"
#define INC_PB_Text .incbin "incbin/F3DEX3_BrW_PB_Text"
#define INC_PB_Data .incbin "incbin/F3DEX3_BrW_PB_Data"
#define INC_PC_Text .incbin "incbin/F3DEX3_BrW_PC_Text"
#define INC_PC_Data .incbin "incbin/F3DEX3_BrW_PC_Data"
#else
#define INC_PA_Text .incbin "incbin/F3DEX3_BrW_NOC_PA_Text"
#define INC_PA_Data .incbin "incbin/F3DEX3_BrW_NOC_PA_Data"
#define INC_PB_Text .incbin "incbin/F3DEX3_BrW_NOC_PB_Text"
#define INC_PB_Data .incbin "incbin/F3DEX3_BrW_NOC_PB_Data"
#define INC_PC_Text .incbin "incbin/F3DEX3_BrW_NOC_PC_Text"
#define INC_PC_Data .incbin "incbin/F3DEX3_BrW_NOC_PC_Data"
#endif

.balign 16

glabel gspF3DEX3_fifoTextStart
#if defined(F3DEX3_PROF) && F3DEX3_PROF == 1
    INC_PA_Text
#elif defined(F3DEX3_PROF) && F3DEX3_PROF == 2
    INC_PB_Text
#elif defined(F3DEX3_PROF) && F3DEX3_PROF == 3
    INC_PC_Text
#elif F3DEX3_OCCLUSION
    .incbin "incbin/F3DEX3_BrW_Text"
#else
    .incbin "incbin/F3DEX3_BrW_NOC_Text"
#endif
glabel gspF3DEX3_fifoTextEnd

.balign 16

glabel gspF3DEX3_fifoDataStart
#if defined(F3DEX3_PROF) && F3DEX3_PROF == 1
    INC_PA_Data
#elif defined(F3DEX3_PROF) && F3DEX3_PROF == 2
    INC_PB_Data
#elif defined(F3DEX3_PROF) && F3DEX3_PROF == 3
    INC_PC_Data
#elif F3DEX3_OCCLUSION
    .incbin "incbin/F3DEX3_BrW_Data"
#else
    .incbin "incbin/F3DEX3_BrW_NOC_Data"
#endif
glabel gspF3DEX3_fifoDataEnd

#ifdef F3DEX3_ALL
/* fish-oot: F3DEX3's three profiling builds too, for the bench to switch to (sys_ucode.c, Fishmod_SetUcodeSet) */
.balign 16
glabel gspF3DEX3_PA_fifoTextStart
    INC_PA_Text
.balign 16
glabel gspF3DEX3_PA_fifoDataStart
    INC_PA_Data
.balign 16
glabel gspF3DEX3_PB_fifoTextStart
    INC_PB_Text
.balign 16
glabel gspF3DEX3_PB_fifoDataStart
    INC_PB_Data
.balign 16
glabel gspF3DEX3_PC_fifoTextStart
    INC_PC_Text
.balign 16
glabel gspF3DEX3_PC_fifoDataStart
    INC_PC_Data
#endif
