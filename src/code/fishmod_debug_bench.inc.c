// Part of fishmod_debug.c (#included there, sharing its statics): the hardware benches: the play bench and the area bench (FISHMOD_HWBENCH).
#ifdef FISHMOD_HWBENCH
/*
 * The hardware benches' shared core (the play bench, FISHMOD_BENCH_PLAY; the area bench has its own records): blocks
 * of frames in each mode, timed, with the RDP's own counters (only the console has them: how much of its time goes on
 * fetching commands and waiting on memory, drawing, loading textures), and a few memory tests, with the RDP busy and
 * idle. The sums go to SRAM as they're made (the SummerCart writes them to the .sav): hwbench_report.py reads them.
 */
#define BENCH_STOPS 22  // the tour's stops: the legs between them
#define BENCH_SKIP 10   // frames after changing mode, not counted (the frame being timed runs behind)
#if defined(FISHMOD_BENCH_FRAMES)
#define BENCH_FRAMES FISHMOD_BENCH_FRAMES // (the short test)
#else
#define BENCH_FRAMES 240 // frames counted per block: Link on his way along the leg
#endif
#define BENCH_ROUNDS 1
#define BENCH_MICRO_FRAMES 30
#define BENCH_VERSION 8
#define BENCH_CONDS 8 // the times of day (4) x clear or rain
#ifndef FISHMOD_BENCH_LAPS
#define FISHMOD_BENCH_LAPS 15 // then it stops and says so
#endif

// the play bench's modes (Bench_SetMode has what each switches off)
#define BENCH_BASE 0
#define BENCH_MODES 8
#define BENCH_MICRO BENCH_MODES

typedef enum BenchMetric {
    BM_PERIOD,     // frame period, us (the time between graph updates)
    BM_CPU_UPDATE, // Play_Update, us (less the cache filling in the cold mode)
    BM_CPU_DRAW,   // Play_Draw (display lists), us
    BM_RSP_GFX,    // RSP graphics task, us
    BM_RDP,        // RDP (scheduler: from its task's start to its end), us
    BM_RSP_AUDIO,  // RSP audio, us
    BM_DP_CLOCK,   // RDP counters, in 64-cycle units: all cycles
    BM_DP_BUF,     //   command buffer busy (fetching, or held up)
    BM_DP_PIPE,    //   pipeline busy (drawing)
    BM_DP_TMEM,    //   loading TMEM
    BM_SCATTER,    // the props' display lists (CPU), us
    BM_FISH_UPD,   // the fishes' updates (CPU), us
    BM_FISH_DRAW,  // the fishes' draws (CPU), us
    BM_PROPS,      // props drawn
    BENCH_METRICS
} BenchMetric;

typedef enum BenchTest {
    BT_CHASE,         // dependent loads, each to a line not in the cache: latency (per load)
    BT_CHASE_UNCACHED,// the same through KSEG1 (no cache at all)
    BT_READ,          // reading 64 KB through the cache, from RDRAM (per KB)
    BT_READ_UNCACHED, // reading 16 KB uncached (per KB)
    BT_WRITE,         // writing 64 KB through the cache and writing it back (per KB)
    BT_COPY,          // bcopy 64 KB, from RDRAM to RDRAM through the cache (per KB)
    BENCH_TESTS
} BenchTest;

typedef struct BenchCell {
    u32 frames;
    u32 sum[BENCH_METRICS];
    u32 max[BENCH_METRICS];
} BenchCell;

typedef struct BenchMicro {
    u32 count;
    u32 sum; // CPU count cycles (46.875 MHz)
    u32 min;
} BenchMicro;

typedef struct BenchResults {
    u32 magic; // 'FBEN'
    u8 version;
    u8 stops;
    u8 modes;
    u8 metrics;
    u8 tests;
    u8 hw;    // the original-hardware build
    u8 pad[2];
    u32 laps; // tours completed
    u32 visits;
    BenchCell cell[BENCH_STOPS][BENCH_MODES];
    BenchMicro micro[BENCH_TESTS][2]; // [test][RDP idle, busy]
#ifdef BENCH_CONDS
    BenchCell cond[BENCH_CONDS][BENCH_MODES]; // all the legs, per time of day and weather
#endif
} BenchResults;

static BenchResults sBench;
#ifdef BENCH_CONDS
static s32 sPbCond; // this lap's time of day and weather (the play bench)
#endif
static s32 sBenchState; // 0: arriving, 1: measuring, 2: micro
static s32 sBenchFrame;
static s32 sBenchBlock; // block in this visit
static u32* sBenchBuf;  // 128 KB for the memory tests
static u32 sBenchRand = 0x1234567;

#define BENCH_BUF_SIZE 0x20000

static u32 Bench_Rand(void) {
    sBenchRand ^= sBenchRand << 13;
    sBenchRand ^= sBenchRand >> 17;
    sBenchRand ^= sBenchRand << 5;
    return sBenchRand;
}

static s32 Bench_RdpBusy(void) {
    return (IO_READ(DPC_STATUS_REG) & (DPC_STATUS_PIPE_BUSY | DPC_STATUS_CMD_BUSY)) != 0;
}

static void Bench_Micro(void) {
    s32 test = (s32)(Bench_Rand() % BENCH_TESTS);
    u32* buf = sBenchBuf;
    s32 busy0 = Bench_RdpBusy();
    u32 t0;
    u32 t1;
    u32 per = 1;
    s32 i;
    u32 sink = 0;

    osWritebackDCacheAll();
    osInvalDCache(buf, BENCH_BUF_SIZE);
    switch (test) {
        case BT_CHASE:
        case BT_CHASE_UNCACHED: {
            // a random cycle through 2048 lines of the first 64 KB (one word each, 32 bytes apart): each load needs
            // the last
            u32* base = (test == BT_CHASE) ? buf : (u32*)OS_PHYSICAL_TO_K1(OS_K0_TO_PHYSICAL(buf));
            u32 idx = 0;

            t0 = osGetCount();
            for (i = 0; i < 2048; i++) {
                idx = base[idx];
            }
            t1 = osGetCount();
            sink = idx;
            per = 2048;
            break;
        }
        case BT_READ:
            t0 = osGetCount();
            for (i = 0; i < 0x10000 / 4; i += 4) {
                sink += buf[i] + buf[i + 1] + buf[i + 2] + buf[i + 3];
            }
            t1 = osGetCount();
            per = 64;
            break;
        case BT_READ_UNCACHED: {
            u32* k1 = (u32*)OS_PHYSICAL_TO_K1(OS_K0_TO_PHYSICAL(buf));

            t0 = osGetCount();
            for (i = 0; i < 0x4000 / 4; i += 4) {
                sink += k1[i] + k1[i + 1] + k1[i + 2] + k1[i + 3];
            }
            t1 = osGetCount();
            per = 16;
            break;
        }
        case BT_WRITE:
            t0 = osGetCount();
            for (i = 0x10000 / 4; i < 0x20000 / 4; i += 4) {
                buf[i] = i;
                buf[i + 1] = i;
                buf[i + 2] = i;
                buf[i + 3] = i;
            }
            osWritebackDCache(&buf[0x10000 / 4], 0x10000);
            t1 = osGetCount();
            per = 64;
            break;
        default: // BT_COPY
            t0 = osGetCount();
            bcopy(buf, &buf[0x10000 / 4], 0x10000);
            osWritebackDCache(&buf[0x10000 / 4], 0x10000);
            t1 = osGetCount();
            per = 64;
            break;
    }
    // (the writes and the copy only go to the second 64 KB: the chase's cycle in the first is left as it was)
    if (sink == 0xDEADBEEF) {
        sBench.pad[0]++; // (keeps the reads from being optimised away)
    }
    if (busy0 == Bench_RdpBusy()) {
        BenchMicro* m = &sBench.micro[test][busy0];
        u32 c = (t1 - t0) / per;

        m->count++;
        m->sum += c;
        if ((m->min == 0) || (c < m->min)) {
            m->min = c;
        }
    }
}

static void Bench_Record(s32 stop, s32 mode) {
    BenchCell* cell = &sBench.cell[stop][mode];
    u32 v[BENCH_METRICS];
    s32 i;

    v[BM_PERIOD] = (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    v[BM_CPU_UPDATE] = sLastCpu[FISHMOD_CPU_UPDATE];
    v[BM_CPU_DRAW] = sLastCpu[FISHMOD_CPU_DRAW];
    v[BM_RSP_GFX] = (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
    v[BM_RDP] = (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
    v[BM_RSP_AUDIO] = (u32)OS_CYCLES_TO_USEC(gRSPAudioTimeTotal);
    v[BM_DP_CLOCK] = gFishmodRdp.clock >> 6;
    v[BM_DP_BUF] = gFishmodRdp.buf >> 6;
    v[BM_DP_PIPE] = gFishmodRdp.pipe >> 6;
    v[BM_DP_TMEM] = gFishmodRdp.tmem >> 6;
    v[BM_SCATTER] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_SCATTER]);
    v[BM_FISH_UPD] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_U_ENGINE]);
    v[BM_FISH_DRAW] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_ENGINE]);
    v[BM_PROPS] = gFishmodScatterDrawn;
    cell->frames++;
    for (i = 0; i < BENCH_METRICS; i++) {
        cell->sum[i] += v[i];
        cell->max[i] = MAX(cell->max[i], v[i]);
    }
#ifdef BENCH_CONDS
    cell = &sBench.cond[sPbCond][mode];
    cell->frames++;
    for (i = 0; i < BENCH_METRICS; i++) {
        cell->sum[i] += v[i];
        cell->max[i] = MAX(cell->max[i], v[i]);
    }
#endif
}

/** Where the bench is, written to SRAM 0x7000 as each mode starts (0x7E00 is the profile): if the console crashes, the save says where. */
static void Bench_Breadcrumb(s32 stop, s32 mode, s32 block) {
    static struct {
        u32 magic; // 'FBCR'
        u32 laps;
        u8 stop;
        u8 mode;
        u8 block;
        u8 pad;
        u32 count; // breadcrumbs written so far
    } sCrumb;

    sCrumb.magic = 0x46424352;
    sCrumb.laps = sBench.laps;
    sCrumb.stop = stop;
    sCrumb.mode = mode;
    sCrumb.block = block;
    sCrumb.count++;
    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_CRUMB), &sCrumb, sizeof(sCrumb), OS_WRITE);
}

static void Bench_SetMode(s32 mode) {
    static const s32 sFlags[BENCH_MODES] = {
        0, // the first pass along the leg (fish waking as Link arrives)
        0,
        FISHMOD_BENCH_NOFISHAI,
        FISHMOD_BENCH_NOFISH,
        FISHMOD_BENCH_NOFISH | FISHMOD_BENCH_NOFISHAI,
        FISHMOD_BENCH_NOPROPS,
        FISHMOD_BENCH_NOROOMXLU,
        FISHMOD_BENCH_NOPROPS | FISHMOD_BENCH_NOFISH | FISHMOD_BENCH_NOFISHAI | FISHMOD_BENCH_NOROOMXLU |
            FISHMOD_BENCH_NOSKY,
    };
    gFishmodBench = sFlags[mode];
}

static void Bench_Save(void) {
    sBench.magic = 0x4642454E; // 'FBEN'
    sBench.version = BENCH_VERSION;
    sBench.stops = BENCH_STOPS;
    sBench.modes = BENCH_MODES;
    sBench.metrics = BENCH_METRICS;
    sBench.tests = BENCH_TESTS;
    sBench.hw = FISHMOD_PROFILE_HW;
    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_BENCH), &sBench, sizeof(sBench), OS_WRITE);
}

/** The memory tests' buffer, and the sums so far from the save. False if there's no memory for it. */
static s32 Bench_Init(void) {
    if (sBenchBuf == NULL) {
        s32 i;

        sBenchBuf = SYSTEM_ARENA_MALLOC(BENCH_BUF_SIZE, "../fishmod_debug.c", __LINE__);
        if (sBenchBuf == NULL) {
            return false;
        }
        // the chase's cycle: each of the first 2048 lines (of 16 bytes) points to the next in a shuffled order
        {
            static u16 sOrder[2048];

            for (i = 0; i < 2048; i++) {
                sOrder[i] = i;
            }
            for (i = 2047; i > 0; i--) {
                s32 j = (s32)(Bench_Rand() % (u32)(i + 1));
                u16 t = sOrder[i];

                sOrder[i] = sOrder[j];
                sOrder[j] = t;
            }
            for (i = 0; i < 2048; i++) {
                // spread over the first 64 KB: line k at 32 * k bytes (8 words apart; the cache's lines are 16)
                sBenchBuf[sOrder[i] * 8] = sOrder[(i + 1) % 2048] * 8;
            }
        }
        // carry on from an earlier night (the save file keeps the sums)
        SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_BENCH), &sBench, sizeof(sBench), OS_READ);
        if ((sBench.magic != 0x4642454E) || (sBench.version != BENCH_VERSION) || (sBench.hw != FISHMOD_PROFILE_HW)) {
            bzero(&sBench, sizeof(sBench));
        }
    }
    return true;
}

#if defined(FISHMOD_BENCH_PLAY) || defined(FISHMOD_BENCH_AREA)
// (Link's placing and steering, for the play bench and the area bench)
static s32 sPbStuckFrame;
static Vec3f sPbStuckPos;
static s8 sPbStickX;
static s8 sPbStickY;

/** Puts Link at (x, z) on the ground or the water there, facing yaw. */
static void PlayBench_Place(PlayState* play, Player* player, f32 x, f32 probeY, f32 z, s16 yaw) {
    CollisionPoly* poly;
    WaterBox* box;
    Vec3f probe;
    f32 floor;
    f32 surface;

    probe.x = x;
    probe.y = probeY + 200.0f;
    probe.z = z;
    floor = BgCheck_EntityRaycastDown1(&play->colCtx, &poly, &probe);
    if (floor < -30000.0f) {
        floor = player->actor.world.pos.y;
    }
    if (BgCheck_GetWaterSurface(play, &play->colCtx, x, z, &surface, &box) && (surface > floor)) {
        floor = surface;
    }
    player->actor.world.pos.x = player->actor.prevPos.x = x;
    player->actor.world.pos.y = player->actor.prevPos.y = floor + 2.0f;
    player->actor.world.pos.z = player->actor.prevPos.z = z;
    player->actor.shape.rot.y = player->actor.world.rot.y = player->yaw = yaw;
    player->actor.home.pos = player->actor.world.pos;
    player->actor.speed = 0.0f;
    player->actor.velocity.y = 0.0f;
    player->actor.floorBgId = BGCHECK_SCENE; // (not carried back by whatever he stood on)
    sPbStuckFrame = 0;
    sPbStuckPos = player->actor.world.pos;
}

/** The stick for heading toward a world yaw (the stick is read relative to the camera). */
static void PlayBench_Steer(PlayState* play, s16 yaw, s32 mag) {
    s16 rel = yaw - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));

    sPbStickX = (s8)(-Math_SinS(rel) * mag);
    sPbStickY = (s8)(Math_CosS(rel) * mag);
}
#endif

#ifdef FISHMOD_BENCH_PLAY
/*
 * The play bench (fishmod/scripts/hwbench_play.inc.c): Link plays the lake instead of a free camera flying it. He
 * runs, swims and dives from each tour stop to the next with the game's own camera behind him, and every third leg he
 * fishes from where he stands if he's on land (casting, waiting on the float, striking and reeling in what bites).
 * Each leg is timed once per mode, Link put back at its start each time: the first pass (the fish ahead waking as he
 * comes) and then the repeats. Each lap is at one time of day and weather, the clock held there: dawn, midday, dusk
 * and night, clear and in rain, in turn.
 */
#define PB_SKIP 30   // frames after putting Link back, not counted (the camera catching up)
#define PB_CLEANUP 400 // at most, after a fishing block: reeling in and through any catch (not counted)
#define PB_ARRIVED 90.0f

static s32 sPbState; // 0: new lap (reload the lake), 1: loading, 2: a block, 3: after a block, 4: memory tests
static s32 sPbLeg;
static s32 sPbBlock;
static s32 sPbFrame;
static s32 sPbFishing;
static u16 sPbButton;
static s32 sPbActive;
static s32 sPbTimer;

static const u16 sPbTimes[4] = { CLOCK_TIME(6, 30), CLOCK_TIME(12, 0), CLOCK_TIME(17, 30), CLOCK_TIME(23, 0) };
// the laps' conditions in turn (time + 4 * rain): clear dawn, rain midday, clear dusk, rain night, rain dawn,
// clear midday, rain dusk, clear night
static const u8 sPbOrder[BENCH_CONDS] = { 0, 5, 2, 7, 4, 1, 6, 3 };


/** Rain on or off without loading the scene again (the day's own weather is clear: FISHMOD_SCRIPT_DAY). */
static void PlayBench_SetRain(PlayState* play, s32 rain) {
    EnvironmentContext* envCtx = &play->envCtx;

    envCtx->precipitation[PRECIP_RAIN_MAX] = rain ? 20 : 0;
    envCtx->precipitation[PRECIP_RAIN_CUR] = rain ? 20 : 0;
    envCtx->stormState = rain ? STORM_STATE_ON : STORM_STATE_OFF;
    envCtx->skyboxConfig = envCtx->changeSkyboxNextConfig = rain ? 1 : 0;
    envCtx->lightConfig = envCtx->changeLightNextConfig = rain ? 2 : 0;
}

/** True for a stop under the water (its title ends "(under)"). */
static s32 PlayBench_Under(const char* title) {
    s32 len = 0;

    while (title[len] != '\0') {
        len++;
    }
    return (len >= 7) && (title[len - 7] == '(') && (title[len - 2] == 'r') && (title[len - 1] == ')');
}


/** The auto-angler's buttons for the rod's state (as FISHMOD_AUTO_ANGLER). */
static u16 PlayBench_Angle(void) {
    switch (gFishmodRodInfo.state) {
        case 0: // rod away
        case 1: // held: cast
            return ((sPbTimer % 40) == 0) ? BTN_CLEFT : 0;
        case 2: // wind-up
            return (gFishmodRodInfo.castPower < 0.5f) ? BTN_CLEFT : 0;
        case 4: // float: strike on a bite
            return (gFishmodRodInfo.biting && ((sPbTimer % 4) == 0)) ? BTN_B : 0;
        case 5: // on the ground: reel back
            return BTN_A;
        case 6: // fight: reel while the tension is comfortable
            return (gFishmodRodInfo.tension < 60.0f) ? BTN_A : 0;
        case 7: // landed: through the catch textbox
            return ((gFishmodRodInfo.choice != 0) && ((sPbTimer % 8) == 0)) ? BTN_A : 0;
    }
    return 0;
}

static void Fishmod_PlayBenchUpdate(PlayState* play) {
    Player* player = GET_PLAYER(play);
#ifdef FISHMOD_BENCH_LEGS
    s32 n = FISHMOD_BENCH_LEGS; // (the short test: a lap of the first few legs)
#else
    s32 n = ARRAY_COUNT(sTourStops);
#endif
    FishmodTourStop* from;
    FishmodTourStop* to;
    s32 mode;
    s32 i;

    if (!Bench_Init() || (player == NULL)) {
        return;
    }
#ifdef FISHMOD_STALL_TEST
    if (sPbTimer == FISHMOD_STALL_TEST) {
        volatile s32 forever = 1;

        while (forever) {} // (checking the watchdog)
    }
#endif
#ifdef FISHMOD_FAULT_TEST
    if (sPbTimer == FISHMOD_FAULT_TEST) {
        *(volatile u32*)0x10 = 0; // (checking the crash record)
    }
#endif
    Fishmod_SampleRdp();
    sPbTimer++;
    sPbStickX = sPbStickY = 0;
    sPbButton = 0;
    sPbActive = true;

    if (sBench.laps >= FISHMOD_BENCH_LAPS) {
        Bench_SetMode(BENCH_BASE);
        sPbActive = false;
        sTourDebugText = true;
        Fishmod_DebugPrintf(2, "BENCH DONE: %d laps saved", sBench.laps);
        Fishmod_DebugPrintf(3, "You can turn the console off");
        Fishmod_DebugPrintf(4, "and send the .sav file");
        return;
    }
    sPbCond = sPbOrder[sBench.laps % BENCH_CONDS];
    // the lake's enemies still bite, but he never runs out of hearts (the bench must outlast the night)
    gSaveContext.save.info.playerData.health = gSaveContext.save.info.playerData.healthCapacity;
    if (sPbState >= 2) {
        gSaveContext.save.dayTime = sPbTimes[sPbCond % 4]; // the clock held
    }
    // no one stops to talk: the owl goes, and any text is passed through
    {
        Actor* actor = play->actorCtx.actorLists[ACTORCAT_NPC].head;

        for (; actor != NULL; actor = actor->next) {
            if (actor->id == ACTOR_EN_OWL) {
                Actor_Kill(actor);
            }
        }
    }
    if ((Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) && ((sPbTimer % 8) == 0)) {
        sPbButton = BTN_A;
    }

    from = &sTourStops[sPbLeg];
    to = &sTourStops[(sPbLeg + 1) % n];
    mode = sPbBlock;
    switch (sPbState) {
        case 0: // a new lap: this lap's time and weather, set in place (as the Song of Storms brings rain)
            Bench_SetMode(BENCH_BASE);
            gSaveContext.save.dayTime = sPbTimes[sPbCond % 4];
            PlayBench_SetRain(play, sPbCond >= 4);
            sPbState = 1;
            sPbFrame = 0;
            sPbLeg = 0;
            sPbBlock = 0;
            break;

        case 1: // the light and the rain settling
            if (++sPbFrame > 100) {
                sPbState = 2;
                sPbFrame = 0;
            }
            break;

        case 2: { // a block: Link on his way along the leg
            f32 dx;
            f32 dz;
            f32 dist;

            if (sPbFrame == 0) {
                Bench_SetMode(mode);
                Bench_Breadcrumb(sPbLeg, mode, sPbBlock);
                // every other leg on the lake bed in the iron boots; the rest in his own boots (swimming, running,
                // fishing)
                Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, (((sPbLeg % 2) == 0) && ((sPbLeg % 3) != 1))
                                                                ? EQUIP_VALUE_BOOTS_IRON
                                                                : EQUIP_VALUE_BOOTS_KOKIRI);
                Player_SetEquipmentData(play, player);
                PlayBench_Place(play, player, from->eye.x, from->eye.y, from->eye.z,
                                Math_Vec3f_Yaw(&from->eye, &to->eye));
                sPbFishing = false;
            }
            if (sPbFrame == 10) {
                // every third leg he fishes, if he's standing on land
                sPbFishing = ((sPbLeg % 3) == 1) && !(player->stateFlags1 & PLAYER_STATE1_27) &&
                             (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND);
            }
            if (sPbFrame >= PB_SKIP) {
                Bench_Record(sPbLeg, mode);
            }
            dx = to->eye.x - player->actor.world.pos.x;
            dz = to->eye.z - player->actor.world.pos.z;
            dist = sqrtf(SQ(dx) + SQ(dz));
            if (sPbFishing && (sPbFrame > 10)) {
                if (sPbFrame < 16) {
                    PlayBench_Steer(play, Math_Atan2S(dz, dx), 20); // face the water first
                } else {
                    sPbButton |= PlayBench_Angle();
                }
            } else if (sPbFrame > 2) {
                if (dist > PB_ARRIVED) {
                    PlayBench_Steer(play, Math_Atan2S(dz, dx), 80);
                }
                // diving where the leg's end is under the water
                if ((player->stateFlags1 & PLAYER_STATE1_27) && PlayBench_Under(to->title) &&
                    ((sPbTimer % 90) == 0)) {
                    sPbButton |= BTN_A;
                }
                // stuck on a wall or a ledge: on along the way
                if (++sPbStuckFrame >= 40) {
                    f32 moved = Math_Vec3f_DistXZ(&player->actor.world.pos, &sPbStuckPos);

                    if ((moved < 25.0f) && (dist > PB_ARRIVED * 2)) {
                        f32 step = 200.0f / dist;

                        PlayBench_Place(play, player, player->actor.world.pos.x + dx * step,
                                        player->actor.world.pos.y, player->actor.world.pos.z + dz * step,
                                        Math_Atan2S(dz, dx));
                    }
                    sPbStuckFrame = 0;
                    sPbStuckPos = player->actor.world.pos;
                }
            }
            if (++sPbFrame >= PB_SKIP + BENCH_FRAMES) {
                sPbState = 3;
                sPbFrame = 0;
            }
            break;
        }

        case 3: // after a block: the line reeled in and any catch put away, then the next mode, or the next leg
            Bench_SetMode(BENCH_BASE);
            if ((gFishmodRodInfo.state > 1) && (sPbFrame++ < PB_CLEANUP)) {
                if (gFishmodRodInfo.state == 7) {
                    sPbButton |= ((sPbTimer % 8) == 0) ? BTN_A : 0;
                } else if (gFishmodRodInfo.state != 2) {
                    sPbButton |= BTN_A;
                }
                break;
            }
            sPbFrame = 0;
            if (++sPbBlock < BENCH_MODES) {
                sPbState = 2;
                Bench_Save(); // after every block: a run cut short keeps what it timed
            } else {
                sPbState = 4;
            }
            break;

        default: // the memory tests, then the next leg
            Bench_Micro();
            if (++sPbFrame >= BENCH_MICRO_FRAMES) {
                sBench.visits++;
                sPbBlock = 0;
                sPbFrame = 0;
                if (++sPbLeg >= n) {
                    sBench.laps++;
                    sPbState = 0;
                } else {
                    sPbState = 2;
                }
                Bench_Save();
            }
            break;
    }
    {
        static const char* sModeNames[] = { "fresh", "base", "fishfrozen", "nofishdraw", "nofish", "noprops",
                                            "nowater", "bare" };

#ifdef FISHMOD_BENCH_SHOWTEXT
        Fishmod_DebugPrintf(3, "pb s%d f%d P %d %d %d st%08X", sPbState, sPbFrame, (s32)player->actor.world.pos.x,
                            (s32)player->actor.world.pos.y, (s32)player->actor.world.pos.z, player->stateFlags1);
#endif
        Fishmod_DebugPrintf(2, "bench lap %d leg %d %s c%d", sBench.laps + 1, sPbLeg + 1,
                            ((sPbState == 2) && (sPbBlock < BENCH_MODES)) ? sModeNames[sPbBlock] : "-", sPbCond);
    }
}

/** The bench's controller: what Fishmod_PlayBenchUpdate decided last frame. */
static s32 PlayBench_Input(Input* input) {
    u16 buttonDiff;

    if (!sPbActive) {
        return false;
    }
    input->prev = input->cur;
    input->cur.button = sPbButton;
    input->cur.stick_x = sPbStickX;
    input->cur.stick_y = sPbStickY;
    buttonDiff = input->prev.button ^ input->cur.button;
    input->press.button = input->cur.button & buttonDiff;
    input->rel.button = input->prev.button & buttonDiff;
    PadUtils_UpdateRelXY(input);
    input->press.stick_x = 0;
    input->press.stick_y = 0;
    return true;
}
#endif

#ifdef FISHMOD_BENCH_AREA
/*
 * The area bench (fishmod/scripts/hwbench_area.inc.c): at each spot Link walks a slow circle, four ways in turn (the
 * iron boots or his own; everything as shipped, or the mod's fish, props, water surface and sky off), and every step
 * of the update (FishmodSect) and every kind of actor is timed. The sums go to SRAM after every spot.
 */
#define AREA_VARIANTS 4
#define AREA_SETTLE 40
#define AREA_FRAMES 200
#define AREA_TOP 16 // actor kinds kept per cell, the costliest
#define AREA_SPOTS ARRAY_COUNT(sAreaSpots)
#define AREA_BARE (FISHMOD_BENCH_NOPROPS | FISHMOD_BENCH_NOFISH | FISHMOD_BENCH_NOFISHAI | FISHMOD_BENCH_NOROOMXLU | \
                   FISHMOD_BENCH_NOSKY)

typedef enum AreaMetric {
    AM_PERIOD,
    AM_CPU_UPDATE,
    AM_CPU_DRAW,
    AM_RSP,
    AM_RDP,
    AM_FISH_UPD,  // the fish's own updates (FISHMOD_PROF_U_ENGINE)
    AM_FISH_DRAW,
    AM_SCATTER,
    AM_ACTORS_N,  // actors updated (count)
    AM_SECT0,     // then FISHMOD_SECT_MAX steps
    AM_CAT0 = AM_SECT0 + FISHMOD_SECT_MAX, // then the actor categories
    AREA_METRICS = AM_CAT0 + FISHMOD_ACTOR_CATS
} AreaMetric;

typedef struct AreaActor {
    u16 id;
    u8 most; // most of it updating in one frame
    u8 pad;
    u32 usec; // summed over the cell's frames
} AreaActor;

typedef struct AreaCell {
    u32 frames;
    u32 sum[AREA_METRICS]; // microseconds (counts for AM_ACTORS_N)
    u32 maxPeriod;
    AreaActor top[AREA_TOP];
} AreaCell;

typedef struct AreaResults {
    u32 magic; // 'FARE'
    u8 version;
    u8 spots;
    u8 variants;
    u8 sects;
    u8 cats;
    u8 top;
    u8 hw;
    u8 pad;
    u32 laps;
    u32 spot; // where it got to (for a run cut short)
    u32 variant;
    AreaCell cell[ARRAY_COUNT(sAreaSpots)][AREA_VARIANTS];
} AreaResults;

static AreaResults sArea;
static u32* sAreaActorUsec; // [spot][variant][FISHMOD_ACTOR_IDS], microseconds
static u8* sAreaActorMost;  // [spot][variant][FISHMOD_ACTOR_IDS]
static s32 sAreaSpot;
static s32 sAreaVariant;
static s32 sAreaFrame;
static s32 sAreaActive;
static s32 sAreaTimer;
static u16 sAreaButton;

/** Called every frame from Fishmod_DebugTiming, before the step timers are cleared: one frame into the cell. */
static void AreaBench_Sample(void) {
    AreaCell* cell;
    u32 v[AREA_METRICS];
    s32 i;
    s32 base;

    if (!sAreaActive || (sAreaFrame <= AREA_SETTLE) || (sAreaActorUsec == NULL)) {
        return;
    }
    cell = &sArea.cell[sAreaSpot][sAreaVariant];
    v[AM_PERIOD] = (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    v[AM_CPU_UPDATE] = sLastCpu[FISHMOD_CPU_UPDATE];
    v[AM_CPU_DRAW] = sLastCpu[FISHMOD_CPU_DRAW];
    v[AM_RSP] = (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
    v[AM_RDP] = (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
    v[AM_FISH_UPD] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_U_ENGINE]);
    v[AM_FISH_DRAW] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_ENGINE]);
    v[AM_SCATTER] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_SCATTER]);
    v[AM_ACTORS_N] = 0;
    for (i = 0; i < FISHMOD_SECT_MAX; i++) {
        v[AM_SECT0 + i] = (u32)OS_CYCLES_TO_USEC(gFishmodSect[i]);
    }
    for (i = 0; i < FISHMOD_ACTOR_CATS; i++) {
        v[AM_CAT0 + i] = (u32)OS_CYCLES_TO_USEC((OSTime)gFishmodCatCycles[i]);
    }
    base = (sAreaSpot * AREA_VARIANTS + sAreaVariant) * FISHMOD_ACTOR_IDS;
    for (i = 0; i < FISHMOD_ACTOR_IDS; i++) {
        if (gFishmodActorCount[i] != 0) {
            v[AM_ACTORS_N] += gFishmodActorCount[i];
            sAreaActorUsec[base + i] += (u32)OS_CYCLES_TO_USEC((OSTime)gFishmodActorCycles[i]);
            sAreaActorMost[base + i] = MAX(sAreaActorMost[base + i], gFishmodActorCount[i]);
        }
    }
    if (v[AM_PERIOD] > 500000) {
        return; // a load or a stall elsewhere: not a frame
    }
    cell->frames++;
    for (i = 0; i < AREA_METRICS; i++) {
        cell->sum[i] += v[i];
    }
    cell->maxPeriod = MAX(cell->maxPeriod, v[AM_PERIOD]);
}

static void AreaBench_Save(void) {
    s32 spot;
    s32 var;
    s32 i;
    s32 k;

    // each cell's costliest kinds of actor, from the full tallies
    for (spot = 0; spot < (s32)AREA_SPOTS; spot++) {
        for (var = 0; var < AREA_VARIANTS; var++) {
            AreaCell* cell = &sArea.cell[spot][var];
            s32 base = (spot * AREA_VARIANTS + var) * FISHMOD_ACTOR_IDS;

            bzero(cell->top, sizeof(cell->top));
            for (i = 0; i < FISHMOD_ACTOR_IDS; i++) {
                u32 us = sAreaActorUsec[base + i];

                if (us == 0) {
                    continue;
                }
                for (k = 0; k < AREA_TOP; k++) {
                    if (us > cell->top[k].usec) {
                        s32 m;

                        for (m = AREA_TOP - 1; m > k; m--) {
                            cell->top[m] = cell->top[m - 1];
                        }
                        cell->top[k].id = i;
                        cell->top[k].most = sAreaActorMost[base + i];
                        cell->top[k].usec = us;
                        break;
                    }
                }
            }
        }
    }
    sArea.magic = 0x46415245; // 'FARE'
    sArea.version = 1;
    sArea.spots = AREA_SPOTS;
    sArea.variants = AREA_VARIANTS;
    sArea.sects = FISHMOD_SECT_MAX;
    sArea.cats = FISHMOD_ACTOR_CATS;
    sArea.top = AREA_TOP;
    sArea.hw = FISHMOD_PROFILE_HW;
    sArea.spot = sAreaSpot;
    sArea.variant = sAreaVariant;
    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_BENCH), &sArea, sizeof(sArea), OS_WRITE);
}

static void Fishmod_AreaBenchUpdate(PlayState* play) {
    Player* player = GET_PLAYER(play);
    const FishmodAreaSpot* spot;

    if (player == NULL) {
        return;
    }
    if (sAreaActorUsec == NULL) {
        u32 n = AREA_SPOTS * AREA_VARIANTS * FISHMOD_ACTOR_IDS;

        sAreaActorUsec = SYSTEM_ARENA_MALLOC(n * sizeof(u32), "../fishmod_debug.c", __LINE__);
        sAreaActorMost = SYSTEM_ARENA_MALLOC(n, "../fishmod_debug.c", __LINE__);
        if ((sAreaActorUsec == NULL) || (sAreaActorMost == NULL)) {
            Fishmod_DebugPrintf(2, "area bench: no memory");
            sAreaActorUsec = NULL;
            return;
        }
        bzero(sAreaActorUsec, n * sizeof(u32));
        bzero(sAreaActorMost, n);
        // the actor tallies start over with each run: a save from before is left as it was until the first spot
        bzero(&sArea, sizeof(sArea));
    }
    sAreaTimer++;
    sPbStickX = sPbStickY = 0;
    sAreaButton = 0;
    sAreaActive = true;
    gSaveContext.save.info.playerData.health = gSaveContext.save.info.playerData.healthCapacity;
    gSaveContext.save.dayTime = FISHMOD_SCRIPT_TIME;
    {
        Actor* actor = play->actorCtx.actorLists[ACTORCAT_NPC].head;

        for (; actor != NULL; actor = actor->next) {
            if (actor->id == ACTOR_EN_OWL) {
                Actor_Kill(actor);
            }
        }
    }
    if ((Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) && ((sAreaTimer % 8) == 0)) {
        sAreaButton = BTN_A;
    }
    if (sArea.laps >= FISHMOD_BENCH_LAPS) {
        gFishmodBench = 0;
        sAreaActive = false;
        sTourDebugText = true;
        Fishmod_DebugPrintf(2, "BENCH DONE: %d laps saved", sArea.laps);
        Fishmod_DebugPrintf(3, "You can turn the console off");
        Fishmod_DebugPrintf(4, "and send the .sav file");
        return;
    }

    spot = &sAreaSpots[sAreaSpot];
    if (sAreaFrame == 0) {
        // the iron boots on the bed (variants 0, 1) or his own (2, 3); as shipped (0, 2) or the mod's work off (1, 3)
        Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, (sAreaVariant < 2) ? EQUIP_VALUE_BOOTS_IRON
                                                                       : EQUIP_VALUE_BOOTS_KOKIRI);
        Player_SetEquipmentData(play, player);
        PlayBench_Place(play, player, spot->x, spot->probeY, spot->z, (s16)(sAreaTimer * 0x1000));
        gFishmodBench = (sAreaVariant & 1) ? AREA_BARE : 0;
    }
    if (sAreaFrame > 4) {
        // a slow circle: the heading turns once round over the spot's frames
        s16 yaw = (s16)(sAreaFrame * (0x10000 / (AREA_SETTLE + AREA_FRAMES)));

        PlayBench_Steer(play, yaw, 45);
    }
    if (++sAreaFrame > AREA_SETTLE + AREA_FRAMES) {
        sAreaFrame = 0;
        if (++sAreaVariant >= AREA_VARIANTS) {
            sAreaVariant = 0;
            if (++sAreaSpot >= (s32)AREA_SPOTS) {
                sAreaSpot = 0;
                sArea.laps++;
            }
            AreaBench_Save();
        }
    }
    Fishmod_DebugPrintf(2, "area lap %d spot %d %s v%d", sArea.laps + 1, sAreaSpot + 1, spot->name, sAreaVariant);
}

static s32 AreaBench_Input(Input* input) {
    u16 buttonDiff;

    if (!sAreaActive) {
        return false;
    }
    input->prev = input->cur;
    input->cur.button = sAreaButton;
    input->cur.stick_x = sPbStickX;
    input->cur.stick_y = sPbStickY;
    buttonDiff = input->prev.button ^ input->cur.button;
    input->press.button = input->cur.button & buttonDiff;
    input->rel.button = input->prev.button & buttonDiff;
    PadUtils_UpdateRelXY(input);
    input->press.stick_x = 0;
    input->press.stick_y = 0;
    return true;
}
#endif
#endif
