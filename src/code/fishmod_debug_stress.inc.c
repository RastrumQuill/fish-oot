// Part of fishmod_debug.c (#included there, sharing its statics): the stress benches (FISHMOD_STRESS).
/*
 * How much the game holds at full speed (20 fps), found by raising one thing step by step until the frame rate
 * drops:
 * - 1, fish: adult Link in the Iron Boots at the bottom of the Lakeside Laboratory's tank, with Lake Hylia's fish
 *   let into it a few more at a time (the manager's roster capped at the step's count, extra fish sent home);
 * - 2, plants: Link in the Iron Boots on the bare floor of Lake Hylia's deep basin, the scene's own props off, and a
 *   kelp forest (the tall underwater kinds, full models, nearest first) filling the view ahead of him, more plants a
 *   step (fishmod_scatter.c, EnFishmod_StressPlantsDraw).
 * - 3, both: the fish bench, then (warped to Lake Hylia) the plant bench, one ROM for the two.
 * Each step lets the count settle, then times STRESS_MEASURE frames (full speed: the mean frame within 20 fps and at most 1
 * in 20 frames over 52 ms): the frame period, the CPU's update and draw,
 * the RSP and the RDP, the fish's own update and draw. A pass ends one step after the first step under 20 fps (or at
 * the last step); STRESS_PASSES passes, summed per step. Saved after every step ('FSTR'): the fish's at
 * FISHMOD_SRAM_BENCH, the plants' at FISHMOD_SRAM_BENCH_PLANTS, for fishmod/tools/stress_report.py.
 */
#ifdef FISHMOD_STRESS

// by kind (1 fish, 2 plants): the first count, the step, and the frames to settle (the new fish spawn two every 8
// frames)
static const s16 sStressStart[3] = { 0, 6, 40 };
static const s16 sStressStepSize[3] = { 0, 6, 40 };
static const s16 sStressSettle[3] = { 0, 100, 40 };
#define STRESS_START sStressStart[gFishmodStress]
#define STRESS_STEP sStressStepSize[gFishmodStress]
#define STRESS_SETTLE sStressSettle[gFishmodStress]
// the plant bench's start (mode 3 warps there after the fish): the bare floor of the deep basin, at noon
#define STRESS_PLANTS_ENTRANCE ENTR_LAKE_HYLIA_0
#define STRESS_PLANTS_POS { -1100.0f, -2000.0f, 5450.0f }
#define STRESS_PLANTS_YAW 0x1500
#define STRESS_PLANTS_TIME 0x8000
#define STRESS_STEPS 40
#ifndef STRESS_MEASURE // (a script can shorten them, to check a bench quickly)
#define STRESS_MEASURE 160 // 8 seconds at 20 fps
#endif
#ifndef STRESS_PASSES
#define STRESS_PASSES 3
#endif
#define STRESS_FULL_SPEED_US 50800 // a step at full speed: the mean frame no longer than this (19.7 fps)

typedef struct StressStep {
    u32 count;     // fish or plants in this step
    u32 frames;    // measured frames (over all passes)
    u32 period;    // sums, microseconds: the frame period
    u32 cpuUpdate;
    u32 cpuDraw;
    u32 rsp;
    u32 rdp;
    u32 fishUpdate;
    u32 fishDraw;
    u32 worst;     // the longest single frame
    u32 slow;      // frames longer than 52 ms
} StressStep;

static struct {
    u32 magic; // 'FSTR'
    u8 kind;   // FISHMOD_STRESS: 1 fish, 2 plants
    u8 passes; // done
    u8 steps;  // the most steps any pass reached
    u8 version;
    StressStep step[STRESS_STEPS];
} sStress;

static s32 sStressStep;
static s32 sStressFrame;
static s32 sStressFailed; // steps under full speed in this pass

s32 gFishmodStress = (FISHMOD_STRESS == 3) ? 1 : FISHMOD_STRESS; // (both: the fish first)
s32 gFishmodStressCount = 0; // (none until Link has settled on the bottom)
static s32 sStressWait = 80; // frames for Link to sink onto the bottom first

static void Stress_Save(void) {
    SsSram_ReadWrite(FISHMOD_SRAM_ADDR((gFishmodStress == 2) ? FISHMOD_SRAM_BENCH_PLANTS : FISHMOD_SRAM_BENCH),
                     &sStress, sizeof(sStress), OS_WRITE);
}

/** Both benches in one ROM: the fish done, on to Lake Hylia for the plants (placed and booted there as their own). */
static void Stress_NextBench(PlayState* play) {
    static const Vec3f sPlantsPos = STRESS_PLANTS_POS;

    bzero(&sStress, sizeof(sStress));
    sStressStep = 0;
    sStressFrame = 0;
    sStressFailed = 0;
    sStressWait = 80;
    gFishmodStress = 2;
    gFishmodStressCount = 0;
    sScript.startPos = sPlantsPos;
    sScript.startYaw = STRESS_PLANTS_YAW;
    sScript.roster = 0; // (Lake Hylia's own fish, as the plant bench always had)
    gSaveContext.save.dayTime = STRESS_PLANTS_TIME;
    play->nextEntranceIndex = STRESS_PLANTS_ENTRANCE;
    play->transitionTrigger = TRANS_TRIGGER_START;
    play->transitionType = TRANS_TYPE_FADE_BLACK;
    sPlacedPlayer = false; // (placed again once the scene is in, as at boot)
    sPlayFrames = 0;
}

static void Stress_Update(PlayState* play) {
    StressStep* st;

    if (sStressWait > 0) {
        sStressWait--;
        return;
    }

    if ((FISHMOD_STRESS == 3) && (gFishmodStress == 1) && (sStress.passes >= STRESS_PASSES)) {
        Stress_NextBench(play);
        return;
    }
    if (sStress.passes >= STRESS_PASSES) {
        gFishmodStressCount = STRESS_START;
        sStressTag = 0;
#ifdef FISHMOD_PCPROF
        gFishmodBenchDone = true; // (the profiler sends its last counts, then FISHMOD-DONE)
#endif
        Fishmod_DebugPrintf(2, "STRESS DONE: %d passes saved", sStress.passes);
        Fishmod_DebugPrintf(3, "You can turn the console off");
        Fishmod_DebugPrintf(4, "and send the .sav file");
        return;
    }
    sStress.magic = 0x46535452;
    sStress.kind = gFishmodStress;
    sStress.version = 1;
    gFishmodStressCount = STRESS_START + sStressStep * STRESS_STEP;
    st = &sStress.step[sStressStep];
    st->count = gFishmodStressCount;
    sStressFrame++;
    sStressTag = ((u32)gFishmodStress << 24) | ((u32)(sStressStep & 0xFF) << 16) | ((u32)(sStress.passes & 0xFF) << 8) |
                 ((sStressFrame > STRESS_SETTLE) ? 1 : 0);
    if (sStressFrame > STRESS_SETTLE) {
        u32 period = (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);

        st->frames++;
        st->period += period;
        st->cpuUpdate += sLastCpu[FISHMOD_CPU_UPDATE];
        st->cpuDraw += sLastCpu[FISHMOD_CPU_DRAW];
        st->rsp += (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
        st->rdp += (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
        st->fishUpdate += (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_U_ENGINE]);
        st->fishDraw += (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_ENGINE]);
        st->worst = MAX(st->worst, period);
        if (period > 52000) {
            st->slow++;
        }
    }
    if (sStressFrame >= STRESS_SETTLE + STRESS_MEASURE) {
        // this pass's own frames of the step: was it at full speed?
        static u32 sPassPeriod;
        static u32 sPassFrames;
        static u32 sPassSlow;

        sPassPeriod = st->period - sPassPeriod;
        sPassFrames = st->frames - sPassFrames;
        sPassSlow = st->slow - sPassSlow;
#ifdef FISHMOD_PCPROF
        {
            // the step's result over USB too (the morning runner keeps no save): bench pass step count, then this
            // pass's frames, mean frame (us) and frames over 52 ms
            char line[96];

            sprintf(line, "FISHMOD-STRESS %d %d %d %u %u %u %u\n", gFishmodStress, sStress.passes, sStressStep,
                    st->count, sPassFrames, (sPassFrames != 0) ? sPassPeriod / sPassFrames : 0, sPassSlow);
            Sc64_UsbText(line);
        }
#endif
        // full speed: the mean frame within 20 fps, and no more than 1 frame in 20 slow (the mean alone hides a
        // stutter: slow frames and quick ones after them)
        if ((sPassFrames == 0) || (sPassPeriod / sPassFrames > STRESS_FULL_SPEED_US) ||
            (sPassSlow * 20 > sPassFrames)) {
            sStressFailed++;
        }
        sStressFrame = 0;
        sStressStep++;
        sStress.steps = MAX(sStress.steps, sStressStep);
        if ((sStressFailed >= 2) || (sStressStep >= STRESS_STEPS)) {
            // the pass is over: back to the start (the fish over the count go home)
            sStress.passes++;
            sStressStep = 0;
            sStressFailed = 0;
        }
        sPassPeriod = sStress.step[sStressStep].period;
        sPassFrames = sStress.step[sStressStep].frames;
        sPassSlow = sStress.step[sStressStep].slow;
        Stress_Save();
    }
    Fishmod_DebugPrintf(2, "stress %s: pass %d step %d count %d", (gFishmodStress == 1) ? "fish" : "plants",
                        sStress.passes + 1, sStressStep + 1, gFishmodStressCount);
}

/** At the start: the Iron Boots on (both stand on the bottom). */
static void Stress_Place(PlayState* play, Player* player) {
    Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_IRON);
    Player_SetEquipmentData(play, player);
    if (gFishmodStress == 2) {
        gFishmodBench |= FISHMOD_BENCH_NOPROPS; // (the scene's own props off: only the forest)
    }
}

#else
s32 gFishmodStress = 0;
s32 gFishmodStressCount = 0;
#endif
