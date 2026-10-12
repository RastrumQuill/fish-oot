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
    { { -1200, 1500, 2600 }, { -1200, -1450, 6200 }, "The whole lake",
      "From up here the props are too far",
      "to draw (they show within ~950", "units); the stops below go close." },
    { { -1100, 3200, 5500 }, { -1100, -1400, 5650 }, "Straight down",
      "The same from overhead. Bare patches", "are on purpose (beds are patchy);", "tell me where it looks empty." },
    { { -700, -1200, 2900 }, { -400, -1330, 3600 }, "North beach",
      "Kept open for casting from the bench", "and a dock to come: sand, a few", "tufts, no reed walls here." },
    { { -500, -1340, 3500 }, { -200, -1420, 3950 }, "Beach shallows (under)",
      "Grass tufts and ribbon weed starting", "as the bed drops. Minnows and", "greengill live here by day." },
    { { -500, -1150, 3500 }, { -950, -1350, 3900 }, "Pillar ruins",
      "The old pillars by the way to Zora's", "Domain: rocks and weed between", "the blocks, vines on the stone." },
    { { -550, -1380, 3550 }, { -1050, -1480, 4000 }, "Pillar ruins (under)",
      "The walkway and plaza are kept", "clear (it's a path); weed and", "rocks grow beside, not on it." },
    { { -2050, -1050, 3600 }, { -2450, -1330, 4150 }, "Lab shore and dock",
      "The lab's shore: shade, pilings,", "a sunken log or two.", "Bass in the shade." },
    { { -3300, -800, 4600 }, { -3800, -1250, 3700 }, "Gerudo river inflow",
      "Where the river comes in: stones,", "ribbon weed, driftwood it brought.", "Current lovers, busiest at dawn." },
    { { -2600, -1420, 4600 }, { -3000, -1560, 5100 }, "Rock garden (under)",
      "Boulders running out toward the", "grave islet, iwagumi-style.", "Bass at the point, pike in ambush." },
    { { -3600, -1150, 5150 }, { -3900, -1360, 5550 }, "West weed flat",
      "Wide shallow meadow on the west", "side: eelgrass with sand lanes,", "a laydown log." },
    { { -3500, -1370, 5200 }, { -3950, -1440, 5750 }, "West weed flat (under)",
      "Is it dense enough? Too dense?", "Pike, bass and greengill hunt", "the edges of the beds." },
    { { -1250, -1420, 7450 }, { -1050, -1560, 8050 }, "The drop-off (under)",
      "The south slope, where the bed falls", "away: kelp-like fronds, tall weed.", "Big fish hold at this edge." },
    { { -1300, -1700, 5300 }, { -900, -2150, 6000 }, "Deep basin (under)",
      "Open water, no plants on purpose.", "Too deep for a bobber by day; big", "fish rise toward it at dusk." },
    { { -1050, -2100, 5850 }, { -1057, -2380, 6230 }, "Water Temple gate",
      "The deepest water, where the rare", "deep fish live: too deep for a", "bobber (sinkers and lures, later)." },
    { { -2400, -1150, 6100 }, { -1900, -1350, 6750 }, "South bridge",
      "Bridge from the grave islet to the", "island, on pilings. Catfish under", "it by day, roaming at night." },
    { { -1850, -1370, 6950 }, { -1350, -1440, 7150 }, "Island shallows (under)",
      "The shelf around the main island:", "weed, and branches off its dead", "tree (the lake's only wood)." },
    { { -150, -1100, 6500 }, { -800, -1313, 7100 }, "Island shallows",
      "From above. The island's ring is", "one of the busiest places;", "too busy?" },
    { { 2000, -1150, 6300 }, { 2350, -1340, 6750 }, "East weed flats",
      "Pondweed and eelgrass clumps with", "sand lanes. Bass, greengill,", "minnow schools." },
    { { 1800, -1450, 6300 }, { 2300, -1370, 6800 }, "East weed flats (under)",
      "Big pondweed, vines, fan weed.", "Does this read as a weed bed", "or as scattered bits?" },
    { { -1700, -1150, 8250 }, { -1850, -1300, 8750 }, "South reed bank",
      "Reed walls and lily pads. The tall", "see-through reeds are the Fishing", "Pond's own; the clumps are new." },
    { { -1900, -1480, 8300 }, { -2000, -1330, 8750 }, "Under the lilies",
      "Looking up at the pads from below.", "Bass sit under them at dawn", "and dusk." },
    { { 1700, -1150, 4450 }, { 2050, -1330, 4800 }, "East shore",
      "Toward the Fishing Pond house: lily", "field and reeds. That's the tour.", "Press A to go round again!" },
};
