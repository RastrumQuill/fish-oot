/*
 * fish-oot: the last object of the audio driver's in the code segment (spec), holding one aligned item in each of
 * its sections, as code/fishmod_pin.c does for the mod's own code. The driver's code and data come to different
 * sizes from build to build (with and without --expak, most of all: six command lists and output buffers instead
 * of two and three, another retrace handler); without this everything of the engine's after them would move with
 * them to other lines of the CPU's caches. That was measured: the build with --expak ran a seventh slower in the
 * emulator, before any of what it adds was switched on, the thread switch (__osDispatchThread) alone taking three
 * times the data cache fills, its thread records fallen on lines already busy. With each of the driver's sections
 * ending on a fixed boundary (16 KB for code, 8 KB for data), what follows starts at the same place in the caches.
 */
#include "ultra64.h"

#ifdef __GNUC__ // (the build whose audio is IDO's doesn't pin it: its driver is the game's, the size it always was)
__attribute__((used, noinline, aligned(0x4000))) void Fishmod_AudioPinText(void) {
}

__attribute__((used, aligned(0x2000))) s32 gFishmodAudioPinData = 1;

__attribute__((used, aligned(0x2000))) const s32 gFishmodAudioPinRodata = 1;

__attribute__((used, aligned(0x2000))) s32 gFishmodAudioPinBss;
#endif
