/*
 * fish-oot: the last object of the audio driver's in the code segment (spec). After it each section of the segment
 * goes on at a fixed place in the CPU's caches (tools/mkspecrules.c, fishmod/data/audio_pin.txt: where it went on
 * before the audio work), so the driver's code and data, which come to different sizes from build to build (with
 * and without --expak most of all), never move what follows them to other cache lines. That was measured: a build
 * with --expak ran a seventh slower in the emulator before any of what it adds was switched on, the thread switch
 * alone taking three times the data cache fills, its thread records fallen on lines already busy; and scenes where
 * an actor's code now collided with sinf's cost 8% more outside the driver. The object holds one item in each
 * section (the link pads only where it finds them: nothing in a build whose driver isn't GCC's), and the scheduler's
 * one variable for the driver, kept out of the scheduler's own data so that stays where it was too.
 */
#include "ultra64.h"

#ifdef __GNUC__ // (the build whose audio is IDO's doesn't pin it: its driver is the game's, the size it always was)
__attribute__((used, noinline)) void Fishmod_AudioPinText(void) {
}

__attribute__((used)) s32 gFishmodAudioPinData = 1;

__attribute__((used)) const s32 gFishmodAudioPinRodata = 1;

__attribute__((used)) s32 gFishmodAudioPinBss;

struct OSScTask;
struct OSScTask* volatile gFishmodSchedUrgent; // set by the audio manager, taken by Sched_HandleNotification (sched.c)
#endif
