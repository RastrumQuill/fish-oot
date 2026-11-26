// A cloudy day's arrival at Lake Hylia (day 1 rolls overcast): the sky in its first frames, for the noise a sky loaded
// clear and then streamed over showed (Fishmod_WeatherToday: the skybox now loads the clouds from the start).
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1
static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 90, 0, 0, 0, true },
};
