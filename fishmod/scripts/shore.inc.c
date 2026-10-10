// Auto-angler standing 80 units back from the Kokiri moat, so the line has to cross the bank: checks the line
// drapes over the ground and floats on the water instead of cutting through them.
#define FISHMOD_AUTO_ANGLER 1

static Vec3f sScriptStartPos = { 900.0f, 0.0f, -100.0f };
static s16 sScriptStartYaw = 0x2000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 10, 0, 0, 0 },
};
