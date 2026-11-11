// The view bench, over a SummerCart64's USB (fishmod_debug.c, FISHMOD_VIEW_BENCH): the camera goes round the Lake
// Hylia tour's 22 stops; at each, once it has settled, the scene is frozen and seven settings take turns for 60 frames
// each (as shipped; no fish; no props; no bed cards; no water surface; no sky; neither fish nor props), so what each part
// of the mod costs the RSP, the RDP and the CPU is measured on the same picture. Two laps (about 25 minutes). Every
// frame's record goes to the frame log (as pcprof_frames), tagged with its stop and setting; read with
// fishmod/tools/viewbench.py FOLDER.
// Build: fishmod/build.sh --script pcprof_views --hw --colpack --gcc-all
#define FISHMOD_PCPROF 1
#define FISHMOD_FRAMELOG 1
#define FISHMOD_PCPROF_NORING 1
#define FISHMOD_VIEW_BENCH 1
#define FISHMOD_TOUR_AUTO 100000 // (the bench moves the camera on itself)
#include "tour.inc.c"
