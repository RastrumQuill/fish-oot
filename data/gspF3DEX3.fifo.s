.include "macro.inc"

/* F3DEX3 (BrW: OoT's Z-buffer and tri setup), patched from the ROM's F3DZEX2 at build time: see fishmod/f3dex3.
   With the occlusion plane where the build profile asks for it (F3DEX3_OCCLUSION), else the NOC variant without. */

.section .rodata

.balign 16

glabel gspF3DEX3_fifoTextStart
#if F3DEX3_OCCLUSION
    .incbin "incbin/F3DEX3_BrW_Text"
#else
    .incbin "incbin/F3DEX3_BrW_NOC_Text"
#endif
glabel gspF3DEX3_fifoTextEnd

.balign 16

glabel gspF3DEX3_fifoDataStart
#if F3DEX3_OCCLUSION
    .incbin "incbin/F3DEX3_BrW_Data"
#else
    .incbin "incbin/F3DEX3_BrW_NOC_Data"
#endif
glabel gspF3DEX3_fifoDataEnd
