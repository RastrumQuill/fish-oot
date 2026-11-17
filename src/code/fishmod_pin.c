/*
 * fish-oot: the last object in the code segment (spec), holding one aligned item in each of its sections. The segment
 * is laid out all its code, then all its data, then its read-only data, then its zeroed data: anything that grows the
 * mod's code would otherwise move every one of the engine's globals, tables and buffers after it to other lines of
 * the CPU's data cache (8 KB, direct mapped), and the heap after the segment with them. With each section ending on a
 * fixed boundary (16 KB for code, the instruction cache's size; 8 KB for data, the data cache's), the next section
 * starts at the same place in the caches whatever the mod's own code and data come to. (The heap's start is aligned
 * as well: main.c.) The cost: up to 40 KB of padding.
 */
#include "ultra64.h"

__attribute__((used, noinline, aligned(0x4000))) void Fishmod_PinText(void) {
}

__attribute__((used, aligned(0x2000))) s32 gFishmodPinData = 1;

__attribute__((used, aligned(0x2000))) const s32 gFishmodPinRodata = 1;

__attribute__((used, aligned(0x2000))) s32 gFishmodPinBss;
