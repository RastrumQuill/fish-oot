// Photo booth: renders every Kokiri species for the wiki (see fishmod/tools/photo_capture.py).
#define FISHMOD_PHOTO 1

static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};

// Fish float high above the test spot so nothing else is in frame.
static Vec3f sPhotoPos = { 1033.0f, 900.0f, 172.0f };
static s16 sPhotoYaw = 0x4000;
