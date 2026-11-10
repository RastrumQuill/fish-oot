#ifndef VBENCH_H
#define VBENCH_H
/*
 * The vanilla bench (vanilla/README.md): the unmodified game, with only what it takes to play fish-oot's play-bench
 * route at Lake Hylia by itself and time it the way the mod's build does, streamed over a SummerCart64's USB.
 */
#include "ultra64.h"

struct GameState;
struct PlayState;
struct Input;

// Play_Update's sections, in fish-oot's order (FishmodSect, include/fishmod.h): the frame records line up with the
// mod's (the first stays 0: the mod's own per-frame work; the second is this bench's)
typedef enum VBenchSect {
    VB_SECT_FM_PLAY,
    VB_SECT_FM_DEBUG,
    VB_SECT_OBJECTS,
    VB_SECT_ROOMREQ,
    VB_SECT_COL_AT,
    VB_SECT_COL_OC,
    VB_SECT_COL_DAMAGE,
    VB_SECT_COL_CLEAR,
    VB_SECT_ACTORS,
    VB_SECT_DYNAPOLY,
    VB_SECT_CUTSCENE,
    VB_SECT_EFFECTS,
    VB_SECT_ROOMFX,
    VB_SECT_SKYBOX,
    VB_SECT_MESSAGE,
    VB_SECT_INTERFACE,
    VB_SECT_ANIMTASK,
    VB_SECT_SFXSRC,
    VB_SECT_CAMERA,
    VB_SECT_ENV,
    VB_SECT_MAX
} VBenchSect;

#define VB_ACTOR_IDS 480
#define VB_ACTOR_CATS 12

extern OSTime gVBenchSect[VB_SECT_MAX];
extern OSTime gVBenchCpuTime[2]; // Play_Update, Play_Draw
extern u32 gVBenchActorCycles[VB_ACTOR_IDS];
extern u32 gVBenchCatCycles[VB_ACTOR_CATS];
extern u32 gVBenchCounterPc;             // exceptasm.s: where the counter interrupt came in
extern void (*gVBenchTimerHook)(OSTimer* timer); // timerintr.c: each timer as it fires

#define VB_SECT(slot, stmt)                             \
    {                                                   \
        OSTime vbStart_ = osGetTime();                  \
        stmt;                                           \
        gVBenchSect[slot] += osGetTime() - vbStart_;    \
    }

void VBench_Boot(void);
void VBench_Update(struct PlayState* play);
void VBench_Input(struct Input* input);
void VBench_DrawText(Gfx** gfxP);
#endif
