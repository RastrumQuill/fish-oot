// Pop-in check: adult Link in the iron boots on the lake's bed off the pier, a few steps toward the shore and then
// still. Under water the plants beyond him (400 to 900 units from the camera) are drawn as their far single cards:
// the bushes and grass ahead should show as dark shapes in the haze, as they do with the cards off
// (#define FISHMOD_SCRIPT_BENCH FISHMOD_BENCH_NOIMP). Before impostor.py's fix they didn't (the cards were a few
// units across). The scripted walk presses nothing, so the camera stays where the scene put it: for the view from
// behind him, Z must be tapped (as the check's own build did).
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_TIME 0x8000
#define FISHMOD_SCRIPT_NOTEXT 1
static Vec3f sScriptStartPos = { -250.0f, -1500.0f, 4480.0f };
static s16 sScriptStartYaw = 0x8000;
#define FISHMOD_SCRIPT_WALK_TO { -250.0f, 0.0f, 4330.0f }
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};
