// Lake Hylia tour, for feedback on the habitat: a free camera glides between stops above and below the water,
// with a Navi-style caption at each. A: next stop, B: back, stick: look around, L: debug text on/off.
// Build: fishmod/build.sh --script tour --opt 5
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1        // a clear day
#define FISHMOD_SCRIPT_TIME 0x5000  // about 7:30 in the morning
#define FISHMOD_TOUR
static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};

// The water's surface is at y -1313 (the full lake).
static FishmodTourStop sTourStops[] = {
    { { -3500, -1325, 4850 }, { -3900, -1450, 5600 }, "Across the west flat (under)",
      "Just under the surface. The weed", "beds run off into the haze: near", "plants up close, far ones as shapes." },
    { { -4000, -1322, 5450 }, { -4000, -1450, 5500 }, "Straight down (under)",
      "Straight down from the surface:", "the weed bed below, and the fish", "that cruise over it." },
    { { -700, -1200, 2900 }, { -400, -1330, 3600 }, "North beach",
      "The open beach by the bench. The", "new dock reaches out from here:", "walk to its end and cast." },
    { { -380, -1345, 3440 }, { -150, -1420, 3950 }, "Beach shallows (under)",
      "Where the beach drops away. Grass", "tufts and ribbon weed begin here;", "minnows and greengill by day." },
    { { -500, -1150, 3500 }, { -950, -1350, 3900 }, "Pillar ruins",
      "The old pillars on the way to", "Zora's Domain. Rocks and weed", "between the blocks, vines on stone." },
    { { -550, -1380, 3550 }, { -1050, -1480, 4000 }, "Pillar ruins (under)",
      "The walkway and plaza stay clear,", "since they're a path. Weed and", "rocks grow alongside them." },
    { { -2050, -1050, 3600 }, { -2450, -1330, 4150 }, "Lab shore and dock",
      "The laboratory's shore: shade,", "pilings and a sunken log or two.", "Bass like the shade here." },
    { { -3300, -800, 4600 }, { -3800, -1250, 3700 }, "Gerudo river inflow",
      "Where the river enters: stones,", "ribbon weed and driftwood it", "carried down. Busiest at dawn." },
    { { -2600, -1420, 4600 }, { -3000, -1560, 5100 }, "Rock garden (under)",
      "Boulders running out toward the", "grave islet. Bass hold at the", "point; pike wait in ambush." },
    { { -3600, -1150, 5150 }, { -3900, -1360, 5550 }, "West weed flat",
      "The wide shallow meadow on the", "west side: eelgrass with sand", "lanes, and a fallen log." },
    { { -3500, -1370, 5200 }, { -3950, -1440, 5750 }, "West weed flat (under)",
      "Inside the meadow. Pike, bass and", "greengill hunt along the edges", "of the beds." },
    { { -1250, -1420, 7450 }, { -1050, -1560, 8050 }, "The drop-off (under)",
      "The south slope, where the bed", "falls away: kelp-like fronds and", "tall weed. Big fish hold here." },
    { { -1300, -1700, 5300 }, { -900, -2150, 6000 }, "Deep basin (under)",
      "Open water, kept bare. Too deep", "for a bobber by day; big fish", "rise toward the surface at dusk." },
    { { -1050, -2100, 5850 }, { -1057, -2380, 6230 }, "Water Temple gate",
      "The deepest water, home of the", "rare deep fish: out of a bobber's", "reach, for sinkers and lures." },
    { { -2400, -1150, 6100 }, { -1900, -1350, 6750 }, "South bridge",
      "The bridge from the grave islet to", "the island, on pilings. Catfish", "shelter beneath it by day." },
    { { -1850, -1370, 6950 }, { -1350, -1440, 7150 }, "Island shallows (under)",
      "The shelf around the main island:", "weed, and branches from its dead", "tree, the lake's only wood." },
    { { -150, -1100, 6500 }, { -800, -1313, 7100 }, "Island shallows",
      "The island's ring from above,", "one of the busiest places in", "the lake." },
    { { 2000, -1150, 6300 }, { 2350, -1340, 6750 }, "East weed flats",
      "Pondweed and eelgrass clumps cut", "by sand lanes. Bass, greengill", "and schools of minnows." },
    { { 1800, -1450, 6300 }, { 2300, -1370, 6800 }, "East weed flats (under)",
      "Among the east beds: big", "pondweed, hanging vines and", "fan weed." },
    { { -1700, -1150, 8250 }, { -1850, -1300, 8750 }, "South reed bank",
      "Walls of reeds and lily pads. The", "tall pale reeds are the Fishing", "Pond's own; the clumps are new." },
    { { -1900, -1480, 8300 }, { -2000, -1330, 8750 }, "Under the lilies",
      "Looking up at the lily pads from", "below. Bass rest under them at", "dawn and dusk." },
    { { 2150, -1160, 4750 }, { 2700, -1330, 5400 }, "East shore",
      "South of the Fishing Pond house:", "lilies and reeds. That's the tour.", "Press A to go round again." },
};
