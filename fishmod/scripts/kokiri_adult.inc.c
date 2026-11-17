// Adult Link put down in Kokiri Forest from boot, standing still: the first frame there is the one that stopped on a
// floating-point exception (the view's look-at point read before it's set: code/fishmod.c, Fishmod_ViewSane). For
// the fix's test: with GCC_EXTRA_OPT=-DFISHMOD_TEST_VIEW_GARBAGE the look-at point is given the two words the crash
// was seen with (add -DFISHMOD_TEST_VIEW_NOFIX to see it stop without the fix).
// Build: fishmod/build.sh --script kokiri_adult --hw --texpack --colpack --meshpack --gcc-all
#define FISHMOD_SCRIPT_ADULT 1
static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 600, 0, 0, 0 },
};
