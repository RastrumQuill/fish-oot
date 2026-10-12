// Animation studio: every Kokiri species swimming in place on a plain backdrop, for recording
// (fishmod/tools/anim_capture.py). Needs FISHMOD_OPT 4 for the per-fish rig choice.
#define FISHMOD_ANIM_STUDIO 1

static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
