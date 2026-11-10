// Part of fishmod_debug.c (#included there, sharing its statics): the PC-sampling profiler (FISHMOD_PCPROF).
/*
 * With a SummerCart64 (found by its ID), the samples go to a PC over USB instead, one bucket per instruction and
 * nothing lost to the save's room: the PC runs `sc64deployer debug`, which saves each packet as a binaryout-*.bin
 * file; n64prof.py reads the folder. Without one (another cart, ares), the save as before.
 *
 * fish-oot's glue for n64prof (n64prof/, a game-independent sampler: see its README.md): libultra's timer interrupt
 * calls a hook (src/libultra/os/timerintr.c, debug builds) that gives the sampler the interrupted thread's PC; once a
 * frame the overlay tables say where each overlay is; every 30 seconds the hottest buckets go to SRAM (over the save
 * files: blank-save ROMs only). Read with n64prof/tools/n64prof.py (fishmod/tools/pcprof.sh finds the dump in a save).
 *
 * It runs the same on the console and in ares (the timer is the CPU's own Count/Compare), so the two can be compared.
 */
#ifdef FISHMOD_PCPROF
#include "z_game_dlftbls.h"
#include "kaleido_manager.h"
#include "effect.h"
#include "ultra64/osint.h"
#define N64PROF_BUCKET_SHIFT 5 // (32-byte buckets: pcprof1 lost 12.8% of its samples to the save's room)
#include "n64prof/n64prof.c"
#include "carthandle.h"

#define PCPROF_INTERVAL 11719 // count cycles between samples on average (46.875 MHz: about 4000 a second)
#define PCPROF_HASH 8192      // buckets in RAM (a power of two)
#define PCPROF_SRAM_SIZE (FISHMOD_SRAM_CRUMB - FISHMOD_SRAM_PCPROF)
#define PCPROF_SAVE_SECONDS 30

extern void (*gFishmodTimerHook)(OSTimer* timer);
extern u32 gFishmodCounterPc; // where the counter interrupt came in (exceptasm.s)

static N64ProfEntry* sPcTable;
static u8* sPcOut;
static OSTimer sPcTimer;

// --- SummerCart64 (its docs: docs/01_memory_map.md, docs/02_n64_commands.md in the SummerCart64 repository) ---
#define SC64_BUFFER 0x1FFE0000 // 8 KB data buffer; the registers are 64 KB above it
#define SC64_BUFFER_SIZE 0x2000
#define SC64_SCR 0x10000
#define SC64_DATA0 0x10004
#define SC64_DATA1 0x10008
#define SC64_IDENTIFIER 0x1000C
#define SC64_KEY 0x10010
#define SC64_CMD_BUSY (1u << 31)
#define SC64_CMD_ERROR (1u << 30)
#define SC64_USB_WRITE 'M'
#define SC64_USB_WRITE_STATUS 'U'
#define SC64_DATATYPE_RAW 2 // sc64deployer debug saves these as binaryout-*.bin

#define PCPROF_USB_HASH 16384           // buckets in each of the two tables (one counting, one being sent)
#define PCPROF_USB_SWAP_SECONDS 10      // send the counts at least this often
#define PCPROF_USB_SWAP_FILL (PCPROF_USB_HASH / 2) // or when the table is half full (the probes stay short)

static OSPiHandle sSc64Handle;
static OSIoMesg sSc64IoMesg;
static OSMesgQueue sSc64Queue;
static OSMesg sSc64Msg;
static s32 sPcUsb;               // 1: streaming to a SummerCart64's USB
static N64ProfEntry* sPcSpare;   // the next table to count into (cleared)
static N64ProfEntry* sPcSending; // the table being sent, or NULL
static u32 sPcSendPos;
static u32 sPcPackets;

static void Sc64_Write(u32 reg, u32 value) {
    osEPiWriteIo(&sSc64Handle, reg, value);
}

static u32 Sc64_Read(u32 reg) {
    u32 value = 0;

    osEPiReadIo(&sSc64Handle, reg, &value);
    return value;
}

/** Runs a command; 0 when it finished without an error (data0 then holds its first result). */
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

/** Is there a SummerCart64? Sets up its PI handle and unlocks its registers. */
static s32 Sc64_Init(void) {
    u32 prevInt;

    if (gCartHandle == NULL) {
        return 0;
    }
    sSc64Handle = *gCartHandle; // (the cart's own bus timings)
    sSc64Handle.baseAddress = (u32)OS_PHYSICAL_TO_K1(SC64_BUFFER);
    bzero(&sSc64Handle.transferInfo, sizeof(__OSTranxInfo));
    prevInt = __osDisableInt();
    sSc64Handle.next = __osPiTable;
    __osPiTable = &sSc64Handle;
    __osRestoreInt(prevInt);
    osCreateMesgQueue(&sSc64Queue, &sSc64Msg, 1);

    Sc64_Write(SC64_KEY, 0x5F554E4C); // "_UNL"
    Sc64_Write(SC64_KEY, 0x4F434B5F); // "OCK_"
    return Sc64_Read(SC64_IDENTIFIER) == 0x53437632; // "SCv2"
}

/** Still sending the last packet? */
static s32 Sc64_UsbBusy(void) {
    u32 status;

    if (Sc64_Command(SC64_USB_WRITE_STATUS, 0, 0, &status) != 0) {
        return 1;
    }
    return (status & SC64_CMD_BUSY) != 0;
}

/** Copies len bytes (8-aligned) into the cart's buffer and sends them to the PC. */
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
    return Sc64_Command(SC64_USB_WRITE, SC64_BUFFER, (SC64_DATATYPE_RAW << 24) | len, NULL);
}

/**
 * The timer hook: one sample. libultra runs its timers in the VI manager's thread after the counter interrupt has
 * posted a message, so the PC is the one the exception handler saw (exceptasm.s keeps it), not the running thread's.
 */
static void PcProf_Hook(OSTimer* timer) {
    if (timer == &sPcTimer) {
        n64prof_sample(gFishmodCounterPc);
        timer->interval = n64prof_next_interval();
    }
}

/** Where each loaded overlay is (once a frame, from the game thread). */
static void PcProf_Ranges(void) {
    N64ProfRange* r = n64prof_ranges_begin();
    s32 n = 0;
    s32 i;

#define PCPROF_ADD(ram, vs, ve)                                                          \
    if (((ram) != NULL) && ((vs) != NULL) && (n < N64PROF_MAX_RANGES)) {                  \
        r[n].ramStart = (u32)(ram);                                                       \
        r[n].ramEnd = (u32)(ram) + ((u32)(ve) - (u32)(vs));                               \
        r[n].vramStart = (u32)(vs);                                                       \
        n++;                                                                              \
    }
    for (i = 0; i < ACTOR_ID_MAX; i++) {
        PCPROF_ADD(gActorOverlayTable[i].loadedRamAddr, gActorOverlayTable[i].vramStart,
                   gActorOverlayTable[i].vramEnd);
    }
    for (i = 0; i < EFFECT_SS_TYPE_MAX; i++) {
        PCPROF_ADD(gEffectSsOverlayTable[i].loadedRamAddr, gEffectSsOverlayTable[i].vramStart,
                   gEffectSsOverlayTable[i].vramEnd);
    }
    for (i = 0; i < GAMESTATE_ID_MAX; i++) {
        PCPROF_ADD(gGameStateOverlayTable[i].loadedRamAddr, gGameStateOverlayTable[i].vramStart,
                   gGameStateOverlayTable[i].vramEnd);
    }
    for (i = 0; i < KALEIDO_OVL_MAX; i++) {
        PCPROF_ADD(gKaleidoMgrOverlayTable[i].loadedRamAddr, gKaleidoMgrOverlayTable[i].vramStart,
                   gKaleidoMgrOverlayTable[i].vramEnd);
    }
#undef PCPROF_ADD
    n64prof_ranges_end(n);
}

/** Once a play frame: start the sampler, keep the overlay ranges current, save every 30 seconds. */
static void PcProf_Update(void) {
    static OSTime sLastSave;
    OSTime now = osGetTime();

    if (sPcTable == NULL) {
        s32 hash = PCPROF_HASH;
        u32 outSize = PCPROF_SRAM_SIZE;

        sPcUsb = Sc64_Init();
        if (sPcUsb) {
            hash = PCPROF_USB_HASH;
            outSize = SC64_BUFFER_SIZE;
            sPcSpare = SYSTEM_ARENA_MALLOC(hash * sizeof(N64ProfEntry), "../fishmod_debug.c", __LINE__);
            if (sPcSpare == NULL) {
                sPcUsb = 0;
                hash = PCPROF_HASH;
                outSize = PCPROF_SRAM_SIZE;
            }
        }
        sPcTable = SYSTEM_ARENA_MALLOC(hash * sizeof(N64ProfEntry), "../fishmod_debug.c", __LINE__);
        sPcOut = SYSTEM_ARENA_MALLOC(outSize, "../fishmod_debug.c", __LINE__);
        if ((sPcTable == NULL) || (sPcOut == NULL)) {
            sPcTable = NULL;
            Fishmod_DebugPrintf(5, "PCPROF: no memory");
            return;
        }
        if (sPcUsb) {
            n64prof_set_bucket_shift(2); // one bucket per instruction: the PC has room for all of them
        }
        n64prof_init(sPcTable, hash, PCPROF_INTERVAL, (u32)_codeSegmentEnd);
        if (sPcUsb) {
            n64prof_clear(sPcSpare);
        }
        PcProf_Ranges();
        gFishmodTimerHook = PcProf_Hook;
        osSetTimer(&sPcTimer, PCPROF_INTERVAL, PCPROF_INTERVAL, NULL, NULL);
        sLastSave = now;
        return;
    }
    // (every 8 frames: every frame cost 0.8% in pcprof1; an overlay loaded since is unmapped for a few frames)
    if ((sPlayFrames & 7) == 0) {
        PcProf_Ranges();
    }
    if (sPcUsb) {
        u32 user[6] = { 0x464D4F44 /* 'FMOD' */, 0, 0, 0, 0, 0 };

        user[1] = sPlayFrames;
        user[2] = FISHMOD_PROFILE_HW;
        // swap tables every few seconds, or sooner if the one counting fills up
        if ((sPcSending == NULL) &&
            ((n64prof_used() >= PCPROF_USB_SWAP_FILL) ||
             (OS_CYCLES_TO_USEC(now - sLastSave) > PCPROF_USB_SWAP_SECONDS * 1000000ULL))) {
            sPcSending = n64prof_swap(sPcSpare);
            sPcSpare = NULL;
            sPcSendPos = 0;
            sLastSave = now;
        }
        // one packet a frame, when the last one has gone (never waits: with no PC listening the counts wait, and
        // the table counting drops samples once full)
        if ((sPcSending != NULL) && !Sc64_UsbBusy()) {
            u32 bytes = n64prof_pack(sPcSending, &sPcSendPos, sPcOut, SC64_BUFFER_SIZE, user);

            if (Sc64_UsbSend(sPcOut, bytes) == 0) {
                sPcPackets++;
            }
            if (n64prof_pack_done(sPcSendPos)) {
                n64prof_clear(sPcSending);
                sPcSpare = sPcSending;
                sPcSending = NULL;
                Fishmod_DebugPrintf(5, "pcprof usb: %u packets", sPcPackets);
            }
        }
        return;
    }
    if (OS_CYCLES_TO_USEC(now - sLastSave) > PCPROF_SAVE_SECONDS * 1000000ULL) {
        u32 user[6] = { 0x464D4F44 /* 'FMOD' */, 0, 0, 0, 0, 0 };
        u32 bytes;

        user[1] = sPlayFrames;
        user[2] = FISHMOD_PROFILE_HW;
        sLastSave = now;
        bytes = n64prof_serialize(sPcOut, PCPROF_SRAM_SIZE, user);
        if (bytes != 0) {
            N64ProfHeader* h = (N64ProfHeader*)sPcOut;

            SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_PCPROF), sPcOut, bytes, OS_WRITE);
            // (once a save: the debug text's formatting every frame cost about 2% in pcprof1)
            Fishmod_DebugPrintf(5, "pcprof dump %u: %u samples, %u buckets", h->dumps, h->total, h->entries);
        }
    }
}
#endif
