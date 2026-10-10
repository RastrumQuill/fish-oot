.include "macro.inc"

/* F3DEX3 (BrW, no occlusion planes), patched from the ROM's F3DZEX2 at build time: see fishmod/f3dex3 */

.section .rodata

.balign 16

glabel gspF3DEX3_fifoTextStart
    .incbin "incbin/F3DEX3_BrW_NOC_Text"
glabel gspF3DEX3_fifoTextEnd

.balign 16

glabel gspF3DEX3_fifoDataStart
    .incbin "incbin/F3DEX3_BrW_NOC_Data"
glabel gspF3DEX3_fifoDataEnd
