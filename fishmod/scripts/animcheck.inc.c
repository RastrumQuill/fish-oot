// The animation check (fishmod/docs/anim-verify.md): Link held still at the test spot and posed by the harness, one
// case a frame (every frame of every one of his animations, then blends, his model groups, faces, tunics and boots,
// the far level of detail), only he drawn; each case's pose and draw streams hashed (fishmod_debug_animcheck.inc.c).
// fishmod/tools/collog.sh --anim saves the log; fishmod/tools/animcheck_compare.py compares two builds'.
// Build: fishmod/build.sh --script animcheck --hw --colpack --gcc-all   (animcheck_adult: the same, adult)
#define FISHMOD_ANIMCHECK 1
#define FISHMOD_SCRIPT_NOTEXT 1

static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
