#ifndef FISHMOD_H
#define FISHMOD_H

#include "ultra64.h"
#include "libu64/pad.h"

struct GameState;
struct PlayState;
struct GfxPrint;

#ifndef FISHMOD_DEBUG
#define FISHMOD_DEBUG 0
#endif

/* Debug harness (fishmod_debug.c), only built with FISHMOD_DEBUG */

#define FISHMOD_DEBUG_LINES 8
#define FISHMOD_DEBUG_LINE_LEN 40

typedef struct FishmodAutoplayStep {
    /* 0x00 */ u16 frames;
    /* 0x02 */ u16 button;
    /* 0x04 */ s8 stickX;
    /* 0x05 */ s8 stickY;
} FishmodAutoplayStep; // size = 0x6

#if FISHMOD_DEBUG
void Fishmod_DebugBoot(void);
void Fishmod_AutoplayInput(struct GameState* gameState, Input* inputs);
void Fishmod_DebugPlayUpdate(struct PlayState* play);
void Fishmod_DebugDrawText(struct GameState* gameState, Gfx** gfxP);
void Fishmod_DebugPrintf(s32 line, const char* fmt, ...);
#endif

#endif
