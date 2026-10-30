// Part of fishmod_debug.c (#included there, sharing its statics): the play log (FISHMOD_PLAYLOG).
#ifdef FISHMOD_PLAYLOG
/**
 * The play log (fishmod/scripts/playlog.inc.c): while the game is played normally at Lake Hylia, where and why frames
 * run slow. Per situation (camera under water or not, the weather, the rod out or not): frames and their summed times.
 * And the slowest seconds, at most one per spot, each with where Link and the camera were and what was going on.
 * Written to the end of SRAM, after the six save slots (0x7A30), so the saves are untouched; every 30 seconds, and the
 * frames after a write aren't counted. Read: fishmod/tools/playlog_report.py save.sav
 */
#define PLOG_ADDR FISHMOD_SRAM_BIG
#define PLOG_WINDOW 20  // frames per sample ("second")
#define PLOG_WORST 24
#define PLOG_SITS 12    // under (2) x weather (3) x rod (2)
#define PLOG_SPOT 400.0f // two samples closer than this (Link's position) are the same spot

typedef struct PlogSit {
    u32 frames;
    u32 slow; // frames over 55 ms (under 18 fps)
    u32 sum[5]; // period, CPU update, CPU draw, RSP, RDP: 10 us units
} PlogSit; // size = 0x1C

typedef struct PlogMoment {
    u16 period; // average over the sample, 0.1 ms
    u16 periodMax;
    u16 upd;
    u16 draw;
    u16 rsp;
    u16 rdp;
    u16 fishUpd;
    u16 fishDraw;
    u16 scatter;
    u16 props;
    s16 link[3];
    s16 eye[3];
    s16 camYaw;   // the camera's facing
    s16 camPitch;
    u16 dayTime;
    u16 second;   // when (seconds of logging)
    u8 weather;
    u8 flags;     // 1: camera under water, 2: rod out, 4: Link swimming, 8: fish on the line
    u8 fish;      // fishmod actors (fish, rod, manager)
    u8 actors;    // all actors
} PlogMoment; // size = 0x2C

typedef struct PlogLog {
    u32 magic; // 'FPLG'
    u16 version;
    u16 worstCount;
    u32 frames;
    u32 samples;
    PlogSit sit[PLOG_SITS];
    PlogMoment worst[PLOG_WORST];
} PlogLog;

static PlogLog sPlog;
static u32 sPlogSum[11]; // period, upd, draw, rsp, rdp, fishUpd, fishDraw, scatter, props (us/count) + max period
static s32 sPlogFrames;
static s32 sPlogSkip;
static s32 sPlogSinceSave;

static void Fishmod_PlayLog(PlayState* play) {
    Player* player = GET_PLAYER(play);
    u32 v[9];
    s32 under;
    s32 weather;
    s32 rod;
    s32 i;

    if ((play->sceneId != SCENE_LAKE_HYLIA) || (player == NULL) || (play->pauseCtx.state != PAUSE_STATE_OFF) ||
        (play->transitionMode != TRANS_MODE_OFF)) {
        sPlogFrames = 0;
        bzero(sPlogSum, sizeof(sPlogSum));
        return;
    }
    if (sPlogSkip > 0) {
        sPlogSkip--;
        return;
    }
    v[0] = (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    v[1] = sLastCpu[FISHMOD_CPU_UPDATE];
    v[2] = sLastCpu[FISHMOD_CPU_DRAW];
    v[3] = (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
    v[4] = (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
    v[5] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_U_ENGINE]);
    v[6] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_ENGINE]);
    v[7] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_SCATTER]);
    v[8] = gFishmodScatterDrawn;
    if (v[0] > 500000) { // a hitch from something else (a load): not a frame
        return;
    }

    under = gFishmodHaze >= 0.5f;
    weather = CLAMP(Fishmod_Weather(play), 0, 2);
    rod = gFishmodRodInfo.state != 0;
    {
        PlogSit* sit = &sPlog.sit[(under * 3 + weather) * 2 + rod];

        sit->frames++;
        sit->slow += v[0] > 55000;
        for (i = 0; i < 5; i++) {
            sit->sum[i] += v[i] / 10;
        }
    }
    sPlog.frames++;
    for (i = 0; i < 9; i++) {
        sPlogSum[i] += v[i];
    }
    sPlogSum[9] = MAX(sPlogSum[9], v[0]);

    if (++sPlogFrames >= PLOG_WINDOW) {
        PlogMoment m;
        s32 slot = -1;
        Camera* cam = GET_ACTIVE_CAM(play);
        Actor* actor;

        m.period = sPlogSum[0] / (PLOG_WINDOW * 100);
        m.periodMax = sPlogSum[9] / 100;
        m.upd = sPlogSum[1] / (PLOG_WINDOW * 100);
        m.draw = sPlogSum[2] / (PLOG_WINDOW * 100);
        m.rsp = sPlogSum[3] / (PLOG_WINDOW * 100);
        m.rdp = sPlogSum[4] / (PLOG_WINDOW * 100);
        m.fishUpd = sPlogSum[5] / (PLOG_WINDOW * 100);
        m.fishDraw = sPlogSum[6] / (PLOG_WINDOW * 100);
        m.scatter = sPlogSum[7] / (PLOG_WINDOW * 100);
        m.props = sPlogSum[8] / PLOG_WINDOW;
        m.link[0] = player->actor.world.pos.x;
        m.link[1] = player->actor.world.pos.y;
        m.link[2] = player->actor.world.pos.z;
        m.eye[0] = play->view.eye.x;
        m.eye[1] = play->view.eye.y;
        m.eye[2] = play->view.eye.z;
        m.camYaw = Math_Vec3f_Yaw(&play->view.eye, &play->view.at);
        m.camPitch = Math_Vec3f_Pitch(&play->view.eye, &play->view.at);
        m.dayTime = gSaveContext.save.dayTime;
        m.second = sPlog.samples;
        m.weather = weather;
        m.flags = under | (rod << 1) | (((player->stateFlags1 & PLAYER_STATE1_27) != 0) << 2) |
                  ((gFishmodRodInfo.tension > 0.0f) << 3);
        m.fish = 0;
        for (actor = play->actorCtx.actorLists[ACTORCAT_NPC].head; actor != NULL; actor = actor->next) {
            m.fish += (actor->id == ACTOR_EN_FISHMOD);
        }
        m.actors = MIN(play->actorCtx.total, 255);
        (void)cam;
        sPlog.samples++;

        // one per spot: a slower sample replaces the one there; else the fastest of the list, if this is slower
        for (i = 0; i < sPlog.worstCount; i++) {
            PlogMoment* w = &sPlog.worst[i];
            f32 dx = w->link[0] - m.link[0];
            f32 dz = w->link[2] - m.link[2];

            if ((SQ(dx) + SQ(dz) < SQ(PLOG_SPOT)) && ((w->flags & 1) == (m.flags & 1))) {
                slot = (m.period > w->period) ? i : -2;
                break;
            }
        }
        if (slot == -1) {
            if (sPlog.worstCount < PLOG_WORST) {
                slot = sPlog.worstCount++;
            } else {
                s32 best = 0;

                for (i = 1; i < PLOG_WORST; i++) {
                    if (sPlog.worst[i].period < sPlog.worst[best].period) {
                        best = i;
                    }
                }
                if (m.period > sPlog.worst[best].period) {
                    slot = best;
                }
            }
        }
        if (slot >= 0) {
            sPlog.worst[slot] = m;
        }
        sPlogFrames = 0;
        bzero(sPlogSum, sizeof(sPlogSum));

        if (++sPlogSinceSave >= 30) {
            sPlogSinceSave = 0;
            sPlog.magic = 0x46504C47;
            sPlog.version = 1;
            SsSram_ReadWrite(FISHMOD_SRAM_ADDR(PLOG_ADDR), &sPlog, sizeof(sPlog), OS_WRITE);
            sPlogSkip = 3; // the write's own hitch isn't the game's
        }
    }
}
#endif
