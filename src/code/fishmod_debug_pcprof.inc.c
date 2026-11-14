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
static s32 sPcStopped; // the route is over: the last counts sent, then nothing

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
#define SC64_DATATYPE_TEXT 1 // sc64deployer debug prints these
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
// a command the cart doesn't finish in SC64_TRIES polls is given up on (a few ms, not the game's frames): counted, and
// the USB sending backs off a while (see PcProf_Update), so a stuck link can't slow the game to a crawl
#define SC64_TRIES 3000
static u32 sUsbTimeouts;     // commands given up on since the last report
static u32 sUsbTimeoutFrame; // the play frame of the first of them
static u32 sUsbTimeoutScr;   // the cart's status register then
static s32 sUsbBackoff;      // frames to leave the USB alone

static s32 Sc64_Command(u8 id, u32 arg0, u32 arg1, u32* data0) {
    s32 tries;

    Sc64_Write(SC64_DATA0, arg0);
    Sc64_Write(SC64_DATA1, arg1);
    Sc64_Write(SC64_SCR, id);
    for (tries = 0; tries < SC64_TRIES; tries++) {
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
        if (tries == SC64_TRIES - 1) {
            if (sUsbTimeouts == 0) {
                sUsbTimeoutFrame = sPlayFrames;
                sUsbTimeoutScr = scr;
            }
            sUsbTimeouts++;
            sUsbBackoff = 30;
        }
    }
    return -1;
}

/** Is there a SummerCart64? Sets up its PI handle and unlocks its registers. */
static s32 Sc64_Init(void) {
    u32 prevInt;

#ifdef FISHMOD_FAKE_SC64
    return 1; // (a test in an emulator: the USB path all but the transfers, which go nowhere)
#endif

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

#ifdef FISHMOD_FAKE_SC64
    return 0;
#endif

    if (Sc64_Command(SC64_USB_WRITE_STATUS, 0, 0, &status) != 0) {
        return 1;
    }
    return (status & SC64_CMD_BUSY) != 0;
}

/** Copies len bytes (8-aligned) into the cart's buffer and sends them to the PC. */
static s32 Sc64_UsbSend(void* buf, u32 len) {
    OSMesg msg;

#ifdef FISHMOD_FAKE_SC64
    return 0;
#endif

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

/** Sends a line of text: `sc64deployer debug` prints it (the morning script watches for its lines). */
static void Sc64_UsbText(const char* text) {
    u32 len = strlen(text);

#ifdef FISHMOD_FAKE_SC64
    return;
#endif
    if (!sPcUsb || (sPcOut == NULL) || (len >= SC64_BUFFER_SIZE)) {
        return;
    }
    {
        s32 wait;

        for (wait = 0; Sc64_UsbBusy(); wait++) {
            if (wait > 200) {
                return; // (the link is stuck: dropped, not waited on)
            }
        }
    }
    bcopy(text, sPcOut, len);
    osWritebackDCache(sPcOut, (len + 7) & ~7);
    sSc64IoMesg.hdr.pri = OS_MESG_PRI_NORMAL;
    sSc64IoMesg.hdr.retQueue = &sSc64Queue;
    sSc64IoMesg.dramAddr = sPcOut;
    sSc64IoMesg.devAddr = 0;
    sSc64IoMesg.size = (len + 7) & ~7;
    {
        OSMesg msg;

        osEPiStartDma(&sSc64Handle, &sSc64IoMesg, OS_WRITE);
        osRecvMesg(&sSc64Queue, &msg, OS_MESG_BLOCK);
    }
    Sc64_Command(SC64_USB_WRITE, SC64_BUFFER, (SC64_DATATYPE_TEXT << 24) | len, NULL);
}

/*
 * `sc64deployer upload --reboot`: the PC loads the next ROM into a running game. It sends Halt over the cart's AUX
 * register (the game answers with the same word, then stops touching the cart), uploads the ROM, then sends Reboot (the
 * game answers, then starts the new ROM's boot code as the console's boot ROM would after a reset). fish-oot's own code
 * from the SummerCart64's register docs (docs/01_memory_map.md) and the console's documented boot state.
 */
#define SC64_IRQ 0x10014
#define SC64_AUX 0x10018
#define SC64_AUX_PENDING (1u << 23) // (SCR)
#define SC64_AUX_CLEAR (1u << 28)   // (IRQ)
#define SC64_AUX_HALT 0xFF000001
#define SC64_AUX_REBOOT 0xFF000002

// with interrupts off, the PI by hand (osEPi* could wait on a thread that will never run again)
static u32 Sc64Raw_Read(u32 addr) {
    while (IO_READ(PI_STATUS_REG) & (PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY)) {}
    return *(vu32*)OS_PHYSICAL_TO_K1(addr);
}

static void Sc64Raw_Write(u32 addr, u32 value) {
    while (IO_READ(PI_STATUS_REG) & (PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY)) {}
    *(vu32*)OS_PHYSICAL_TO_K1(addr) = value;
}

#ifdef __GNUC__
/** Starts the cart's boot code (its IPL3) afresh, as after the reset button: never returns. Interrupts are off. */
static void Fishmod_Reboot(void) {
    u32 cic = osCicId;
    u32 tvType = osTvType & 3;
    u32 seed = (cic == 6103) ? 0x78 : (cic == 6105) ? 0x91 : (cic == 6106) ? 0x85 : 0x3F;
    u32 version = (tvType == OS_TV_PAL) ? 6 : (tvType == OS_TV_MPAL) ? 4 : 1;
    u32 header;
    s32 i;

    // the RSP stopped, its flags clear; the RDP left to finish what it has
    IO_WRITE(SP_STATUS_REG, SP_SET_HALT);
    while (!(IO_READ(SP_STATUS_REG) & SP_STATUS_HALT)) {}
    while (IO_READ(SP_DMA_BUSY_REG)) {}
    IO_WRITE(SP_STATUS_REG, SP_CLR_INTR | SP_CLR_SSTEP | SP_CLR_INTR_BREAK | SP_CLR_BROKE | SP_CLR_SIG0 | SP_CLR_SIG1 |
                                SP_CLR_SIG2 | SP_CLR_SIG3 | SP_CLR_SIG4 | SP_CLR_SIG5 | SP_CLR_SIG6 | SP_CLR_SIG7);
    IO_WRITE(SP_SEMAPHORE_REG, 0);
    IO_WRITE(SP_PC_REG, 0);
    for (i = 0; (i < 1000000) && (IO_READ(DPC_STATUS_REG) & DPC_STATUS_PIPE_BUSY); i++) {}

    // no interrupt from anything: audio and the other interfaces quiet, the picture blank from the top
    IO_WRITE(MI_INTR_MASK_REG, MI_INTR_MASK_CLR_SP | MI_INTR_MASK_CLR_SI | MI_INTR_MASK_CLR_AI | MI_INTR_MASK_CLR_VI |
                                   MI_INTR_MASK_CLR_PI | MI_INTR_MASK_CLR_DP);
    IO_WRITE(AI_CONTROL_REG, 0);
    IO_WRITE(AI_DRAM_ADDR_REG, 0);
    IO_WRITE(AI_LEN_REG, 0);
    while (IO_READ(VI_CURRENT_REG) > 1) {}
    IO_WRITE(VI_INTR_REG, 0x3FF);
    IO_WRITE(VI_H_START_REG, 0);
    IO_WRITE(VI_CURRENT_REG, 0);
    IO_WRITE(PI_STATUS_REG, PI_STATUS_RESET | PI_STATUS_CLR_INTR);
    IO_WRITE(SI_STATUS_REG, 0);

    // the cart's bus timings from its header (read at the slowest), as the boot ROM sets them
    IO_WRITE(PI_BSD_DOM1_LAT_REG, 0xFF);
    IO_WRITE(PI_BSD_DOM1_PWD_REG, 0xFF);
    IO_WRITE(PI_BSD_DOM1_PGS_REG, 0x0F);
    IO_WRITE(PI_BSD_DOM1_RLS_REG, 0x03);
    header = Sc64Raw_Read(0x10000000);
    IO_WRITE(PI_BSD_DOM1_LAT_REG, header & 0xFF);
    IO_WRITE(PI_BSD_DOM1_PWD_REG, (header >> 8) & 0xFF);
    IO_WRITE(PI_BSD_DOM1_PGS_REG, (header >> 16) & 0x0F);
    IO_WRITE(PI_BSD_DOM1_RLS_REG, (header >> 20) & 0x03);

    // its boot code (ROM 0x40-0xFFF) into the RSP's data memory, where the boot ROM puts it
    for (i = 0x40; i < 0x1000; i += 4) {
        *(vu32*)(0xA4000000 + i) = Sc64Raw_Read(0x10000000 + i);
    }

    {
    register u32 rBootDevice __asm__("s3") = osRomType & 1; // (0: the cart)
    register u32 rTvType __asm__("s4") = tvType;
    register u32 rResetType __asm__("s5") = 1; // a warm start (memory already set up, as after the reset button)
    register u32 rSeed __asm__("s6") = seed;   // the CIC's seed
    register u32 rVersion __asm__("s7") = version; // the boot ROM's version

    // then: the caches written back and emptied (the new game is copied in under them), the boot ROM's registers on
    // leaving (the boot code's checks read some of them), and in
    __asm__ volatile(".set push\n"
                     ".set noreorder\n"
                     ".set noat\n"
                     "    li    $t0, 0x34000000\n" // Status: coprocessors 0 and 1 usable, 64-bit FPU, interrupts off
                     "    mtc0  $t0, $12\n"
                     "    nop\n"
                     "    lui   $t0, 0x8000\n"
                     "    addiu $t1, $t0, 0x2000\n"
                     "1:  cache 0x01, 0($t0)\n" // D-cache: index write back and invalidate
                     "    addiu $t0, $t0, 16\n"
                     "    bne   $t0, $t1, 1b\n"
                     "    nop\n"
                     "    lui   $t0, 0x8000\n"
                     "    addiu $t1, $t0, 0x4000\n"
                     "2:  cache 0x00, 0($t0)\n" // I-cache: index invalidate
                     "    addiu $t0, $t0, 32\n"
                     "    bne   $t0, $t1, 2b\n"
                     "    nop\n"
                     "    lui   $t5, 0xBFC0\n" // the PIF: wait until it isn't busy (bit 7 of its last byte); read
                     "3:  lw    $t0, 0x7FC($t5)\n" // its word 0x24 last, as the boot ROM does before leaving
                     "    andi  $t0, $t0, 0x80\n"
                     "    bnez  $t0, 3b\n"
                     "    nop\n"
                     "    lw    $t0, 0x7E4($t5)\n"
                     "    lui   $sp, 0xA400\n"
                     "    ori   $sp, $sp, 0x1FF0\n"
                     "    lui   $ra, 0xA400\n"
                     "    li    $t1, 1\n"
                     "    beq   $s4, $t1, 4f\n"
                     "    ori   $ra, $ra, 0x1550\n" // (NTSC: 0xA4001550; the others 0xA4001554)
                     "    ori   $ra, $ra, 0x1554\n"
                     "4:  move  $at, $zero\n"
                     "    move  $v0, $zero\n"
                     "    move  $v1, $zero\n"
                     "    move  $a0, $zero\n"
                     "    move  $a1, $zero\n"
                     "    move  $a2, $zero\n"
                     "    move  $a3, $zero\n"
                     "    move  $t0, $zero\n"
                     "    move  $t1, $zero\n"
                     "    li    $t2, 0x40\n"
                     "    move  $t4, $zero\n"
                     "    move  $t5, $zero\n"
                     "    move  $t6, $zero\n"
                     "    move  $t7, $zero\n"
                     "    move  $s0, $zero\n"
                     "    move  $s1, $zero\n"
                     "    move  $s2, $zero\n"
                     "    move  $t8, $zero\n"
                     "    move  $t9, $zero\n"
                     "    move  $k0, $zero\n"
                     "    move  $k1, $zero\n"
                     "    move  $gp, $zero\n"
                     "    move  $fp, $zero\n"
                     "    lui   $t3, 0xA400\n"
                     "    ori   $t3, $t3, 0x0040\n"
                     "    jr    $t3\n"
                     "    nop\n"
                     ".set pop\n"
                     :
                     : "r"(rBootDevice), "r"(rTvType), "r"(rResetType), "r"(rSeed), "r"(rVersion)
                     : "memory");
    }
    for (;;) {}
}
#endif

/** Halt: answered, then nothing touches the cart again (interrupts off: no thread runs) until Reboot comes. */
static void Sc64_Halt(void) {
    __osDisableInt();
    for (;;) {
        u32 scr;

        Sc64Raw_Write(SC64_BUFFER + SC64_KEY, 0x5F554E4C); // (unlocked again: the upload may have reset the cart)
        Sc64Raw_Write(SC64_BUFFER + SC64_KEY, 0x4F434B5F);
        scr = Sc64Raw_Read(SC64_BUFFER + SC64_SCR);
        if (scr & SC64_AUX_PENDING) {
            u32 msg = Sc64Raw_Read(SC64_BUFFER + SC64_AUX);

            Sc64Raw_Write(SC64_BUFFER + SC64_IRQ, SC64_AUX_CLEAR);
            Sc64Raw_Write(SC64_BUFFER + SC64_AUX, msg);
#ifdef __GNUC__
            if (msg == SC64_AUX_REBOOT) {
                Fishmod_Reboot();
            }
#endif
        }
    }
}

/** Once a frame: anything from the PC on the AUX channel? (Ping is answered; Halt and Reboot as above.) */
static void Sc64_AuxPoll(void) {
    u32 msg;

#ifdef FISHMOD_FAKE_SC64
    return;
#endif
    if (!(Sc64_Read(SC64_SCR) & SC64_AUX_PENDING)) {
        return;
    }
    msg = Sc64_Read(SC64_AUX);
    Sc64_Write(SC64_IRQ, SC64_AUX_CLEAR);
    Sc64_Write(SC64_AUX, msg);
    if ((msg == SC64_AUX_HALT) || (msg == SC64_AUX_REBOOT)) {
        __osDisableInt();
#ifdef __GNUC__
        if (msg == SC64_AUX_REBOOT) {
            Fishmod_Reboot();
        }
#endif
        Sc64_Halt();
    }
}

/*
 * The frame log (FISHMOD_FRAMELOG, over the SummerCart64's USB with the profile): a record of every frame, so a slow
 * one can be told apart and explained. 52 words a record (FRAMELOG_WORDS; fishmod/tools/framelog.py reads them):
 *   0 the frame (play frames so far)          1 the route: leg << 24 | lap << 16 | conditions << 8 | counted
 *   2 the frame period, us                    3, 4 the CPU's update and draw, us
 *   5, 6 the RSP's graphics and audio, us     7 the RDP, us
 *   8-11 the RDP's counters: clock, command buffer busy, pipeline busy, TMEM (cycles at 62.5 MHz)
 *   12, 13 the fish's update and draw, us     14 the props' drawing, us; 15 props drawn
 *   16-35 Play_Update's sections (FishmodSect), us    36-47 the actor categories' updates, us
 *   48-51 the four costliest actor kinds this frame: id << 20 | us
 *   52-55 (version 2) F3DEX3's counters for the frame's graphics task (sched.c): vertices << 16 | tris drawn;
 *   tris in << 14 | rectangles; cycles stalled on a full RDP FIFO; 0
 * 39 records a packet ('FFRM', version, count, the first record's frame). The profile's samples are tagged by their
 * frame as they're counted: 1 for a frame over 52 ms, 2 otherwise (n64prof_frame_end; n64prof.py --frames).
 */
#ifdef FISHMOD_FRAMELOG
#define FRAMELOG_WORDS 60 // (56, 57: the pictures shown since the last record, and how long: version bit 16; 58, 59: the
                          // press-to-picture latency: version bit 17)
#define FRAMELOG_PER_PACKET ((SC64_BUFFER_SIZE - 16) / (FRAMELOG_WORDS * 4))
#define PCPROF_RING 1024 // samples a frame can hold (about 290 ms at 3,500 a second)
#define FRAMELOG_SLOW_US 52000

typedef struct FrameLogPacket {
    u32 magic; // 'FFRM'
    u32 version;
    u32 count;
    u32 first;
    u32 rec[FRAMELOG_PER_PACKET][FRAMELOG_WORDS];
} FrameLogPacket;

typedef char FrameLogSectsFit[(16 + FISHMOD_SECT_MAX <= 36) && (36 + FISHMOD_ACTOR_CATS <= 48) ? 1 : -1];

static FrameLogPacket* sFl[2];
static s32 sFlCur;
static s32 sFlReady = -1; // the packet waiting to be sent, or -1
static u32 sFlLost;       // records dropped (both packets full: the USB fell behind)
#endif

#ifdef FISHMOD_FRAMELOG
// each actor kind over the whole run: its updates (CPU count cycles) and how many of it updated, frames added up
static u64 sActTotal[FISHMOD_ACTOR_IDS];
static u32 sActInst[FISHMOD_ACTOR_IDS];

/**
 * The run's actor report, at its end (before FISHMOD-DONE): the kinds whose collision checks (Actor_UpdateBgCheckInfo)
 * and whose updates cost the most, with how many calls and instances. Text lines in the debug log:
 *   FISHMOD-BGCHECK id calls us        FISHMOD-ACTOR id instance-frames us
 */
static void PcProf_ActorReport(void) {
    static u8 sTaken[FISHMOD_ACTOR_IDS];
    char line[96];
    s32 n;
    s32 i;

    bzero(sTaken, sizeof(sTaken));
    for (n = 0; n < 12; n++) {
        s32 best = -1;

        for (i = 0; i < FISHMOD_ACTOR_IDS; i++) {
            if (!sTaken[i] && (gFishmodBgCycles[i] != 0) && ((best < 0) || (gFishmodBgCycles[i] > gFishmodBgCycles[best]))) {
                best = i;
            }
        }
        if (best < 0) {
            break;
        }
        sTaken[best] = true;
        sprintf(line, "FISHMOD-BGCHECK %d %u %u\n", best, gFishmodBgCalls[best],
                (u32)OS_CYCLES_TO_USEC(gFishmodBgCycles[best]));
        Sc64_UsbText(line);
    }
    bzero(sTaken, sizeof(sTaken));
    for (n = 0; n < 16; n++) {
        s32 best = -1;

        for (i = 0; i < FISHMOD_ACTOR_IDS; i++) {
            if (!sTaken[i] && (sActTotal[i] != 0) && ((best < 0) || (sActTotal[i] > sActTotal[best]))) {
                best = i;
            }
        }
        if (best < 0) {
            break;
        }
        sTaken[best] = true;
        sprintf(line, "FISHMOD-ACTOR %d %u %u\n", best, sActInst[best], (u32)OS_CYCLES_TO_USEC(sActTotal[best]));
        Sc64_UsbText(line);
    }
}
#endif

/** The frame just ended: its samples tagged, and its record logged (USB only). */
static void PcProf_FrameEnd(void) {
#ifdef FISHMOD_FRAMELOG
    u32 period = (u32)OS_CYCLES_TO_USEC(gGraphUpdatePeriod);
    FrameLogPacket* pk;
    u32* r;
    u32 top[4] = { 0, 0, 0, 0 };
    s32 i;
    s32 j;

    if (!sPcUsb || (sFl[0] == NULL)) {
        return;
    }
    for (i = 0; i < FISHMOD_ACTOR_IDS; i++) {
        if (gFishmodActorCount[i] != 0) {
            sActTotal[i] += gFishmodActorCycles[i];
            sActInst[i] += gFishmodActorCount[i];
        }
    }
    n64prof_frame_end((period > FRAMELOG_SLOW_US) ? 1 : 2);
    pk = sFl[sFlCur];
    if (pk->count >= FRAMELOG_PER_PACKET) {
        if (sFlReady >= 0) {
            sFlLost++;
            return; // (both full: this frame's record is lost)
        }
        sFlReady = sFlCur;
        sFlCur ^= 1;
        pk = sFl[sFlCur];
        pk->count = 0;
    }
    if (pk->count == 0) {
        pk->magic = 0x4646524D; // 'FFRM'
#if defined(F3DEX3_ALL)
        pk->version = 3 | 0x30000; // (58 words; each record's set of F3DEX3's counters in r[1] bits 1-2)
#elif defined(F3DEX3_PROF)
        pk->version = 2 | (F3DEX3_PROF << 8) | 0x30000; // (58 words: 52-55 F3DEX3's counters; which set: 1-3 A-C)
#else
        pk->version = 2 | 0x30000; // (58 words a record: 52-55 F3DEX3's counters, the default set; 56-57 the screen)
#endif
        pk->first = sPlayFrames;
    }
    r = pk->rec[pk->count++];
    bzero(r, FRAMELOG_WORDS * 4);
    r[0] = sPlayFrames;
#ifdef FISHMOD_BENCH_PLAY
    r[1] = ((u32)(sPbLeg & 0xFF) << 24) | ((u32)(sBench.laps & 0xFF) << 16) | ((u32)(sPbCond & 0xFF) << 8) |
           (sBenchCounted ? 1 : 0); // (bit 0: a frame the bench counted, inside a block)
    sBenchCounted = false;
#elif defined(FISHMOD_VIEW_BENCH)
    r[1] = sViewTag; // (stop << 24 | lap << 16 | variant << 8 | bit 0: counted)
#elif defined(FISHMOD_RDP_BENCH)
    r[1] = sRdpTag; // (test << 24 | repetition << 16 | bit 0: counted)
#elif defined(FISHMOD_STRESS)
    r[1] = sStressTag; // (bench << 24 | step << 16 | pass << 8 | bit 0: measured)
#endif
    r[2] = period;
    r[3] = sLastCpu[FISHMOD_CPU_UPDATE];
    r[4] = sLastCpu[FISHMOD_CPU_DRAW];
    r[5] = (u32)OS_CYCLES_TO_USEC(gRSPGfxTimeTotal);
    r[6] = (u32)OS_CYCLES_TO_USEC(gRSPAudioTimeTotal);
    r[7] = (u32)OS_CYCLES_TO_USEC(gRDPTimeTotal);
    r[8] = gFishmodRdp.clock;
    r[9] = gFishmodRdp.buf;
    r[10] = gFishmodRdp.pipe;
    r[11] = gFishmodRdp.tmem;
    r[12] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_U_ENGINE]);
    r[13] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_ENGINE]);
    r[14] = (u32)OS_CYCLES_TO_USEC(gFishmodLastProf[FISHMOD_PROF_D_SCATTER]);
    r[15] = gFishmodScatterDrawn;
    {
        extern u32 gFishmodRspCounters[4];

        r[52] = gFishmodRspCounters[0];
        r[53] = gFishmodRspCounters[1];
        r[54] = gFishmodRspCounters[2];
        r[55] = gFishmodRspCounters[3];
    }
    {
        // the pictures the scheduler put on screen since the last record (sched.c): r[56] = how many << 16 | the most
        // refreshes one stayed; r[57] = refreshes in all (3 a picture at 20 fps; a hitch holds one for 4 or more)
        extern u32 gFishmodShown[6];
        u32 prevInt = __osDisableInt();

        r[56] = (gFishmodShown[0] << 16) | (gFishmodShown[1] & 0xFFFF);
        r[57] = gFishmodShown[2];
        gFishmodShown[0] = gFishmodShown[1] = gFishmodShown[2] = 0;
        {
            // press to picture since the last record (sched.c): r[58] the mean (us) << 0, r[59] the most (ms) << 16 |
            // pictures that waited behind another << 8 | pictures (version bit 17)
            extern u32 gFishmodLatency[4];
            u32 n = gFishmodLatency[2];

            r[58] = (n != 0) ? gFishmodLatency[0] / n : 0;
            r[59] = (MIN(gFishmodLatency[1] / 1000, 0xFFFF) << 16) | (MIN(gFishmodLatency[3], 0xFF) << 8) | MIN(n, 0xFF);
            gFishmodLatency[0] = gFishmodLatency[1] = gFishmodLatency[2] = gFishmodLatency[3] = 0;
        }
        __osRestoreInt(prevInt);
#ifdef F3DEX3_ALL
        {
            extern u32 gFishmodRspCountersSet;

            r[1] |= (gFishmodRspCountersSet & 3) << 1; // (version 3: the counters' set, per record)
        }
#endif
    }
    for (i = 0; i < FISHMOD_SECT_MAX; i++) {
        r[16 + i] = (u32)OS_CYCLES_TO_USEC(gFishmodSect[i]);
    }
    for (i = 0; i < FISHMOD_ACTOR_CATS; i++) {
        r[36 + i] = (u32)OS_CYCLES_TO_USEC((OSTime)gFishmodCatCycles[i]);
    }
    // the four costliest actor kinds (an insertion into four slots, cycles << 0 kept alongside the id)
    for (i = 0; i < FISHMOD_ACTOR_IDS; i++) {
        u32 c = gFishmodActorCycles[i];

        if (c > (top[3] >> 0)) {
            for (j = 3; (j > 0) && (c > top[j - 1]); j--) {
                top[j] = top[j - 1];
                r[48 + j] = r[48 + j - 1];
            }
            top[j] = c;
            r[48 + j] = ((u32)i << 20) | MIN((u32)OS_CYCLES_TO_USEC((OSTime)c), 0xFFFFF);
        }
    }
#endif
}

/**
 * The timer hook: one sample. libultra runs its timers in the VI manager's thread after the counter interrupt has
 * posted a message, so the PC is the one the exception handler saw (exceptasm.s keeps it), not the running thread's.
 */
static void PcProf_Hook(OSTimer* timer) {
    if (timer == &sPcTimer) {
        extern u32 gFishmodCounterRa;
        u32 pc = gFishmodCounterPc;

        // a sample in __osRestoreInt (a leaf: mtc0, jr ra) is the time its caller spent with interrupts off (the timer's
        // interrupt waited for it): counted at the call site instead (its return address), so it shows who
        if ((pc - (u32)__osRestoreInt) < 0x20) {
            pc = gFishmodCounterRa - 8; // (the jal)
        }
        n64prof_sample(pc);
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
#ifdef FISHMOD_PCPROF_CALIB
/*
 * Calibration (FISHMOD_PCPROF_CALIB): five loops with a known answer, run once with the sampler going, so the console
 * shows how each kind of waiting lands in the profile (fishmod/tools/inst_profile.py --func PcCalib_...): about a
 * third of a second each, timed by the count register.
 *   PcCalib_Alu        adds in a loop that fits the instruction cache: no waiting (the baseline mix)
 *   PcCalib_LoadHit    loads from 1 KB, always in the data cache
 *   PcCalib_LoadMiss   loads every 32 bytes over 3 MB of RDRAM: every one a data-cache miss
 *   PcCalib_FetchMiss  32 KB of straight-line code, twice the instruction cache: every line fetched from RDRAM
 *   PcCalib_Div        float divisions back to back (29 cycles each)
 */
#define CALIB_COUNTS (46875000 / 3) // a third of a second at the count register's rate
#define NO_INLINE __attribute__((noinline))
static volatile u32 sCalibSink;
static u32 sCalibHot[256];

static NO_INLINE void PcCalib_Alu(void) {
    u32 start = osGetCount();

    while ((osGetCount() - start) < CALIB_COUNTS) {
        __asm__ volatile(".set noreorder\n.rept 256\naddiu $t0, $t0, 1\n.endr\n.set reorder" ::: "t0");
    }
}

static NO_INLINE void PcCalib_LoadHit(void) {
    u32 start = osGetCount();
    volatile u32* p = sCalibHot;
    u32 sum = 0;
    s32 i;

    while ((osGetCount() - start) < CALIB_COUNTS) {
        for (i = 0; i < 256; i++) {
            sum += p[i];
        }
    }
    sCalibSink = sum;
}

static NO_INLINE void PcCalib_LoadMiss(void) {
    u32 start = osGetCount();
    volatile u32* p = (volatile u32*)0x80000400; // (the game's own code and data, only read: 3 MB, 375 times the cache)
    u32 sum = 0;
    s32 i;

    while ((osGetCount() - start) < CALIB_COUNTS) {
        for (i = 0; i < (3 << 20) / 4; i += 8) {
            sum += p[i];
        }
    }
    sCalibSink = sum;
}

static NO_INLINE void PcCalib_FetchMiss(void) {
    u32 start = osGetCount();

    while ((osGetCount() - start) < CALIB_COUNTS) {
        __asm__ volatile(".set noreorder\n.rept 8192\naddiu $t0, $t0, 1\n.endr\n.set reorder" ::: "t0");
    }
}

static NO_INLINE void PcCalib_Div(void) {
    u32 start = osGetCount();
    volatile f32 x = 1.0f;
    f32 y = 1.0001f;
    f32 v = x;

    // (in assembly: -ffast-math would turn a division by the same number into a multiply)
    while ((osGetCount() - start) < CALIB_COUNTS) {
        __asm__ volatile(".rept 64\ndiv.s %0, %0, %1\n.endr" : "+f"(v) : "f"(y));
    }
    x = v;
}

static void PcProf_Calibrate(void) {
    PcCalib_Alu();
    PcCalib_LoadHit();
    PcCalib_LoadMiss();
    PcCalib_FetchMiss();
    PcCalib_Div();
    Sc64_UsbText("FISHMOD-CALIB done\n");
}
#endif

static void PcProf_Update(void) {
    static OSTime sLastSave;
    OSTime now = osGetTime();

    if (sPcTable == NULL) {
        s32 hash = PCPROF_HASH;
#ifdef FISHMOD_NO_SAMPLE_STORE
        {
            // (a console test without the audio's sample store: no more samples copied or switched to it)
            extern u32 gFishmodSampleStoreStart;

            gFishmodSampleStoreStart = 0;
        }
#endif
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
#ifdef FISHMOD_FRAMELOG
            {
                u32* rings = SYSTEM_ARENA_MALLOC(2 * PCPROF_RING * sizeof(u32), "../fishmod_debug.c", __LINE__);

                sFl[0] = SYSTEM_ARENA_MALLOC(2 * sizeof(FrameLogPacket), "../fishmod_debug.c", __LINE__);
                if ((rings != NULL) && (sFl[0] != NULL)) {
                    sFl[1] = sFl[0] + 1;
#ifndef FISHMOD_PCPROF_NORING // (a console bisection: profile samples untagged)
                    n64prof_set_frame_ring(rings, PCPROF_RING); // (samples tagged by their frame: slow or not)
#endif
                } else {
                    sFl[0] = NULL;
                }
            }
#endif
            Sc64_UsbText("FISHMOD-START\n"); // (for fishmod/tools/morning: the new ROM is running)
        }
        PcProf_Ranges();
        gFishmodTimerHook = PcProf_Hook;
        osSetTimer(&sPcTimer, PCPROF_INTERVAL, PCPROF_INTERVAL, NULL, NULL);
        sLastSave = now;
        return;
    }
#ifdef FISHMOD_PCPROF_CALIB
    {
        // once, early (before the route counts anything): the calibration loops, the sampler running
        static s32 sCalibrated;

        if (!sCalibrated && (sPlayFrames >= 20)) {
            sCalibrated = true;
            PcProf_Calibrate();
        }
    }
#endif
    // (every 8 frames: every frame cost 0.8% in pcprof1; an overlay loaded since is unmapped for a few frames)
    if ((sPlayFrames & 7) == 0) {
        PcProf_Ranges();
    }
#if defined(FISHMOD_PCPROF_NORING) || defined(FISHMOD_FRAMELOG_NOSEND) || defined(FISHMOD_FAKE_SC64)
    if ((sPlayFrames % 30) == 0) { // (the bisection ROMs: still running?)
        Fishmod_DebugPrintf(5, "test: frame %u, %u packets", sPlayFrames, sPcPackets);
    }
#endif
    if (sPcUsb) {
        u32 user[6] = { 0x464D4F44 /* 'FMOD' */, 0, 0, 0, 0, 0 };
        static s32 sDoneSaid;

#ifndef FISHMOD_NO_AUX // (a console test without the reboot handshake's per-frame poll of the cart)
        Sc64_AuxPoll();
#endif
        if (sUsbBackoff > 0) {
            sUsbBackoff--; // (the cart didn't finish a command: left alone a while)
            return;
        }
        if (sUsbTimeouts != 0) {
            // the link works again: say what happened (the morning runner's debug log keeps it)
            char line[96];
            u32 n = sUsbTimeouts;

            sUsbTimeouts = 0;
            sprintf(line, "FISHMOD-USB-STALL %u commands given up, from frame %u, status %08X, now frame %u\n", n,
                    sUsbTimeoutFrame, sUsbTimeoutScr, sPlayFrames);
            Sc64_UsbText(line);
        }
        user[1] = sPlayFrames;
        user[2] = FISHMOD_PROFILE_HW;
        // the route over (the bench's last lap): stop sampling, send what's counted, and mark the last packets
        // (user[3] = 1) so the reader knows the run ended there
        if (gFishmodBenchDone && !sPcStopped && (sPcSending == NULL)) {
            osStopTimer(&sPcTimer);
            gFishmodTimerHook = NULL;
            sPcStopped = true;
            sPcSending = n64prof_swap(sPcSpare);
            sPcSpare = NULL;
            sPcSendPos = 0;
        }
        if (sPcStopped) {
            user[3] = 1;
            if (sPcSending == NULL) {
                static s32 sSinceDone;

                Fishmod_DebugPrintf(5, "pcprof usb: DONE, %u packets: stop sc64deployer", sPcPackets);
                if (sDoneSaid && (++sSinceDone >= 20) && !Sc64_UsbBusy()) {
                    // A second after DONE the game stops itself for good, interrupts off, touching nothing: the
                    // morning runner uploads the next ROM into the cart while this one is still on, and a game still
                    // running reads the new ROM's bytes as its own (the frozen last frame of 12 Nov's runs). Now the
                    // picture stops here on purpose; switch the console off when the runner says.
                    Sc64_UsbText("FISHMOD-HALTED (the run is over: the picture stays until the console is switched off)\n");
                    for (sSinceDone = 0; (sSinceDone < 100000) && Sc64_UsbBusy(); sSinceDone++) {
                    }
                    __osDisableInt();
                    while (true) {
                    }
                }
                if (!sDoneSaid && !Sc64_UsbBusy()) { // (for fishmod/tools/morning: the run is over)
                    sDoneSaid = true;
#ifdef FISHMOD_FRAMELOG
                    PcProf_ActorReport();
#endif
                    Sc64_UsbText("FISHMOD-DONE\n");
                }
                return;
            }
        }
        // swap tables every few seconds, or sooner if the one counting fills up
        if ((sPcSending == NULL) &&
            ((n64prof_used() >= PCPROF_USB_SWAP_FILL) ||
             (OS_CYCLES_TO_USEC(now - sLastSave) > PCPROF_USB_SWAP_SECONDS * 1000000ULL))) {
            sPcSending = n64prof_swap(sPcSpare);
            sPcSpare = NULL;
            sPcSendPos = 0;
            sLastSave = now;
        }
#ifdef FISHMOD_FRAMELOG
        // the frame log first (one packet a frame in all)
#ifdef FISHMOD_FRAMELOG_NOSEND // (a console bisection: the records are made, never sent)
        sFlReady = -1;
#endif
        if ((sFlReady >= 0) && !Sc64_UsbBusy()) {
            FrameLogPacket* pk = sFl[sFlReady];

            if (Sc64_UsbSend(pk, 16 + pk->count * FRAMELOG_WORDS * 4) == 0) {
                sPcPackets++;
            }
            sFlReady = -1;
            return;
        }
#endif
        // one packet a frame, when the last one has gone (never waits: with no PC listening the counts wait, and
        // the table counting drops samples once full)
#ifdef FISHMOD_NO_PROFILE_SEND // (long runs: the frame log only, the profile's samples not sent)
        if (0) {
#else
        if ((sPcSending != NULL) && !Sc64_UsbBusy()) {
#endif
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
