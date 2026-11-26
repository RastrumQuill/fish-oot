// The Temple of Time's seams (tools/seampass.py): five cameras where most of its T-junctions were (the pedestal's
// octagon, the floor in the middle and by the door, the hall toward the entrance, the ceiling). Build with and without
// --seams and compare: fishmod/tools/tour_capture.py ROM OUT --stops 5.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_TEMPLE_OF_TIME_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_TOUR
#define FISHMOD_TOUR_AUTO 200
static Vec3f sScriptStartPos = { 0.0f, 0.0f, 1500.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};

static FishmodTourStop sTourStops[] = {
    { { 0, 150, 450 }, { 0, 20, 0 }, "Seams: the octagon", "", "", "" },
    { { -520, 120, 220 }, { -120, 30, 0 }, "Seams: the octagon's side", "", "", "" },
    { { 0, 160, 1750 }, { 0, 0, 1150 }, "Seams: the floor by the door", "", "", "" },
    { { 0, 220, 1900 }, { 0, 120, 2600 }, "Seams: toward the entrance", "", "", "" },
    { { 0, 300, -250 }, { 0, 950, -800 }, "Seams: the ceiling", "", "", "" },
};
