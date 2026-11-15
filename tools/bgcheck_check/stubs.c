/*
 * What the collision code (z_bgcheck.c, sys_math3d.c and the few game files linked with them) needs from the rest of
 * the game, for bgcheck_check on a PC. Memory comes from the C library, zeroed (the same in both builds compared).
 */
#include <stdlib.h>
#include <string.h>
#include "ultra64.h"
#include "actor.h"
#include "regs.h"
#include "segmented_address.h"
#include "libc64/malloc.h"
#include "tha.h"

static RegEditor sRegEditor;
RegEditor* gRegEditor = &sRegEditor;
uintptr_t gSegments[NUM_SEGMENTS];

Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale = 1.0f;
s8 gSfxDefaultReverb;

static void* Stub_Alloc(size_t size) {
    void* p = NULL;

    if (posix_memalign(&p, 16, size + 16) != 0) {
        abort();
    }
    memset(p, 0, size + 16);
    return p;
}

void* THA_AllocTailAlign16(TwoHeadArena* tha, size_t size) {
    return Stub_Alloc(size);
}

void* THA_AllocTailAlign(TwoHeadArena* tha, size_t size, uintptr_t mask) {
    return Stub_Alloc(size);
}

void* SystemArena_Malloc(u32 size) {
    return Stub_Alloc(size);
}

void SystemArena_Free(void* ptr) {
    free(ptr);
}

void Actor_SetObjectDependency(struct PlayState* play, Actor* actor) {
}

void DynaPolyActor_UnsetAllInteractFlags(DynaPolyActor* dynaActor) {
    dynaActor->interactFlags = 0;
}

void LogUtils_HungupThread(const char* name, int line) {
    abort();
}

void Audio_PlaySfxGeneral(u16 sfxId, Vec3f* pos, u8 token, f32* freqScale, f32* vol, s8* reverbAdd) {
}

/* the console's memory (driver --ram 4 or 8), and the Expansion Pak's memory a build may use for its collision
   tables (z_bgcheck.c's BGCHECK_TABLES_RAM, pointed here by tables_ram.h): the frame buffers end below it */
u32 osMemSize = 0x800000;
u8 gBgCheckCheckRam[0x100000] __attribute__((aligned(16)));

void* SysCfb_GetFbEnd(void) {
    return gBgCheckCheckRam;
}
