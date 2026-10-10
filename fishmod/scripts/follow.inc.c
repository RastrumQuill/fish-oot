// Follow camera: chases the fish nearest to Link to check swimming.
#define FISHMOD_VIEW 1
#define FISHMOD_VIEW_FOLLOW 1

static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};

static Vec3f sViewEye = { 1150.0f, 140.0f, 260.0f };
static Vec3f sViewAt = { 1210.0f, -40.0f, 40.0f };
