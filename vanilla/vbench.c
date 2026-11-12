/*
 * The vanilla bench (vanilla/README.md): fish-oot's play-bench route at Lake Hylia (fishmod/scripts/hwbench_play.inc.c
 * and tour.inc.c, the same stops and blocks), played by itself on the unmodified game, with each frame logged and the
 * CPU sampled the way the mod's frame-log build does it (src/code/fishmod_debug_pcprof.inc.c, FISHMOD_FRAMELOG),
 * over a SummerCart64's USB. The records and packets are the mod's, so fishmod/tools/framelog.py and
 * n64prof/tools/n64prof.py read both runs alike.
 *
 * Where the mod's route fishes (every third leg, on land), Link turns to the water and stands: vanilla has no rod
 * there. Each block is timed for the same frames either way.
 *
 * Its buffers are in the Expansion Pak's RAM above what the game uses (vanilla's frame buffers end near 0x8044C000
 * even with 8 MB), so the game's own memory is as it is without the bench.
 */
#include "vbench.h"
#include "ultra64.h"
#include "ultra64/osint.h"
#include "array_count.h"
#include "attributes.h"
#include "controller.h"
#include "libu64/pad.h"
#include "libc64/sprintf.h"
#include "libu64/gfxprint.h"
#include "sram.h"
#include "inventory.h"
#include "actor.h"
#include "gfx.h"
#include "play_state.h"
#include "player.h"
#include "save.h"
#include "environment.h"
#include "camera.h"
#include "bgcheck.h"
#include "message.h"
#include "sys_math.h"
#include "z_lib.h"
#include "speed_meter.h"
#include "segment_symbols.h"
#include "z_game_dlftbls.h"
#include "z_actor_dlftbls.h"
#include "kaleido_manager.h"
#include "effect.h"
#include "carthandle.h"
#include "sequence.h"
#include "item.h"
#include "regs.h"
#include "libc/math.h"
#include "libc64/qrand.h"
#include "versions.h"

#include "n64prof/n64prof.c"

// --- the route: fish-oot's tour stops, from the same file the mod's build reads ---
typedef struct FishmodTourStop {
    Vec3f eye;
    Vec3f at;
    const char* title;
    const char* lines[3];
} FishmodTourStop;
typedef struct FishmodAutoplayStep {
    u16 frames;
    u16 button;
    s8 stickX;
    s8 stickY;
    u8 inPlay;
    u8 untilPause;
} FishmodAutoplayStep;
#include "vbench_tour.inc.c"

#define BENCH_LAPS 8
#define BENCH_MODES 1      // blocks a leg: once, the first pass (as the mod's frame-log build, PB_BLOCKS 1)
#define BENCH_FRAMES 240   // frames counted a block
#define BENCH_GAP_FRAMES 30 // after a leg's blocks (the mod's memory tests' frames, the tests left out in both)
#define PB_SKIP 30
#define PB_ARRIVED 90.0f
#define BENCH_CONDS 8

OSTime gVBenchSect[VB_SECT_MAX];
OSTime gVBenchCpuTime[2];
u32 gVBenchActorCycles[VB_ACTOR_IDS];
u32 gVBenchCatCycles[VB_ACTOR_CATS];
u32 gVBenchCounterPc;
void (*gVBenchTimerHook)(OSTimer* timer);

static s32 sPlayFrames;
static s32 sPlaced;
static s32 sActive;
static s32 sState;
static s32 sLeg;
static s32 sBlock;
static s32 sFrame;
static s32 sTimer;
static s32 sLaps;
static s32 sCond;
static s32 sFishing;
static s32 sCounted;
static s32 sDone;
static s32 sStuckFrame;
static Vec3f sStuckPos;
static s8 sStickX;
static s8 sStickY;
static u16 sButton;
static char sTextLine[64];

static const u16 sTimes[4] = { CLOCK_TIME(6, 30), CLOCK_TIME(12, 0), CLOCK_TIME(17, 30), CLOCK_TIME(23, 0) };
static const u8 sOrder[BENCH_CONDS] = { 0, 5, 2, 7, 4, 1, 6, 3 };

/** From the title setup: a save as the mod's debug boot makes it (adult, the full lake), straight into Lake Hylia. */
void VBench_Boot(void) {
    gSaveContext.save.linkAge = LINK_AGE_CHILD;
    gSaveContext.fileNum = 0xFF;
    Sram_InitDebugSave();
    gSaveContext.language = LANGUAGE_ENG; // (file select would set it: the US ROM's own language)
    SET_EVENTCHKINF(EVENTCHKINF_B1); // (Lake Hylia's intro cutscene seen)
    gSaveContext.save.dayTime = FISHMOD_SCRIPT_TIME;
    gSaveContext.save.totalDays = FISHMOD_SCRIPT_DAY;
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.buttonStatus[0] = gSaveContext.buttonStatus[1] = gSaveContext.buttonStatus[2] =
        gSaveContext.buttonStatus[3] = gSaveContext.buttonStatus[4] = BTN_ENABLED;
    gSaveContext.save.entranceIndex = FISHMOD_SCRIPT_ENTRANCE;
    gSaveContext.save.linkAge = LINK_AGE_ADULT;
    gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_MASTER;
    Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
    Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_IRON);
    Inventory_ChangeEquipment(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_ZORA);
    Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_HYLIAN);
    SET_EVENTCHKINF(EVENTCHKINF_RESTORED_LAKE_HYLIA);
    gSaveContext.respawnFlag = 0;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = ENTR_LOAD_OPENING;
    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
    gSaveContext.natureAmbienceId = 0xFF;
    gSaveContext.showTitleCard = false;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    sPlayFrames = 0;
    sPlaced = false;
}

/** Puts Link at (x, z) on the ground or the water there, facing yaw (as the mod's bench). */
static void VBench_Place(PlayState* play, Player* player, f32 x, f32 probeY, f32 z, s16 yaw) {
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
    player->actor.floorBgId = BGCHECK_SCENE;
    sStuckFrame = 0;
    sStuckPos = player->actor.world.pos;
}

static void VBench_Steer(PlayState* play, s16 yaw, s32 mag) {
    s16 rel = yaw - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));

    sStickX = (s8)(-Math_SinS(rel) * mag);
    sStickY = (s8)(Math_CosS(rel) * mag);
}

static void VBench_SetRain(PlayState* play, s32 rain) {
    EnvironmentContext* envCtx = &play->envCtx;

    envCtx->precipitation[PRECIP_RAIN_MAX] = rain ? 20 : 0;
    envCtx->precipitation[PRECIP_RAIN_CUR] = rain ? 20 : 0;
    envCtx->stormState = rain ? STORM_STATE_ON : STORM_STATE_OFF;
    envCtx->skyboxConfig = envCtx->changeSkyboxNextConfig = rain ? 1 : 0;
    envCtx->lightConfig = envCtx->changeLightNextConfig = rain ? 2 : 0;
}

static s32 VBench_Under(const char* title) {
    s32 len = 0;

    while (title[len] != '\0') {
        len++;
    }
    return (len >= 7) && (title[len - 7] == '(') && (title[len - 2] == 'r') && (title[len - 1] == ')');
}

// --- the profiler and the frame log over the SummerCart64 (as the mod's fishmod_debug_pcprof.inc.c) ---
#define SC64_BUFFER 0x1FFE0000
#define SC64_BUFFER_SIZE 0x2000
#define SC64_SCR 0x10000
#define SC64_DATA0 0x10004
#define SC64_DATA1 0x10008
#define SC64_IDENTIFIER 0x1000C
#define SC64_KEY 0x10010
#define SC64_CMD_BUSY (1u << 31)
#define SC64_CMD_ERROR (1u << 30)

#define PCPROF_INTERVAL 11719
#define PCPROF_HASH 16384
#define PCPROF_SWAP_FRAMES 200 // about 10 s at 20 fps
#define PCPROF_RING 1024
#define FRAMELOG_WORDS 52
#define FRAMELOG_PER_PACKET ((SC64_BUFFER_SIZE - 16) / (FRAMELOG_WORDS * 4))
#define FRAMELOG_SLOW_US 52000

// the Expansion Pak's RAM the game leaves alone
#define VB_RAM 0x80700000
#define VB_TABLE_A ((N64ProfEntry*)(VB_RAM + 0x00000))
#define VB_TABLE_B ((N64ProfEntry*)(VB_RAM + 0x20000))
#define VB_OUT ((u8*)(VB_RAM + 0x40000))
#define VB_RINGS ((u32*)(VB_RAM + 0x42000))
#define VB_FRAMELOG ((FrameLogPacket*)(VB_RAM + 0x44000))

typedef struct FrameLogPacket {
    u32 magic;
    u32 version;
    u32 count;
    u32 first;
    u32 rec[FRAMELOG_PER_PACKET][FRAMELOG_WORDS];
} FrameLogPacket;

static OSPiHandle sSc64Handle;
static OSIoMesg sSc64IoMesg;
static OSMesgQueue sSc64Queue;
static OSMesg sSc64Msg;
static OSTimer sPcTimer;
static s32 sUsb;      // 1: a SummerCart64, streaming
static s32 sStopped;
static N64ProfEntry* sSpare;
static N64ProfEntry* sSending;
static unsigned int sSendPos; // (n64prof's own types: u32 is unsigned long here)
static u32 sPackets;
static s32 sLastSwap;
static FrameLogPacket* sFl[2];
static s32 sFlCur;
static s32 sFlReady = -1;

static void Sc64_Write(u32 reg, u32 value) {
    osEPiWriteIo(&sSc64Handle, reg, value);
}

static u32 Sc64_Read(u32 reg) {
    u32 value = 0;

    osEPiReadIo(&sSc64Handle, reg, &value);
    return value;
}

static s32 Sc64_Command(u8 id, u32 arg0, u32 arg1, u32* data0) {
    s32 tries;

    Sc64_Write(SC64_DATA0, arg0);
    Sc64_Write(SC64_DATA1, arg1);
    Sc64_Write(SC64_SCR, id);
    for (tries = 0; tries < 100000; tries++) {
        u32 scr = Sc64_Read(SC64_SCR);

        if (!(scr & SC64_CMD_BUSY)) {
            if (scr & SC64_CMD_ERROR) {
                return -1;
            }
            if (data0 != NULL) {
                *data0 = Sc64_Read(SC64_DATA0);
            }
            return 0;
        }
    }
    return -1;
}

static s32 Sc64_Init(void) {
    u32 prevInt;

    if (gCartHandle == NULL) {
        return 0;
    }
    sSc64Handle = *gCartHandle;
    sSc64Handle.baseAddress = (u32)OS_PHYSICAL_TO_K1(SC64_BUFFER);
    bzero(&sSc64Handle.transferInfo, sizeof(__OSTranxInfo));
    prevInt = __osDisableInt();
    sSc64Handle.next = __osPiTable;
    __osPiTable = &sSc64Handle;
    __osRestoreInt(prevInt);
    osCreateMesgQueue(&sSc64Queue, &sSc64Msg, 1);
    Sc64_Write(SC64_KEY, 0x5F554E4C);
    Sc64_Write(SC64_KEY, 0x4F434B5F);
    return Sc64_Read(SC64_IDENTIFIER) == 0x53437632;
}

static s32 Sc64_UsbBusy(void) {
    u32 status;

    if (Sc64_Command('U', 0, 0, &status) != 0) {
        return 1;
    }
    return (status & SC64_CMD_BUSY) != 0;
}

static s32 Sc64_UsbSend(void* buf, u32 len) {
    OSMesg msg;

    sSc64IoMesg.hdr.pri = OS_MESG_PRI_NORMAL;
    sSc64IoMesg.hdr.retQueue = &sSc64Queue;
    sSc64IoMesg.dramAddr = buf;
    sSc64IoMesg.devAddr = 0;
    sSc64IoMesg.size = len;
    osWritebackDCache(buf, len);
    osEPiStartDma(&sSc64Handle, &sSc64IoMesg, OS_WRITE);
    osRecvMesg(&sSc64Queue, &msg, OS_MESG_BLOCK);
    return Sc64_Command('M', SC64_BUFFER, (2 << 24) | len, NULL);
}

static void PcProf_Hook(OSTimer* timer) {
    if (timer == &sPcTimer) {
        n64prof_sample(gVBenchCounterPc);
        timer->interval = n64prof_next_interval();
    }
}

static void PcProf_Ranges(void) {
    N64ProfRange* r = n64prof_ranges_begin();
    s32 n = 0;
    s32 i;

#define VB_ADD(ram, vs, ve)                                                  \
    if (((ram) != NULL) && ((vs) != NULL) && (n < N64PROF_MAX_RANGES)) {     \
        r[n].ramStart = (u32)(ram);                                          \
        r[n].ramEnd = (u32)(ram) + ((u32)(ve) - (u32)(vs));                  \
        r[n].vramStart = (u32)(vs);                                          \
        n++;                                                                 \
    }
    for (i = 0; i < ACTOR_ID_MAX; i++) {
        VB_ADD(gActorOverlayTable[i].loadedRamAddr, gActorOverlayTable[i].vramStart, gActorOverlayTable[i].vramEnd);
    }
    for (i = 0; i < EFFECT_SS_TYPE_MAX; i++) {
        VB_ADD(gEffectSsOverlayTable[i].loadedRamAddr, gEffectSsOverlayTable[i].vramStart,
               gEffectSsOverlayTable[i].vramEnd);
    }
    for (i = 0; i < GAMESTATE_ID_MAX; i++) {
        VB_ADD(gGameStateOverlayTable[i].loadedRamAddr, gGameStateOverlayTable[i].vramStart,
               gGameStateOverlayTable[i].vramEnd);
    }
    for (i = 0; i < KALEIDO_OVL_MAX; i++) {
        VB_ADD(gKaleidoMgrOverlayTable[i].loadedRamAddr, gKaleidoMgrOverlayTable[i].vramStart,
               gKaleidoMgrOverlayTable[i].vramEnd);
    }
#undef VB_ADD
    n64prof_ranges_end(n);
}

static void PcProf_Start(void) {
    if (osMemSize < 0x800000) {
        sprintf(sTextLine, "vbench: no Expansion Pak");
        return;
    }
    sUsb = Sc64_Init();
    if (!sUsb) {
        sprintf(sTextLine, "vbench: no SummerCart64 found");
        return;
    }
    n64prof_set_bucket_shift(2);
    n64prof_init(VB_TABLE_A, PCPROF_HASH, PCPROF_INTERVAL, (u32)_codeSegmentEnd);
    sSpare = VB_TABLE_B;
    n64prof_clear(sSpare);
    sFl[0] = VB_FRAMELOG;
    sFl[1] = VB_FRAMELOG + 1;
    sFl[0]->count = sFl[1]->count = 0;
#ifdef VB_FRAME_TAGS // (off: the tags crashed on the console in frames1, not in ares; releases/history/notes-bisect1.md)
    n64prof_set_frame_ring((unsigned int*)VB_RINGS, PCPROF_RING);
#endif
    PcProf_Ranges();
    gVBenchTimerHook = PcProf_Hook;
    osSetTimer(&sPcTimer, PCPROF_INTERVAL, PCPROF_INTERVAL, NULL, NULL);
    sLastSwap = sPlayFrames;
}

/** The frame just ended: its samples tagged, and its record logged (the mod's layout). */
static void VBench_FrameEnd(void) {
    u32 period = (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    FrameLogPacket* pk;
    u32* r;
    u32 top[4];
    s32 i;
    s32 j;

    if (sUsb && !sStopped) {
        n64prof_frame_end((period > FRAMELOG_SLOW_US) ? 1 : 2);
        pk = sFl[sFlCur];
        if (pk->count >= FRAMELOG_PER_PACKET) {
            if (sFlReady >= 0) {
                goto clear; // (both full: this record is lost)
            }
            sFlReady = sFlCur;
            sFlCur ^= 1;
            pk = sFl[sFlCur];
            pk->count = 0;
        }
        if (pk->count == 0) {
            pk->magic = 0x4646524D;
            pk->version = 1;
            pk->first = sPlayFrames;
        }
        r = pk->rec[pk->count++];
        bzero(r, FRAMELOG_WORDS * 4);
        r[0] = sPlayFrames;
        r[1] = ((u32)(sLeg & 0xFF) << 24) | ((u32)(sLaps & 0xFF) << 16) | ((u32)(sCond & 0xFF) << 8) |
               (sCounted ? 1 : 0);
        r[2] = period;
        r[3] = (u32)OS_CYCLES_TO_USEC(gVBenchCpuTime[0]);
        r[4] = (u32)OS_CYCLES_TO_USEC(gVBenchCpuTime[1]);
        r[5] = (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
        r[6] = (u32)OS_CYCLES_TO_USEC(gRSPAudioTimeTotal);
        r[7] = (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
        r[8] = IO_READ(DPC_CLOCK_REG) & 0xFFFFFF;
        r[9] = IO_READ(DPC_BUFBUSY_REG) & 0xFFFFFF;
        r[10] = IO_READ(DPC_PIPEBUSY_REG) & 0xFFFFFF;
        r[11] = IO_READ(DPC_TMEM_REG) & 0xFFFFFF;
        IO_WRITE(DPC_STATUS_REG, DPC_CLR_CLOCK_CTR | DPC_CLR_CMD_CTR | DPC_CLR_PIPE_CTR | DPC_CLR_TMEM_CTR);
        for (i = 0; i < VB_SECT_MAX; i++) {
            r[16 + i] = (u32)OS_CYCLES_TO_USEC(gVBenchSect[i]);
        }
        for (i = 0; i < VB_ACTOR_CATS; i++) {
            r[36 + i] = (u32)OS_CYCLES_TO_USEC((OSTime)gVBenchCatCycles[i]);
        }
        top[0] = top[1] = top[2] = top[3] = 0;
        for (i = 0; i < VB_ACTOR_IDS; i++) {
            u32 c = gVBenchActorCycles[i];

            if (c > top[3]) {
                for (j = 3; (j > 0) && (c > top[j - 1]); j--) {
                    top[j] = top[j - 1];
                    r[48 + j] = r[48 + j - 1];
                }
                top[j] = c;
                r[48 + j] = ((u32)i << 20) | MIN((u32)OS_CYCLES_TO_USEC((OSTime)c), 0xFFFFF);
            }
        }
    }
clear:
    sCounted = false;
    gVBenchCpuTime[0] = gVBenchCpuTime[1] = 0;
    bzero(gVBenchSect, sizeof(gVBenchSect));
    bzero(gVBenchActorCycles, sizeof(gVBenchActorCycles));
    bzero(gVBenchCatCycles, sizeof(gVBenchCatCycles));
}

/** Once a frame: one packet to the PC at most (the frame log first). */
static void PcProf_Update(void) {
    unsigned int user[6];

    if (!sUsb) {
        return;
    }
    user[0] = 0x56414E49; // 'VANI'
    user[1] = sPlayFrames;
    user[2] = 1;
    user[3] = sStopped ? 1 : 0;
    user[4] = user[5] = 0;
    if ((sPlayFrames & 7) == 0) {
        PcProf_Ranges();
    }
    if (sDone && !sStopped && (sSending == NULL)) {
        osStopTimer(&sPcTimer);
        gVBenchTimerHook = NULL;
        sStopped = true;
        sSending = n64prof_swap(sSpare);
        sSpare = NULL;
        sSendPos = 0;
    }
    if ((sSending == NULL) && !sStopped &&
        ((n64prof_used() >= PCPROF_HASH / 2) || (sPlayFrames - sLastSwap >= PCPROF_SWAP_FRAMES))) {
        sSending = n64prof_swap(sSpare);
        sSpare = NULL;
        sSendPos = 0;
        sLastSwap = sPlayFrames;
    }
    if ((sFlReady >= 0) && !Sc64_UsbBusy()) {
        FrameLogPacket* pk = sFl[sFlReady];

        if (Sc64_UsbSend(pk, 16 + pk->count * FRAMELOG_WORDS * 4) == 0) {
            sPackets++;
        }
        sFlReady = -1;
    } else if ((sSending != NULL) && !Sc64_UsbBusy()) {
        u32 bytes = n64prof_pack(sSending, &sSendPos, VB_OUT, SC64_BUFFER_SIZE, user);

        if (Sc64_UsbSend(VB_OUT, bytes) == 0) {
            sPackets++;
        }
        if (n64prof_pack_done(sSendPos)) {
            n64prof_clear(sSending);
            sSpare = sSending;
            sSending = NULL;
        }
    }
    if (sStopped && (sSending == NULL)) {
        sprintf(sTextLine, "vbench usb: DONE, %u packets: stop sc64deployer", sPackets);
    } else {
        sprintf(sTextLine, "vbench usb: %u packets", sPackets);
    }
}

// --- the route, played (fish-oot's Fishmod_PlayBenchUpdate, the rod left out) ---
static void VBench_Route(PlayState* play, Player* player) {
    s32 n = ARRAY_COUNT(sTourStops);
    FishmodTourStop* from;
    FishmodTourStop* to;
    Actor* actor;

    sTimer++;
    sStickX = sStickY = 0;
    sButton = 0;
    sActive = true;
    if (sLaps >= BENCH_LAPS) {
        sActive = false;
        sDone = true;
        return;
    }
    sCond = sOrder[sLaps % BENCH_CONDS];
    gSaveContext.save.info.playerData.health = gSaveContext.save.info.playerData.healthCapacity;
    if (sState >= 2) {
        gSaveContext.save.dayTime = sTimes[sCond % 4];
    }
    for (actor = play->actorCtx.actorLists[ACTORCAT_NPC].head; actor != NULL; actor = actor->next) {
        if (actor->id == ACTOR_EN_OWL) {
            Actor_Kill(actor);
        }
    }
    if ((Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) && ((sTimer % 8) == 0)) {
        sButton = BTN_A;
    }
    from = &sTourStops[sLeg];
    to = &sTourStops[(sLeg + 1) % n];
    switch (sState) {
        case 0:
            gSaveContext.save.dayTime = sTimes[sCond % 4];
            VBench_SetRain(play, sCond >= 4);
            sState = 1;
            sFrame = 0;
            sLeg = 0;
            sBlock = 0;
            break;

        case 1:
            if (++sFrame > 100) {
                sState = 2;
                sFrame = 0;
            }
            break;

        case 2: {
            f32 dx;
            f32 dz;
            f32 dist;

            if (sFrame == 0) {
                Inventory_ChangeEquipment(EQUIP_TYPE_BOOTS, (((sLeg % 2) == 0) && ((sLeg % 3) != 1))
                                                                ? EQUIP_VALUE_BOOTS_IRON
                                                                : EQUIP_VALUE_BOOTS_KOKIRI);
                Player_SetEquipmentData(play, player);
                VBench_Place(play, player, from->eye.x, from->eye.y, from->eye.z,
                             Math_Vec3f_Yaw(&from->eye, &to->eye));
                sFishing = false;
            }
            if (sFrame == 10) {
                sFishing = ((sLeg % 3) == 1) && !(player->stateFlags1 & PLAYER_STATE1_27) &&
                           (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND);
            }
            if (sFrame >= PB_SKIP) {
                sCounted = true;
            }
            dx = to->eye.x - player->actor.world.pos.x;
            dz = to->eye.z - player->actor.world.pos.z;
            dist = sqrtf(SQ(dx) + SQ(dz));
            if (sFishing && (sFrame > 10)) {
                if (sFrame < 16) {
                    VBench_Steer(play, Math_Atan2S(dz, dx), 20); // face the water, then stand (no rod here)
                }
            } else if (sFrame > 2) {
                if (dist > PB_ARRIVED) {
                    VBench_Steer(play, Math_Atan2S(dz, dx), 80);
                }
                if ((player->stateFlags1 & PLAYER_STATE1_27) && VBench_Under(to->title) && ((sTimer % 90) == 0)) {
                    sButton |= BTN_A;
                }
                if (++sStuckFrame >= 40) {
                    f32 moved = Math_Vec3f_DistXZ(&player->actor.world.pos, &sStuckPos);

                    if ((moved < 25.0f) && (dist > PB_ARRIVED * 2)) {
                        f32 step = 200.0f / dist;

                        VBench_Place(play, player, player->actor.world.pos.x + dx * step, player->actor.world.pos.y,
                                     player->actor.world.pos.z + dz * step, Math_Atan2S(dz, dx));
                    }
                    sStuckFrame = 0;
                    sStuckPos = player->actor.world.pos;
                }
            }
            if (++sFrame >= PB_SKIP + BENCH_FRAMES) {
                sState = 3;
                sFrame = 0;
            }
            break;
        }

        case 3:
            sFrame = 0;
            sState = (++sBlock < BENCH_MODES) ? 2 : 4;
            break;

        default:
            if (++sFrame >= BENCH_GAP_FRAMES) {
                sBlock = 0;
                sFrame = 0;
                if (++sLeg >= n) {
                    sLaps++;
                    sState = 0;
                } else {
                    sState = 2;
                }
            }
            break;
    }
}

/** Once a play frame (inside Play_Update, as the mod's debug update). */
void VBench_Update(PlayState* play) {
    Player* player = GET_PLAYER(play);

    VBench_FrameEnd(); // the frame before: its record (its sections are complete)
    sPlayFrames++;
    if (!sPlaced && (player != NULL) && (sPlayFrames > 20) && (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
        (play->transitionMode == TRANS_MODE_OFF)) {
        player->actor.world.pos = player->actor.prevPos = player->actor.home.pos = sScriptStartPos;
        player->actor.shape.rot.y = player->actor.world.rot.y = player->yaw = sScriptStartYaw;
        sPlaced = true;
        Rand_Seed(0x46495348); // (as the mod's scripted runs)
        PcProf_Start();
    }
    if (sPlaced && (player != NULL)) {
        VBench_Route(play, player);
    }
    PcProf_Update();
}

/** The bench's controller: what VBench_Route decided (port 1). */
void VBench_Input(Input* input) {
    u16 buttonDiff;

    if (!sActive) {
        return;
    }
    input->prev = input->cur;
    input->cur.button = sButton;
    input->cur.stick_x = sStickX;
    input->cur.stick_y = sStickY;
    buttonDiff = input->prev.button ^ input->cur.button;
    input->press.button = input->cur.button & buttonDiff;
    input->rel.button = input->prev.button & buttonDiff;
    PadUtils_UpdateRelXY(input);
    input->press.stick_x = 0;
    input->press.stick_y = 0;
}

/** One line of text (as the mod's profile builds show only their profiler's line). */
void VBench_DrawText(Gfx** gfxP) {
    GfxPrint printer;

    if (sTextLine[0] == '\0') {
        return;
    }
    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, *gfxP);
    GfxPrint_SetColor(&printer, 255, 255, 100, 255);
    GfxPrint_SetPos(&printer, 2, 23);
    GfxPrint_Printf(&printer, "%s", sTextLine);
    *gfxP = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);
}
