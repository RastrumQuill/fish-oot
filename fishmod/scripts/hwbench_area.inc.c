// The area bench (fishmod_debug.c, FISHMOD_BENCH_AREA): the slow stretch of Lake Hylia between the Gerudo river inflow,
// the west bridge and the rock garden, looked at closely. Adult Link (as the test build's warp: Zora tunic) walks a
// slow circle at each spot, in the iron boots on the bed and in his own boots (swimming), each with everything as
// shipped and with the mod's fish, props, water surface and sky off. Every step of the game's update is timed, and
// every kind of actor's updates. Two spots elsewhere on the lake for comparison. Clear midday, the clock held.
// Sums to SRAM after every spot; read with fishmod/tools/area_report.py save.sav.
// Build: fishmod/build.sh --script hwbench_area --opt 5 --hw --f3dex3
#define FISHMOD_HWBENCH 1
#define FISHMOD_BENCH_AREA 1
#define FISHMOD_SCRIPT_ADULT 1
#ifndef FISHMOD_BENCH_LAPS
#define FISHMOD_BENCH_LAPS 30 // about 10 minutes a lap
#endif
#include "tour.inc.c"
#undef FISHMOD_SCRIPT_TIME
#define FISHMOD_SCRIPT_TIME 0x8000 // midday

// x, z, the height to look for the ground below (under the bridge: below its deck), and a name
static FishmodAreaSpot sAreaSpots[] = {
    { -3300, 4600, -1500, "Inflow mouth" },
    { -3100, 4600, -1500, "West of the bridge" },
    { -2900, 4600, -1500, "East of the bridge" },
    { -2600, 4600, -1500, "Rock garden" },
    { -3000, 4900, -1500, "Under the bridge" },
    { -3000, 4300, 0, "On the bridge" },
    { -3600, 5150, -1400, "West weed flat" },
    { -3800, 3900, -900, "River, upstream" },
    { -700, 2900, -900, "North beach (elsewhere)" },
    { -1250, 7450, -1300, "Drop-off (elsewhere)" },
};
