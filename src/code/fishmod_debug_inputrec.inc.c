/*
 * The input recorder and its replay (FISHMOD_INPUTREC, FISHMOD_INPUTPLAY): a playthrough of the real game, from
 * power-on, played back by any later build. For the profilers: the game's own path through every system, cutscenes
 * and all, instead of a route made for a bench.
 *
 * Recording (a SummerCart64's USB, on the console or in the patched ares): every frame's controller word as the game
 * read it, the save memory once at power-on, the whole save context each time a game state starts (the title, the
 * file select, every scene loaded), and once a second where Link is and the random numbers' state.
 *
 * Replay (the recording attached to the ROM at INPUTREC_ROM, fishmod/tools/inputrec.py): the same controller words,
 * frame by frame. A build that isn't the one recorded won't stay in step on its own (objects load over a different
 * number of frames, the sound's timing moves a text box's end), so the replay puts itself back in step instead of
 * needing to be exact:
 * - each game state starts from the recording's save context, so a scene always starts as it did, whatever went
 *   before;
 * - once a second the random numbers are set to the recording's, and Link is put where he was if he's strayed;
 * - a scene left early skips the rest of its inputs; a scene not left when the recording left it is given a few
 *   seconds, then left by force for the next one.
 * How often each of those happened is the replay's report (gFishmodReplay, the 'FISHMOD-REPLAY' lines over USB): all
 * zeros is a replay in step with its recording.
 *
 * The stream is 32-bit words:
 *   bbbbxxyy   a frame's buttons and stick (buttons never 0xFFFF or 0xFFFE)
 *   FFFFnnnn   the last frame's word again, n more frames
 *   FFFE00tt, a count, that many words   a block: tt 1 a game state starts (its number in the game's table, the
 *              count of states so far, the random numbers' state, then the save context's words that changed since
 *              the last one, in runs: offset << 16 | words, the words); tt 2 once a second in play (the frame in this
 *              state, the random numbers, Link's position, yaw << 16 | scene << 8 | room); tt 0xFF the end
 */
#if defined(FISHMOD_INPUTREC) || defined(FISHMOD_INPUTPLAY)

#if defined(FISHMOD_INPUTREC) && !defined(FISHMOD_PCPROF)
#error "the input recorder shares the profiler's SummerCart64 link: define FISHMOD_PCPROF"
#endif

#include "z_game_dlftbls.h"

#define INPUTREC_ROM 0x03000000 // where a recording sits in the ROM (past the game's 40 MB)
#define IR_TAG 0xFFFE0000
#define IR_REPEAT 0xFFFF0000
#define IR_SEG 1
#define IR_SYNC 2
#define IR_END 0xFF
#define IR_SYNC_FRAMES 20
#define IR_SAVE_WORDS ((sizeof(SaveContext) + 3) / 4)

FishmodReplay gFishmodReplay;

static u32* sIrPrev; // the save context as of the last state's start (the next one's changes are against it)
static u32 sIrSegFrame;
static u32 sIrSegs;
static u32 sIrLast = 0xFFFFFFFF; // the last frame's word
static s32 sIrOff;               // no memory, or no recording: the game plays as usual

static u32 InputRec_Word(Input* input) {
    return ((u32)input->cur.button << 16) | ((u32)(u8)input->cur.stick_x << 8) | (u8)input->cur.stick_y;
}

static s32 InputRec_StateIndex(GameStateFunc init) {
    s32 i;

    for (i = 0; i < GAMESTATE_ID_MAX; i++) {
        if (gGameStateOverlayTable[i].init == init) {
            return i;
        }
    }
    return -1;
}

static void InputRec_Line(const char* fmt, ...) {
#ifdef FISHMOD_SCREEN_TEXT
    va_list args;
    char buf[128];

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    buf[FISHMOD_DEBUG_LINE_LEN - 1] = '\0';
    bcopy(buf, sDebugLines[4], FISHMOD_DEBUG_LINE_LEN);
#else
    (void)fmt;
#endif
}

#ifdef FISHMOD_INPUTREC
/*
 * Recording. The words wait in a ring until the USB is free: a packet is 'FINP', its number, its words, the words
 * dropped so far (the ring full: no PC listening), then the words.
 */
#define IR_RING 8192 // words
#define IR_PACKET 1020

static u32* sIrRing;
static u32* sIrPacket;
static u32 sIrHead;
static u32 sIrCount;
static u32 sIrSeq;
static u32 sIrLost;
static u32 sIrRepeat;
static u32 sIrSinceSend;

static void InputRec_Put(u32 word) {
    if (sIrCount >= IR_RING) {
        sIrLost++;
        return;
    }
    sIrRing[(sIrHead + sIrCount++) % IR_RING] = word;
}

static void InputRec_Flush(void) {
    if (sIrRepeat != 0) {
        InputRec_Put(IR_REPEAT | sIrRepeat);
        sIrRepeat = 0;
    }
}

/** Sends a packet if there's one worth sending and the USB is free. */
static s32 InputRec_Pump(void) {
    u32 n;
    u32 i;

    sIrSinceSend++;
    if ((sIrRing == NULL) || (sIrCount == 0) || ((sIrCount < 256) && (sIrSinceSend < 40)) || Sc64_UsbBusy()) {
        return 0;
    }
    n = MIN(sIrCount, IR_PACKET);
    sIrPacket[0] = 0x46494E50; // 'FINP'
    sIrPacket[1] = sIrSeq;
    sIrPacket[2] = n;
    sIrPacket[3] = sIrLost;
    for (i = 0; i < n; i++) {
        sIrPacket[4 + i] = sIrRing[(sIrHead + i) % IR_RING];
    }
    if (Sc64_UsbSend(sIrPacket, (16 + n * 4 + 7) & ~7) != 0) {
        return 0; // (not taken: the words stay for the next try)
    }
    sIrSeq++;
    sIrHead = (sIrHead + n) % IR_RING;
    sIrCount -= n;
    sIrSinceSend = 0;
    return 1;
}

static s32 InputRec_Setup(void) {
    if (sIrOff || (sIrRing != NULL)) {
        return !sIrOff;
    }
    sIrRing = SYSTEM_ARENA_MALLOC(IR_RING * 4, "../fishmod_debug.c", __LINE__);
    sIrPacket = SYSTEM_ARENA_MALLOC(16 + IR_PACKET * 4 + 8, "../fishmod_debug.c", __LINE__);
    sIrPrev = SYSTEM_ARENA_MALLOC(IR_SAVE_WORDS * 4, "../fishmod_debug.c", __LINE__);
    if ((sIrRing == NULL) || (sIrPacket == NULL) || (sIrPrev == NULL) || !Sc64_Ensure()) {
        sIrOff = true;
        sIrRing = NULL;
        return false;
    }
    bzero(sIrPrev, IR_SAVE_WORDS * 4);
    return true;
}

void Fishmod_InputRecSram(u8* sram) {
    static s32 sSent;
    u32 i;
    s32 wait;

    if (sSent || !InputRec_Setup()) {
        return;
    }
    sSent = true;
    // the save memory as the game found it, 4 KB a packet ('FSRM', the packet's number, of how many), waited for
    for (i = 0; i < SRAM_SIZE / 0x1000; i++) {
        sIrPacket[0] = 0x4653524D;
        sIrPacket[1] = i;
        sIrPacket[2] = SRAM_SIZE / 0x1000;
        sIrPacket[3] = IR_SAVE_WORDS; // (a replay checks its save context is the same size)
        bcopy(sram + i * 0x1000, &sIrPacket[4], 0x1000);
        for (wait = 0; Sc64_UsbBusy() && (wait < 100000); wait++) {
        }
        Sc64_UsbSend(sIrPacket, 16 + 0x1000);
    }
}

void Fishmod_InputRecState(GameState* gameState, GameStateFunc init) {
    u32* now = (u32*)&gSaveContext;
    u32 i;
    u32 words = 3;

    if (!InputRec_Setup()) {
        return;
    }
    InputRec_Flush();
    sIrLast = 0xFFFFFFFF;
    // (the block's length first: the runs counted, then written)
    for (i = 0; i < IR_SAVE_WORDS;) {
        if (now[i] != sIrPrev[i]) {
            u32 n = 1;

            while ((i + n < IR_SAVE_WORDS) && (now[i + n] != sIrPrev[i + n])) {
                n++;
            }
            words += 1 + n;
            i += n;
        } else {
            i++;
        }
    }
    InputRec_Put(IR_TAG | IR_SEG);
    InputRec_Put(words);
    InputRec_Put(InputRec_StateIndex(init));
    InputRec_Put(sIrSegs++);
    InputRec_Put(*Fishmod_RandState());
    for (i = 0; i < IR_SAVE_WORDS;) {
        if (now[i] != sIrPrev[i]) {
            u32 n = 1;
            u32 k;

            while ((i + n < IR_SAVE_WORDS) && (now[i + n] != sIrPrev[i + n])) {
                n++;
            }
            InputRec_Put((i << 16) | n);
            for (k = 0; k < n; k++) {
                InputRec_Put(now[i + k]);
                sIrPrev[i + k] = now[i + k];
            }
            i += n;
        } else {
            i++;
        }
    }
    sIrSegFrame = 0;
}

void Fishmod_InputRecInput(GameState* gameState, Input* inputs) {
    u32 word;

    if (sIrOff || (sIrRing == NULL)) {
        return;
    }
    if ((gameState->main == Play_Main) && ((sIrSegFrame % IR_SYNC_FRAMES) == 0)) {
        PlayState* play = (PlayState*)gameState;
        Player* player = GET_PLAYER(play);

        if (player != NULL) {
            InputRec_Flush();
            InputRec_Put(IR_TAG | IR_SYNC);
            InputRec_Put(6);
            InputRec_Put(sIrSegFrame);
            InputRec_Put(*Fishmod_RandState());
            InputRec_Put(*(u32*)&player->actor.world.pos.x);
            InputRec_Put(*(u32*)&player->actor.world.pos.y);
            InputRec_Put(*(u32*)&player->actor.world.pos.z);
            InputRec_Put(((u32)(u16)player->actor.shape.rot.y << 16) | ((play->sceneId & 0xFF) << 8) |
                         (play->roomCtx.curRoom.num & 0xFF));
        }
    }
    word = InputRec_Word(&inputs[0]);
    if ((word == sIrLast) && (sIrRepeat < 0xFFFF)) {
        sIrRepeat++;
    } else {
        InputRec_Flush();
        InputRec_Put(word);
        sIrLast = word;
    }
    sIrSegFrame++;
    gFishmodReplay.frames++;
    if ((gFishmodReplay.frames % 20) == 0) {
        InputRec_Flush(); // (so a recording cut off loses a second at most)
        InputRec_Line("REC %u:%02u  %u states  %u sent%s", gFishmodReplay.frames / 1200,
                      (gFishmodReplay.frames / 20) % 60, sIrSegs, sIrSeq, (sIrLost != 0) ? "  LOST WORDS" : "");
    }
    if (((gFishmodReplay.frames % 600) == 0) && (gameState->main == Play_Main)) {
        // every half minute, for the PC's terminal: still recording, and whether anything was dropped
        char line[96];

        sprintf(line, "FISHMOD-REC %u:%02u, %u game states, %u packets, %u words dropped\n", gFishmodReplay.frames / 1200,
                (gFishmodReplay.frames / 20) % 60, sIrSegs, sIrSeq, sIrLost);
        Sc64_UsbText(line);
    }
    gFishmodReplay.lost = sIrLost;
    if (gameState->main != Play_Main) {
        InputRec_Pump(); // (in play the profiler's update shares the USB out: PcProf_Update)
    }
}
#endif

#ifdef FISHMOD_INPUTPLAY
/* Replay: the stream read from the ROM a block at a time. */
#define IR_BUF 1024 // words
#define IR_LATE_FRAMES 100 // how long a state may outlast its recording before it's left by force
#define IR_SNAP_DIST 40.0f

static u32* sIrBuf;
static u32 sIrPos;  // the next word's index in the stream
static u32 sIrBufAt = 0xFFFFFFFF;
static u32 sIrWords;
static u32 sIrRepeat;
static u32 sIrSramRom;
static s32 sIrEnded;
static s32 sIrForce = -1; // the state to leave for
static u32 sIrLate;

/** Reads from the ROM past the game's files (the DMA manager only reads inside its file table). */
static void InputPlay_Read(void* buf, u32 rom, u32 size) {
    extern OSPiHandle* gCartHandle;
    static OSMesgQueue sQueue;
    static OSMesg sMsg;
    static OSIoMesg sIo;
    static s32 sInit;
    OSMesg msg;

    if (!sInit) {
        sInit = true;
        osCreateMesgQueue(&sQueue, &sMsg, 1);
    }
    osInvalDCache(buf, size);
    sIo.hdr.pri = OS_MESG_PRI_NORMAL;
    sIo.hdr.retQueue = &sQueue;
    sIo.dramAddr = buf;
    sIo.devAddr = rom;
    sIo.size = size;
    osEPiStartDma(gCartHandle, &sIo, OS_READ);
    osRecvMesg(&sQueue, &msg, OS_MESG_BLOCK);
}

static u32 InputPlay_Peek(void) {
    u32 base = sIrPos & ~(IR_BUF - 1);

    if (sIrPos >= sIrWords) {
        return IR_TAG | IR_END;
    }
    if (base != sIrBufAt) {
        InputPlay_Read(sIrBuf, gFishmodReplay.rom + base * 4, IR_BUF * 4);
        sIrBufAt = base;
    }
    return sIrBuf[sIrPos & (IR_BUF - 1)];
}

static u32 InputPlay_Next(void) {
    u32 w = InputPlay_Peek();

    sIrPos++;
    return w;
}

static s32 InputPlay_Setup(void) {
    u32* head;

    if (sIrOff || (sIrBuf != NULL)) {
        return !sIrOff;
    }
    sIrBuf = SYSTEM_ARENA_MALLOC(IR_BUF * 4, "../fishmod_debug.c", __LINE__);
    sIrPrev = SYSTEM_ARENA_MALLOC(IR_SAVE_WORDS * 4, "../fishmod_debug.c", __LINE__);
    if ((sIrBuf == NULL) || (sIrPrev == NULL)) {
        sIrOff = true;
        return false;
    }
    // the header: 'FINP', version, the save memory's bytes, the stream's words, the save context's words
    head = sIrBuf;
    InputPlay_Read(head, INPUTREC_ROM, 32);
    if ((head[0] != 0x46494E50) || (head[1] != 1) || (head[4] != IR_SAVE_WORDS)) {
        sIrOff = true;
        gFishmodReplay.state = (head[0] != 0x46494E50) ? 0xE1 : 0xE2; // no recording; one that doesn't fit this build
        InputRec_Line((head[0] != 0x46494E50) ? "REPLAY: no recording in the ROM" : "REPLAY: recording doesn't fit");
        return false;
    }
    sIrSramRom = INPUTREC_ROM + 32;
    gFishmodReplay.rom = sIrSramRom + head[2];
    sIrWords = head[3];
    bzero(sIrPrev, IR_SAVE_WORDS * 4);
    return true;
}

static void InputPlay_Report(const char* what) {
#ifdef FISHMOD_PCPROF
    char line[160];

    sprintf(line, "FISHMOD-REPLAY %s state %u frame %u: early %u (%u frames) late %u forced %u snaps %u rng %u\n", what,
            gFishmodReplay.segs, gFishmodReplay.frames, gFishmodReplay.early, gFishmodReplay.skipped,
            gFishmodReplay.late, gFishmodReplay.forced, gFishmodReplay.snaps, gFishmodReplay.rng);
    Sc64_UsbText(line);
#else
    (void)what; // (no USB in this build: the counts are read from gFishmodReplay)
#endif
}

void Fishmod_InputRecSram(u8* sram) {
    static s32 sDone;

    if (sDone || !InputPlay_Setup()) {
        return;
    }
    sDone = true;
    // the recording's save memory: into the game's copy, and into the cart (the file select reads it from there)
    InputPlay_Read(sram, sIrSramRom, SRAM_SIZE);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000), sram, SRAM_SIZE, OS_WRITE);
}

/** Skips the words of a block (its tag just read). */
static void InputPlay_SkipBlock(void) {
    u32 n = InputPlay_Next();

    sIrPos += n;
}

void Fishmod_InputRecState(GameState* gameState, GameStateFunc init) {
    u32* now = (u32*)&gSaveContext;
    s32 index = InputRec_StateIndex(init);
    u32 skipped = 0;
    u32 n;
    u32 w;
    u32 end;

    if (!InputPlay_Setup() || sIrEnded) {
        return;
    }
    // to the recording's next state: the inputs before it are a state the replay left early
    skipped = sIrRepeat;
    sIrRepeat = 0;
    while (true) {
        w = InputPlay_Peek();
        if ((w & 0xFFFF0000) == IR_TAG) {
            if (((w & 0xFF) == IR_SEG) || ((w & 0xFF) == IR_END)) {
                break;
            }
            sIrPos++;
            InputPlay_SkipBlock();
        } else {
            sIrPos++;
            skipped += ((w & 0xFFFF0000) == IR_REPEAT) ? (w & 0xFFFF) : 1;
        }
    }
    if ((skipped != 0) && (sIrForce < 0)) {
        gFishmodReplay.early++;
        gFishmodReplay.skipped += skipped;
    }
    sIrForce = -1;
    sIrLate = 0;
    if ((w & 0xFF) == IR_END) {
        return; // (the end is met by the next frame's input)
    }
    // (peeked: the block is taken only if this is the state it's for)
    InputPlay_Read(sIrBuf, gFishmodReplay.rom + (sIrPos + 2) * 4, 8);
    sIrBufAt = 0xFFFFFFFF;
    if ((s32)sIrBuf[0] != index) {
        sIrForce = sIrBuf[0]; // the wrong state: left on its first frame for the recording's
        return;
    }
    sIrPos++;
    n = InputPlay_Next();
    end = sIrPos + n;
    InputPlay_Next();
    gFishmodReplay.segs = InputPlay_Next() + 1;
    *Fishmod_RandState() = InputPlay_Next();
    while (sIrPos < end) {
        u32 run = InputPlay_Next();
        u32 at = run >> 16;

        for (run &= 0xFFFF; (run != 0) && (at < IR_SAVE_WORDS); run--, at++) {
            sIrPrev[at] = InputPlay_Next();
        }
    }
    sIrPos = end;
    bcopy(sIrPrev, now, sizeof(SaveContext));
    sIrSegFrame = 0;
    sIrLast = 0;
    gFishmodReplay.state = index;
    if ((gFishmodReplay.segs % 8) == 1) {
        InputPlay_Report("at");
    }
}

static void InputPlay_Sync(GameState* gameState) {
    u32 frame = InputPlay_Next();
    u32 rng = InputPlay_Next();
    Vec3f pos;
    u32 where;

    *(u32*)&pos.x = InputPlay_Next();
    *(u32*)&pos.y = InputPlay_Next();
    *(u32*)&pos.z = InputPlay_Next();
    where = InputPlay_Next();
    if (gameState->main != Play_Main) {
        return;
    }
    {
        PlayState* play = (PlayState*)gameState;
        Player* player = GET_PLAYER(play);

        if (*Fishmod_RandState() != rng) {
            gFishmodReplay.rng++;
#ifndef FISHMOD_INPUTPLAY_LOOSE
            *Fishmod_RandState() = rng;
#endif
        }
        gFishmodReplay.syncs++;
#ifndef FISHMOD_INPUTPLAY_LOOSE
        if ((player != NULL) && (((where >> 8) & 0xFF) == (u32)(play->sceneId & 0xFF)) &&
            (Math_Vec3f_DistXYZ(&player->actor.world.pos, &pos) > IR_SNAP_DIST)) {
            player->actor.world.pos = pos;
            player->actor.prevPos = pos;
            player->actor.shape.rot.y = player->actor.world.rot.y = player->yaw = (s16)(where >> 16);
            gFishmodReplay.snaps++;
        }
#else
        if ((player != NULL) && (Math_Vec3f_DistXYZ(&player->actor.world.pos, &pos) > IR_SNAP_DIST)) {
            gFishmodReplay.snaps++; // (counted, not put right: the test of what stays in step by itself)
        }
#endif
    }
    (void)frame;
}

void Fishmod_InputRecInput(GameState* gameState, Input* inputs) {
    Input* input = &inputs[0];
    u32 word = 0;
    u16 buttonDiff;

    if (sIrOff || (sIrBuf == NULL)) {
        return;
    }
    if (sIrEnded) {
        static s32 sSaid;

#ifdef FISHMOD_PCPROF
        if (!sSaid && sPcUsb && (sPcOut != NULL)) {
#else
        if (!sSaid) {
#endif
            sSaid = true;
            InputPlay_Report("END");
            gFishmodBenchDone = true; // (the profiler sends its last counts and stops)
        }
        return; // (the controller's own from here)
    }
#ifdef FISHMOD_INPUTPLAY_PERTURB
    // (the test of the replay's own repairs: the random numbers knocked out of step every few seconds)
    if ((gFishmodReplay.frames % FISHMOD_INPUTPLAY_PERTURB) == (FISHMOD_INPUTPLAY_PERTURB - 1)) {
        Rand_Next();
    }
#endif
    if (sIrForce >= 0) {
        GameStateOverlay* ovl = &gGameStateOverlayTable[sIrForce];

        gameState->running = false;
        gameState->init = ovl->init;
        gameState->size = ovl->instanceSize;
        gFishmodReplay.forced++;
        word = 0;
    } else if (sIrRepeat != 0) {
        sIrRepeat--;
        word = sIrLast;
    } else {
        while (true) {
            u32 w = InputPlay_Peek();

            if ((w & 0xFFFF0000) == IR_TAG) {
                if ((w & 0xFF) == IR_SYNC) {
                    sIrPos += 2;
                    InputPlay_Sync(gameState);
                    continue;
                }
                if ((w & 0xFF) == IR_END) {
                    sIrEnded = true;
                    gFishmodReplay.ended = 1; // (a runner watching this word stops the emulator here)
                    return;
                }
                if ((w & 0xFF) == IR_SEG) {
                    // the recording left this state here: a little longer (a text box a few frames behind), then by
                    // force
                    if (sIrLate == 0) {
                        gFishmodReplay.late++;
                    }
                    if (++sIrLate > IR_LATE_FRAMES) {
                        InputPlay_Read(sIrBuf, gFishmodReplay.rom + (sIrPos + 2) * 4, 8);
                        sIrBufAt = 0xFFFFFFFF;
                        sIrForce = sIrBuf[0];
                    }
                    word = 0;
                    break;
                }
                sIrPos++;
                InputPlay_SkipBlock();
                continue;
            }
            sIrPos++;
            if ((w & 0xFFFF0000) == IR_REPEAT) {
                sIrRepeat = (w & 0xFFFF) - 1;
                word = sIrLast;
            } else {
                word = sIrLast = w;
            }
            break;
        }
    }
    input->prev = input->cur;
    input->cur.button = word >> 16;
    input->cur.stick_x = (s8)(word >> 8);
    input->cur.stick_y = (s8)word;
    buttonDiff = input->prev.button ^ input->cur.button;
    input->press.button = input->cur.button & buttonDiff;
    input->rel.button = input->prev.button & buttonDiff;
    PadUtils_UpdateRelXY(input);
    input->press.stick_x = input->cur.stick_x - input->prev.stick_x;
    input->press.stick_y = input->cur.stick_y - input->prev.stick_y;
    sIrSegFrame++;
    gFishmodReplay.frames++;
    if ((gFishmodReplay.frames % 20) == 0) {
        InputRec_Line("PLAY %u:%02u s%u e%u l%u f%u p%u r%u", gFishmodReplay.frames / 1200,
                      (gFishmodReplay.frames / 20) % 60, gFishmodReplay.segs, gFishmodReplay.early, gFishmodReplay.late,
                      gFishmodReplay.forced, gFishmodReplay.snaps, gFishmodReplay.rng);
    }
}
#endif

#else

FishmodReplay gFishmodReplay;

void Fishmod_InputRecSram(u8* sram) {
}

void Fishmod_InputRecState(GameState* gameState, GameStateFunc init) {
}

void Fishmod_InputRecInput(GameState* gameState, Input* inputs) {
}

#endif
