#include "sys_cfb.h"

#include "libu64/debug.h"
#include "attributes.h"
#include "gfx.h"
#include "line_numbers.h"
#include "printf.h"
#include "translation.h"

#pragma increment_block_number "gc-eu:128 gc-eu-mq:128 gc-jp:128 gc-jp-ce:128 gc-jp-mq:128 gc-us:128 gc-us-mq:128" \
                               "pal-1.1:128"

uintptr_t sSysCfbFbPtr[3];
s32 sSysCfbCount = 2; // fish-oot: three with the Expansion Pak (the CPU and RCP a frame ahead when they can)
uintptr_t sSysCfbEnd;

u32 gFishmodSampleStoreStart; // fish-oot: RAM no one else uses, for the audio driver's sample store (0: none)
u32 gFishmodSampleStoreEnd;

void SysCfb_Init(s32 n64dd) {
    u32 screenSize;
    UNUSED_NDEBUG uintptr_t tmpFbEnd;

    if (osMemSize >= 0x800000) {
        PRINTF(T("８Ｍバイト以上のメモリが搭載されています\n", "8MB or more memory is installed\n"));
        tmpFbEnd = 0x8044BE80;
        if (n64dd == 1) {
            PRINTF(T("RAM 8M mode (N64DD対応)\n", "RAM 8M mode (N64DD compatible)\n"));
#if DEBUG_FEATURES
            sSysCfbEnd = 0x805FB000;
#else
            sSysCfbEnd = 0x80600000;
#endif
        } else {
            PRINTF(T("このバージョンのマージンは %dK バイトです\n", "The margin for this version is %dK bytes\n"),
                   (0x4BC00 / 1024));
#if DEBUG_FEATURES
            sSysCfbEnd = tmpFbEnd;
#else
            // fishmod: frame buffers at the top of the Expansion Pak, so the 4 MB below go to the system heap
            sSysCfbEnd = 0x80800000;
#endif
        }
    } else if (osMemSize >= 0x400000) {
        PRINTF("RAM4M mode\n");
        sSysCfbEnd = 0x80400000;
    } else {
        LogUtils_HungupThread("../sys_cfb.c", LN4(305, 308, 322, 341, 354));
    }

    screenSize = SCREEN_WIDTH * SCREEN_HEIGHT;
    sSysCfbEnd &= ~0x3F;

    if (1) {}

    PRINTF(T("システムが使用する最終アドレスは %08x です\n", "The final address used by the system is %08x\n"),
           sSysCfbEnd);
    sSysCfbFbPtr[0] = sSysCfbEnd - (screenSize * 4);
    sSysCfbFbPtr[1] = sSysCfbEnd - (screenSize * 2);
#if !DEBUG_FEATURES
    if (sSysCfbEnd == 0x80800000) {
        // fishmod: with the Expansion Pak, the two frame buffers in different 1 MB banks of RDRAM (the tops of the last
        // two), so the RDP drawing one and the video interface reading out the other don't keep closing each other's
        // open rows in the same bank. (The Z buffer is in the base RAM, another bank again.) The system heap, which
        // ends at the lower frame buffer, gives up the 870 KB between them.
        sSysCfbFbPtr[0] = 0x80700000 - (screenSize * 2);
        sSysCfbFbPtr[1] = 0x80800000 - (screenSize * 2);
        // a third, just below the second: with three, a frame can be drawn while the last waits its turn on screen, so
        // the CPU and the RCP can get a frame ahead, and a fast frame's spare time goes to a slow one after it instead of
        // to waiting (graph.c); the game runs the same frames either way
        sSysCfbFbPtr[2] = sSysCfbFbPtr[1] - (screenSize * 2);
#ifndef FISHMOD_NO_RUNAHEAD // (console tests: one new mechanism left out at a time, -D in GCC_EXTRA_OPT)
        sSysCfbCount = 3;
#endif
#ifndef FISHMOD_NO_STORE
        // the space between them goes to the audio driver: copies of the samples being played (audio/internal/load.c)
        gFishmodSampleStoreStart = 0x80700000;
        gFishmodSampleStoreEnd = sSysCfbFbPtr[2];
#endif
    }
#endif
    PRINTF(T("フレームバッファのアドレスは %08x と %08x です\n", "Frame buffer addresses are %08x and %08x\n"),
           sSysCfbFbPtr[0], sSysCfbFbPtr[1]);
}

void SysCfb_Reset(void) {
    sSysCfbFbPtr[0] = 0;
    sSysCfbFbPtr[1] = 0;
    sSysCfbFbPtr[2] = 0;
    sSysCfbCount = 2;
    sSysCfbEnd = 0;
}

s32 SysCfb_GetFbCount(void) {
    return sSysCfbCount;
}

void* SysCfb_GetFbPtr(s32 idx) {
    if (idx < sSysCfbCount) {
        return (void*)sSysCfbFbPtr[idx];
    }
    return NULL;
}

void* SysCfb_GetFbEnd(void) {
    return (void*)sSysCfbEnd;
}
