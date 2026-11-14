// The animation check on the Tektite (blue, Lake Hylia's): spawned at the lake, held still, posed through every frame of
// object_tite's 8 animations for its skeleton (fishmod_debug_animcheck.inc.c, FISHMOD_ANIMCHECK_ACTOR), its drawing
// hashed. Before and after a change to its skeleton or its lists: fishmod/tools/animcheck_compare.py.
// Build: fishmod/build.sh --script animcheck_object_tite --hw --colpack --gcc-all   (animcheck_object_tite_pix: pictures)
#define FISHMOD_ANIMCHECK_ACTOR ACTOR_EN_TITE
#define FISHMOD_ANIMCHECK_ACTOR_PARAMS TEKTITE_BLUE
#define FISHMOD_ANIMCHECK_ACTOR_TABLE "src/fishmod_data/fishmod_animcheck_object_tite.h"
#define FISHMOD_ANIMCHECK_CAM_HEIGHT 10.0f
#define FISHMOD_ANIMCHECK_CAM_DIST 90.0f
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_ANIMCHECK 1
#define FISHMOD_SCRIPT_NOTEXT 1
#include "src/overlays/actors/ovl_En_Tite/z_en_tite.h"

static Vec3f sScriptStartPos = { -2093.0f, -1037.0f, 705.0f }; // (the lake's south shore, where lake.inc.c starts)
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
