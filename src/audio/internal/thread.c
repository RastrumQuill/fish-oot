/**
 * Original Filename: sub_sys.c
 */

#include "array_count.h"
#include "audiothread_cmd.h"
#include "stack_pad.h"
#include "ultra64.h"
#include "versions.h"
#include "audio.h"

#define SAMPLES_TO_OVERPRODUCE 0x10
#define EXTRA_BUFFERED_AI_SAMPLES_TARGET 0x80

AudioTask* AudioThread_UpdateImpl(void);
#if FISHMOD_DEBUG
extern s32 gFishmodAudioLockstep; /* fish-oot: the audio check (code/audio_thread_manager.c) */
extern u8* gFishmodAudNoise;
void Fishmod_AudLog_Buffer(s16* buf, s32 samples);
#endif
void AudioThread_SetFadeInTimer(s32 seqPlayerIndex, s32 fadeTimer);
void AudioThread_SetFadeOutTimer(s32 seqPlayerIndex, s32 fadeTimer);
void AudioThread_ProcessCmds(u32);
void AudioThread_ProcessSeqPlayerCmd(SequencePlayer* seqPlayer, AudioCmd* cmd);
void AudioThread_ProcessChannelCmd(SequenceChannel* channel, AudioCmd* cmd);
s32 func_800E66C0(s32 flags);

#if FISHMOD_DEBUG
/*
 * fish-oot: the audio check (lockstep): before the last update's RSP task is sent, its sample DMAs are waited for (the
 * game's own wait comes after the send: in play the DMAs are long done by the next refresh; updates back to back can
 * outrun them). The done messages go back on the queue, so the update's own wait finds them as before.
 */
void AudioThread_LockstepWaitDmas(void) {
    OSMesg msg[64];
    s32 n = gAudioCtx.curAudioFrameDmaCount;
    s32 i;

    if (n > 64) {
        n = 64;
    }
    for (i = 0; i < n; i++) {
        osRecvMesg(&gAudioCtx.curAudioFrameDmaQueue, &msg[i], OS_MESG_BLOCK);
    }
    for (i = 0; i < n; i++) {
        osSendMesg(&gAudioCtx.curAudioFrameDmaQueue, msg[i], OS_MESG_NOBLOCK);
    }
}
#endif

// AudioMgr_Retrace
AudioTask* AudioThread_Update(void) {
    return AudioThread_UpdateImpl();
}

#if !(OOT_VERSION < PAL_1_0 || !PLATFORM_N64)
static s32 sMaxAbiCmdCnt = 0x80;
static AudioTask* sWaitingAudioTask = NULL;
#endif

/**
 * This is Audio_Update for the audio thread
 *
 * original name: CreateAudioTask (note: function is heavily modified in Animal Crossing)
 */
extern u8 gFishmodDmaTtlAhead; // (load.c)
// (in every build, so the files that set them are the same code with and without --expak)
u8 gFishmodAudioAhead = false; // updates built ahead when the driver is steady (--expak): off until the console says
                               // it pays (the audio_sched_ab bench switches it by lap; the audio check has it on)
static s32 sFishmodCallSlot; // the output buffer ring's slot of the refresh being handled
#if FISHMOD_EXPAK
u8 gFishmodTaskRing = 2;      // the command lists and tasks in use: 6, or the heap's 2 for a spec too big (heap.c)
u8 gFishmodBuiltCount;        // the updates built by the last call (0: its update was built ahead; 1-3), for the audio
AudioTask* gFishmodBuilt[3];  // manager (code/audio_thread_manager.c): their tasks, in order,
u8 gFishmodBuiltDmas;         // and the sample loads the last of them started (its task can't run until they're done)
static u8 sFishmodAhead;      // updates built and not yet due (0-2)
static u8 sFishmodWillBatch;  // this call's update is to be followed by the next two (decided before it's built)
#define FISHMOD_FILTER_SNAPS 160 // filters copied for one task: three ticks of 24 notes and two reverbs' pairs, and room
static s16 sFishmodFilterSnaps[FISHMOD_TASK_RING][FISHMOD_FILTER_SNAPS * 8] __attribute__((aligned(16)));
extern s16* gFishmodFilterSnap; // (synthesis.c)
extern s16* gFishmodFilterSnapEnd;
static AudioTask* sFishmodAheadTask[2];
#define FISHMOD_TASK_RING_NOW gFishmodTaskRing

/**
 * Whether the next refresh's update can be built now: it must not need anything that only happens between
 * refreshes. That is the RSP's output (a reverb that downsamples reads back what the task before last wrote), a
 * reset in progress or just started (its steps are timed in refreshes), or the driver's own two-refresh mode.
 */
static s32 AudioThread_FishmodSteady(void) {
    s32 i;

    if (!gFishmodAudioAhead || (gFishmodTaskRing != FISHMOD_TASK_RING) || (gAudioCtx.resetStatus != 0) ||
        (gAudioCtx.resetTimer != 0) || (gAudioCtx.audioBufferParameters.specUnk4 != 1)) {
        return false;
    }
    for (i = 0; i < gAudioCtx.numSynthesisReverbs; i++) {
        if (gAudioCtx.synthesisReverbs[i].downsampleRate >= 2) {
            return false;
        }
    }
    // nor while anything is being loaded: a load can take the room of a font or a sample that a task built and not
    // yet run still reads its codebooks and samples from
    if ((gAudioCtx.preloadSampleStackTop > 0) || (gAudioCtx.slowLoads[0].state != 0 /* SLOW_LOAD_STATE_WAITING (load.c) */) ||
        (gAudioCtx.slowLoads[1].state != 0)) {
        return false;
    }
    for (i = 0; i < ARRAY_COUNT(gAudioCtx.asyncLoads); i++) {
        if (gAudioCtx.asyncLoads[i].status != 0) {
            return false;
        }
    }
    return true;
}
#else
#define FISHMOD_TASK_RING_NOW FISHMOD_TASK_RING
#endif

/*
 * fish-oot: the rest of the audio update, as the game's: the sample loads waited for, the loads and the reset stepped,
 * the game's commands taken, the command list built for the next slot of the rings, the task filled in. Returns the
 * task (NULL where the game's update returned none). Its own function so that an update can be followed by the next
 * refreshes' in the same wake (AudioThread_UpdateImpl, FISHMOD_EXPAK): ahead is how many refreshes early this one is
 * built (0: on its own refresh, exactly the game's update), taskCount the update's number (the game's totalTaskCount
 * when it would have run), samplesRemainingInAi what the audio interface holds, or will by then.
 */
static AudioTask* AudioThread_Body(u32 samplesRemainingInAi, u32 taskCount, s32 ahead, s32* lenOut, s32* cmdsOut) {
    s32 abiCmdCnt;
    s32 j;
    s32 sp5C;
    s16* curAiBuffer;
    OSTask_t* task;
    s32 index;
    u32 sp4C;
    s32 sp48;
    s32 i;
#if FISHMOD_DEBUG
    extern u64 gFishmodAudTime[];
    u32 fmStart = osGetCount();
#endif

    gFishmodDmaTtlAhead = ahead;
    gAudioCtx.rspTaskIndex = (gAudioCtx.rspTaskIndex + 1) % FISHMOD_TASK_RING_NOW;
#if FISHMOD_EXPAK
    // (a task that will run after later updates are built filters with its own copies of the coefficients)
    gFishmodFilterSnap = sFishmodWillBatch ? sFishmodFilterSnaps[gAudioCtx.rspTaskIndex] : NULL;
    gFishmodFilterSnapEnd = sFishmodFilterSnaps[gAudioCtx.rspTaskIndex] + FISHMOD_FILTER_SNAPS * 8;
#endif
    gAudioCtx.curAiBufIndex = (gAudioCtx.curAiBufIndex + 1) % FISHMOD_AI_RING;
#if FISHMOD_AI_RING != 3
    if ((gAudioCtx.resetStatus != 0) || (gAudioCtx.resetTimer > 16)) {
        // fish-oot: an update that builds nothing (the audio heap being reset) leaves its output buffer as it was, and
        // that buffer is still queued in its turn: with the game's three buffers it holds the update's of three
        // refreshes ago. With six it would hold another, older one: given the same samples and length here, so
        // what's heard while the heap resets is what always was.
        s32 from = (gAudioCtx.curAiBufIndex + FISHMOD_AI_RING - 3) % FISHMOD_AI_RING;

        gAudioCtx.aiBufLengths[gAudioCtx.curAiBufIndex] = gAudioCtx.aiBufLengths[from];
        bcopy(gAudioCtx.aiBuffers[from], gAudioCtx.aiBuffers[gAudioCtx.curAiBufIndex], AIBUF_SIZE);
    }
#endif

    sp5C = gAudioCtx.curAudioFrameDmaCount;
    for (i = 0; i < gAudioCtx.curAudioFrameDmaCount; i++) {
        if (osRecvMesg(&gAudioCtx.curAudioFrameDmaQueue, NULL, OS_MESG_NOBLOCK) == 0) {
            sp5C--;
        }
    }

    if (sp5C != 0) {
        for (i = 0; i < sp5C; i++) {
            osRecvMesg(&gAudioCtx.curAudioFrameDmaQueue, NULL, OS_MESG_BLOCK);
        }
    }

    sp48 = MQ_GET_COUNT(&gAudioCtx.curAudioFrameDmaQueue);
    if (sp48 != 0) {
        for (i = 0; i < sp48; i++) {
            osRecvMesg(&gAudioCtx.curAudioFrameDmaQueue, NULL, OS_MESG_NOBLOCK);
        }
    }

    gAudioCtx.curAudioFrameDmaCount = 0;
    if (ahead == 0) {
        // (the countdowns drop once a refresh: an update built ahead leaves them, and its own refresh drops them)
        AudioLoad_DecreaseSampleDmaTtls();
    }
    AudioLoad_ProcessLoads(gAudioCtx.resetStatus);
    AudioLoad_ProcessScriptLoads();

    if (gAudioCtx.resetStatus != 0) {
        if (AudioHeap_ResetStep() == 0) {
            if (gAudioCtx.resetStatus == 0) {
                osSendMesg(gAudioCtx.audioResetQueueP, (OSMesg)(u32)gAudioCtx.specId, OS_MESG_NOBLOCK);
            }

            sWaitingAudioTask = NULL;
            return NULL;
        }
    }

    if (gAudioCtx.resetTimer > 16) {
        return NULL;
    }
    if (gAudioCtx.resetTimer != 0) {
        gAudioCtx.resetTimer++;
    }

    gAudioCtx.curTask = &gAudioCtx.rspTask[gAudioCtx.rspTaskIndex];
    gAudioCtx.curAbiCmdBuf = gAudioCtx.abiCmdBufs[gAudioCtx.rspTaskIndex];

    index = gAudioCtx.curAiBufIndex;
    curAiBuffer = gAudioCtx.aiBuffers[index];

    gAudioCtx.aiBufLengths[index] =
        (s16)((((gAudioCtx.audioBufferParameters.samplesPerFrameTarget - samplesRemainingInAi) +
                EXTRA_BUFFERED_AI_SAMPLES_TARGET) &
               ~0xF) +
              SAMPLES_TO_OVERPRODUCE);
    if (gAudioCtx.aiBufLengths[index] < gAudioCtx.audioBufferParameters.minAiBufferLength) {
        gAudioCtx.aiBufLengths[index] = gAudioCtx.audioBufferParameters.minAiBufferLength;
    }

    if (gAudioCtx.aiBufLengths[index] > gAudioCtx.audioBufferParameters.maxAiBufferLength) {
        gAudioCtx.aiBufLengths[index] = gAudioCtx.audioBufferParameters.maxAiBufferLength;
    }

    *lenOut = gAudioCtx.aiBufLengths[index];

    j = 0;
    if (gAudioCtx.resetStatus == 0) {
        // msg = 0000RREE R = read pos, E = End Pos
        while (osRecvMesg(gAudioCtx.threadCmdProcQueueP, (OSMesg*)&sp4C, OS_MESG_NOBLOCK) != -1) {
            if (1) {}
            AudioThread_ProcessCmds(sp4C);
            j++;
        }
        *cmdsOut = j;
        if ((j == 0) && (gAudioCtx.threadCmdQueueFinished)) {
            AudioThread_ScheduleProcessCmds();
        }
    }

    gAudioCtx.curAbiCmdBuf =
        AudioSynth_Update(gAudioCtx.curAbiCmdBuf, &abiCmdCnt, curAiBuffer, gAudioCtx.aiBufLengths[index]);

    // Update audioRandom to the next random number
#if FISHMOD_DEBUG
    if (gFishmodAudioLockstep) {
        /* fish-oot: the audio check: the cycle count and the code's bytes vary between builds; fixed stand-ins */
        // (not the output buffer's old samples either, as the game adds: which old buffer that is depends on how many
        // there are in the ring, 3 or 6 (FISHMOD_AI_RING), and the check compares builds of both)
        gAudioCtx.audioRandom =
            (gAudioCtx.audioRandom + taskCount) * (taskCount * 0x9E3779B1 + 1);
        gWaveSamples[8] = (s16*)(gFishmodAudNoise + (gAudioCtx.audioRandom & 0xFFF0));
    } else
#endif
    {
        gAudioCtx.audioRandom = (gAudioCtx.audioRandom + taskCount) * osGetCount();
        gAudioCtx.audioRandom = gAudioCtx.audioRandom + gAudioCtx.aiBuffers[index][taskCount & 0xFF];

        // gWaveSamples[8] interprets compiled assembly code as s16 samples as a way to generate sound with noise.
        // Start with the address of AudioThread_Update, and offset it by a random number between 0 - 0xFFF0
        // Use the resulting address as the starting address to interpret an array of samples i.e. `s16 samples[]`
        gWaveSamples[8] = (s16*)((u8*)AudioThread_Update + (gAudioCtx.audioRandom & 0xFFF0));
    }

    index = gAudioCtx.rspTaskIndex;
    gAudioCtx.curTask->msgQueue = NULL;
    gAudioCtx.curTask->unk_44 = NULL;

    task = &gAudioCtx.curTask->task.t;
    task->type = M_AUDTASK;
    task->flags = 0;
    task->ucode_boot = aspMainTextStart;
    task->ucode_boot_size = SP_UCODE_SIZE;
    task->ucode_data_size = (size_t)(aspMainDataEnd - aspMainDataStart) * sizeof(u64) - 1;
    task->ucode = aspMainTextStart;
    task->ucode_data = aspMainDataStart;
    task->ucode_size = SP_UCODE_SIZE;
    task->dram_stack = NULL;
    task->dram_stack_size = 0;
    task->output_buff = NULL;
    task->output_buff_size = NULL;
    if (1) {}
    task->data_ptr = (u64*)gAudioCtx.abiCmdBufs[index];
    task->data_size = abiCmdCnt * sizeof(Acmd);
    task->yield_data_ptr = NULL;
    task->yield_data_size = 0;

    if (sMaxAbiCmdCnt < abiCmdCnt) {
        sMaxAbiCmdCnt = abiCmdCnt;
    }
#if FISHMOD_DEBUG
    gFishmodAudTime[0]++;
    gFishmodAudTime[1] += (u32)(osGetCount() - fmStart);
#endif

    if (gAudioCtx.audioBufferParameters.specUnk4 == 1) {
        return gAudioCtx.curTask;
    } else {
        sWaitingAudioTask = gAudioCtx.curTask;
        return NULL;
    }
}

AudioTask* AudioThread_UpdateImpl(void) {
#if OOT_VERSION < PAL_1_0 || !PLATFORM_N64
    static s32 sMaxAbiCmdCnt = 0x80;
    static AudioTask* sWaitingAudioTask = NULL;
#endif
    u32 samplesRemainingInAi;
    s32 abiCmdCnt;
    STACK_PAD(s32);
    s32 j;
    s32 sp5C;
    s16* curAiBuffer;
    OSTask_t* task;
    s32 index;
    u32 sp4C;
    s32 sp48;
    s32 i;

#if FISHMOD_DEBUG
    extern u64 gFishmodAudTime[];
    u32 fmStart = osGetCount();
#endif

    gAudioCtx.totalTaskCount++;
    if ((gAudioCtx.totalTaskCount % gAudioCtx.audioBufferParameters.specUnk4) != 0) {
        if (gAudioCustomUpdateFunction != NULL) {
            gAudioCustomUpdateFunction();
        }

        if (((gAudioCtx.totalTaskCount % gAudioCtx.audioBufferParameters.specUnk4) + 1) ==
            gAudioCtx.audioBufferParameters.specUnk4) {
            return sWaitingAudioTask;
        } else {
            return NULL;
        }
    }

    osSendMesg(gAudioCtx.taskStartQueueP, (OSMesg)gAudioCtx.totalTaskCount, OS_MESG_NOBLOCK);
    // fish-oot: this refresh's place in the ring of output buffers, kept apart from the last one built
    // (gAudioCtx.curAiBufIndex, which AudioThread_Body advances): they're the same unless updates were built ahead
    sFishmodCallSlot = (sFishmodCallSlot + 1) % FISHMOD_AI_RING;
    index = (sFishmodCallSlot - 2 + FISHMOD_AI_RING) % FISHMOD_AI_RING;
    samplesRemainingInAi = osAiGetLength() / 4;
#if FISHMOD_DEBUG
    if (gFishmodAudioLockstep) {
        /* fish-oot: the audio check: as many samples every update, not what the hardware still holds */
        samplesRemainingInAi = EXTRA_BUFFERED_AI_SAMPLES_TARGET;
    }
#endif

    if (gAudioCtx.resetTimer < 16) {
        if (gAudioCtx.aiBufLengths[index] != 0) {
#if FISHMOD_DEBUG
            if (gFishmodAudioLockstep) {
                Fishmod_AudLog_Buffer(gAudioCtx.aiBuffers[index], gAudioCtx.aiBufLengths[index]);
            }
#endif
            osAiSetNextBuffer(gAudioCtx.aiBuffers[index], gAudioCtx.aiBufLengths[index] * 4);
            if (gAudioCtx.aiBuffers[index]) {}
            if (gAudioCtx.aiBufLengths[index]) {}
        }
    }

#if FISHMOD_EXPAK
    if (sFishmodAhead == 0) // (an update built ahead had its call before it was built)
#endif
    if (gAudioCustomUpdateFunction != NULL) {
        gAudioCustomUpdateFunction();
    }

    {
        AudioTask* rspTask;
        s32 aiLen = 0;
        s32 cmdsTaken = 0;
#if FISHMOD_EXPAK
        if (sFishmodAhead != 0) {
            // this refresh's update was built with an earlier one's: only its sample buffers' countdowns are due
            // (the game's custom update function ran before it was built: below)
            AudioLoad_DecreaseSampleDmaTtls();
            rspTask = sFishmodAheadTask[0];
            sFishmodAheadTask[0] = sFishmodAheadTask[1];
            sFishmodAhead--;
            gFishmodBuiltCount = 0;
            return rspTask;
        }
#endif
#if FISHMOD_EXPAK
        // (decided before the update is built, for its filters' copies: the game's commands are waiting, so the
        // next two refreshes' updates will follow it if the driver is still steady after)
        sFishmodWillBatch = AudioThread_FishmodSteady() && (MQ_GET_COUNT(gAudioCtx.threadCmdProcQueueP) != 0);
#endif
        rspTask = AudioThread_Body(samplesRemainingInAi, gAudioCtx.totalTaskCount, 0, &aiLen, &cmdsTaken);
#if FISHMOD_EXPAK
        // the next two refreshes' updates too, while this one's code and data are in the caches: the same updates
        // (AudioThread_Body), only made early. Only after an update that took commands from the game: the game sends
        // them once a frame, three refreshes, so the two refreshes after are the ones it will have nothing new for,
        // and what it does send is heard as soon as it ever was. Each is given what the audio interface will hold by
        // its refresh: what it holds now, plus what was just made, less a refresh's playing.
        while (sFishmodWillBatch && (cmdsTaken != 0) && (sFishmodAhead < 2) && AudioThread_FishmodSteady()) {
            s32 cmdsAhead;

            samplesRemainingInAi += aiLen;
            j = (gAudioCtx.audioBufferParameters.aiSamplingFrequency * 1001 + 30000) / 60000;
            samplesRemainingInAi = (samplesRemainingInAi > (u32)j) ? samplesRemainingInAi - j : 0;
#if FISHMOD_DEBUG
            if (gFishmodAudioLockstep) {
                samplesRemainingInAi = EXTRA_BUFFERED_AI_SAMPLES_TARGET;
            }
#endif
            if (gAudioCustomUpdateFunction != NULL) {
                gAudioCustomUpdateFunction(); // (once an update, before it: as on its own refresh)
            }
            // (the update's own number, for whatever in it reads the count: the sequences' random numbers do,
            // AudioThread_NextRandom. The game's thread can't see it meanwhile: this one outranks it.)
            gAudioCtx.totalTaskCount += sFishmodAhead + 1;
            sFishmodAheadTask[sFishmodAhead] =
                AudioThread_Body(samplesRemainingInAi, gAudioCtx.totalTaskCount, sFishmodAhead + 1, &aiLen, &cmdsAhead);
            gAudioCtx.totalTaskCount -= sFishmodAhead + 1;
            sFishmodAhead++;
        }
        sFishmodWillBatch = false;
        gFishmodBuilt[0] = rspTask;
        gFishmodBuilt[1] = sFishmodAheadTask[0];
        gFishmodBuilt[2] = sFishmodAheadTask[1];
        gFishmodBuiltCount = 1 + sFishmodAhead;
        gFishmodBuiltDmas = (gAudioCtx.curAudioFrameDmaCount > 255) ? 255 : gAudioCtx.curAudioFrameDmaCount;
#endif
        gFishmodDmaTtlAhead = 0;
        return rspTask;
    }
}

/**
 * original name: Nap_AudioSysProcess
 */
void AudioThread_ProcessGlobalCmd(AudioCmd* cmd) {
    s32 i;
    STACK_PADS(s32, 3);
    u32 flags;

    switch (cmd->op) {
        case AUDIOCMD_OP_GLOBAL_SYNC_LOAD_SEQ_PARTS:
            AudioLoad_SyncLoadSeqParts(cmd->arg1, cmd->arg2);
            break;

        case AUDIOCMD_OP_GLOBAL_INIT_SEQPLAYER:
            AudioLoad_SyncInitSeqPlayer(cmd->arg0, cmd->arg1, cmd->arg2);
            AudioThread_SetFadeInTimer(cmd->arg0, cmd->asInt);
            break;

        case AUDIOCMD_OP_GLOBAL_INIT_SEQPLAYER_SKIP_TICKS:
            AudioLoad_SyncInitSeqPlayerSkipTicks(cmd->arg0, cmd->arg1, cmd->asInt);
            break;

        case AUDIOCMD_OP_GLOBAL_DISABLE_SEQPLAYER:
            if (gAudioCtx.seqPlayers[cmd->arg0].enabled) {
                if (cmd->asInt == 0) {
                    AudioSeq_SequencePlayerDisableAsFinished(&gAudioCtx.seqPlayers[cmd->arg0]);
                } else {
                    AudioThread_SetFadeOutTimer(cmd->arg0, cmd->asInt);
                }
            }
            break;

        case AUDIOCMD_OP_GLOBAL_SET_SOUND_OUTPUT_MODE:
            gAudioCtx.soundOutputMode = cmd->asUInt;
            break;

        case AUDIOCMD_OP_GLOBAL_MUTE:
            for (i = 0; i < gAudioCtx.audioBufferParameters.numSequencePlayers; i++) {
                SequencePlayer* seqPlayer = &gAudioCtx.seqPlayers[i];

                seqPlayer->muted = true;
                seqPlayer->recalculateVolume = true;
            }
            break;

        case AUDIOCMD_OP_GLOBAL_UNMUTE:
            if (cmd->asUInt == 1) {
                for (i = 0; i < gAudioCtx.numNotes; i++) {
                    Note* note = &gAudioCtx.notes[i];
                    NoteSampleState* sampleState = &note->sampleState;

                    if (sampleState->bitField0.enabled && (note->playbackState.unk_04 == 0) &&
                        (note->playbackState.parentLayer->channel->muteBehavior & MUTE_BEHAVIOR_3)) {
                        sampleState->bitField0.finished = true;
                    }
                }
            }

            for (i = 0; i < gAudioCtx.audioBufferParameters.numSequencePlayers; i++) {
                SequencePlayer* seqPlayer = &gAudioCtx.seqPlayers[i];

                seqPlayer->muted = false;
                seqPlayer->recalculateVolume = true;
            }
            break;

        case AUDIOCMD_OP_GLOBAL_SYNC_LOAD_INSTRUMENT:
            AudioLoad_SyncLoadInstrument(cmd->arg0, cmd->arg1, cmd->arg2);
            break;

        case AUDIOCMD_OP_GLOBAL_ASYNC_LOAD_SAMPLE_BANK:
            AudioLoad_AsyncLoadSampleBank(cmd->arg0, cmd->arg1, cmd->arg2, &gAudioCtx.externalLoadQueue);
            break;

        case AUDIOCMD_OP_GLOBAL_ASYNC_LOAD_FONT:
            AudioLoad_AsyncLoadFont(cmd->arg0, cmd->arg1, cmd->arg2, &gAudioCtx.externalLoadQueue);
            break;

        case AUDIOCMD_OP_GLOBAL_ASYNC_LOAD_SEQ:
            AudioLoad_AsyncLoadSeq(cmd->arg0, cmd->arg1, cmd->arg2, &gAudioCtx.externalLoadQueue);
            break;

        case AUDIOCMD_OP_GLOBAL_DISCARD_SEQ_FONTS:
            AudioLoad_DiscardSeqFonts(cmd->arg1);
            break;

        case AUDIOCMD_OP_GLOBAL_SET_CHANNEL_MASK:
            gAudioCtx.threadCmdChannelMask[cmd->arg0] = cmd->asUShort;
            break;

        case AUDIOCMD_OP_GLOBAL_RESET_AUDIO_HEAP:
            gAudioCtx.resetStatus = 5;
            gAudioCtx.specId = cmd->asUInt;
            break;

        case AUDIOCMD_OP_GLOBAL_SET_CUSTOM_UPDATE_FUNCTION:
            gAudioCustomUpdateFunction = (AudioCustomUpdateFunction)cmd->asUInt;
            break;

        case AUDIOCMD_OP_GLOBAL_SET_DRUM_FONT:
        case AUDIOCMD_OP_GLOBAL_SET_SFX_FONT:
        case AUDIOCMD_OP_GLOBAL_SET_INSTRUMENT_FONT:
            Audio_SetFontInstrument(cmd->op - AUDIOCMD_OP_GLOBAL_SET_DRUM_FONT, cmd->arg0, cmd->arg1, cmd->data);
            break;

        case AUDIOCMD_OP_GLOBAL_DISABLE_ALL_SEQPLAYERS:
            flags = cmd->asUInt;
            if (flags == 1) {
                for (i = 0; i < gAudioCtx.audioBufferParameters.numSequencePlayers; i++) {
                    SequencePlayer* seqPlayer = &gAudioCtx.seqPlayers[i];

                    if (seqPlayer->enabled) {
                        AudioSeq_SequencePlayerDisableAsFinished(seqPlayer);
                    }
                }
            }
            func_800E66C0(flags);
            break;

        case AUDIOCMD_OP_GLOBAL_POP_PERSISTENT_CACHE:
            AudioHeap_PopPersistentCache(cmd->asInt);
            break;

        default:
            break;
    }
}

/**
 * original name: __Nas_GroupFadeOut
 */
void AudioThread_SetFadeOutTimer(s32 seqPlayerIndex, s32 fadeTimer) {
    SequencePlayer* seqPlayer = &gAudioCtx.seqPlayers[seqPlayerIndex];

    if (fadeTimer == 0) {
        fadeTimer = 1;
    }

    seqPlayer->fadeVelocity = -(seqPlayer->fadeVolume / fadeTimer);
    seqPlayer->state = 2;
    seqPlayer->fadeTimer = fadeTimer;
}

/**
 * original name: __Nas_GroupFadeIn
 */
void AudioThread_SetFadeInTimer(s32 seqPlayerIndex, s32 fadeTimer) {
    SequencePlayer* seqPlayer;

    if (fadeTimer != 0) {
        seqPlayer = &gAudioCtx.seqPlayers[seqPlayerIndex];
        seqPlayer->state = 1;
        seqPlayer->fadeTimerUnkEu = fadeTimer;
        seqPlayer->fadeTimer = fadeTimer;
        seqPlayer->fadeVolume = 0.0f;
        seqPlayer->fadeVelocity = 0.0f;
    }
}

/**
 * original name: Nap_AudioPortInit
 */
void AudioThread_InitMesgQueuesImpl(void) {
    gAudioCtx.threadCmdWritePos = 0;
    gAudioCtx.threadCmdReadPos = 0;
    gAudioCtx.threadCmdQueueFinished = false;

    gAudioCtx.taskStartQueueP = &gAudioCtx.taskStartQueue;
    gAudioCtx.threadCmdProcQueueP = &gAudioCtx.threadCmdProcQueue;
    gAudioCtx.audioResetQueueP = &gAudioCtx.audioResetQueue;

    osCreateMesgQueue(gAudioCtx.taskStartQueueP, gAudioCtx.taskStartMsgBuf, ARRAY_COUNT(gAudioCtx.taskStartMsgBuf));
    osCreateMesgQueue(gAudioCtx.threadCmdProcQueueP, gAudioCtx.threadCmdProcMsgBuf,
                      ARRAY_COUNT(gAudioCtx.threadCmdProcMsgBuf));
    osCreateMesgQueue(gAudioCtx.audioResetQueueP, gAudioCtx.audioResetMsgBuf, ARRAY_COUNT(gAudioCtx.audioResetMsgBuf));
}

/**
 * original name: Nap_PortSet
 */
void AudioThread_QueueCmd(u32 opArgs, void** data) {
    AudioCmd* cmd = &gAudioCtx.threadCmdBuf[gAudioCtx.threadCmdWritePos & 0xFF];

    cmd->opArgs = opArgs;
    cmd->data = *data;

    gAudioCtx.threadCmdWritePos++;

    if (gAudioCtx.threadCmdWritePos == gAudioCtx.threadCmdReadPos) {
        gAudioCtx.threadCmdWritePos--;
    }
}

/**
 * original name: Nap_SetF32
 */
void AudioThread_QueueCmdF32(u32 opArgs, f32 data) {
    AudioThread_QueueCmd(opArgs, (void**)&data);
}

/**
 * original name: Nap_SetS32
 */
void AudioThread_QueueCmdS32(u32 opArgs, s32 data) {
    AudioThread_QueueCmd(opArgs, (void**)&data);
}

/**
 * original name: Nap_SetS8
 */
void AudioThread_QueueCmdS8(u32 opArgs, s8 data) {
    u32 uData = data << 0x18;

    AudioThread_QueueCmd(opArgs, (void**)&uData);
}

/**
 * original name: Nap_SetU16
 */
void AudioThread_QueueCmdU16(u32 opArgs, u16 data) {
    u32 uData = data << 0x10;

    AudioThread_QueueCmd(opArgs, (void**)&uData);
}

/**
 * original name: Nap_SendStart
 */
s32 AudioThread_ScheduleProcessCmds(void) {
    static s32 D_801304E8 = 0;
    s32 ret;

    if (D_801304E8 < (u8)((gAudioCtx.threadCmdWritePos - gAudioCtx.threadCmdReadPos) + 0x100)) {
        D_801304E8 = (u8)((gAudioCtx.threadCmdWritePos - gAudioCtx.threadCmdReadPos) + 0x100);
    }

    ret = osSendMesg(gAudioCtx.threadCmdProcQueueP,
                     (OSMesg)(((gAudioCtx.threadCmdReadPos & 0xFF) << 8) | (gAudioCtx.threadCmdWritePos & 0xFF)),
                     OS_MESG_NOBLOCK);
    if (ret != -1) {
        gAudioCtx.threadCmdReadPos = gAudioCtx.threadCmdWritePos;
        ret = 0;
    } else {
        return -1;
    }

    return ret;
}

/**
 * original name: Nap_FlushPort
 */
void AudioThread_ResetCmdQueue(void) {
    gAudioCtx.threadCmdQueueFinished = false;
    gAudioCtx.threadCmdReadPos = gAudioCtx.threadCmdWritePos;
}

/**
 * original name: Nap_Process1Command
 */
void AudioThread_ProcessCmd(AudioCmd* cmd) {
    SequencePlayer* seqPlayer;
    u16 threadCmdChannelMask;
    s32 channelIndex;

    if ((cmd->op & 0xF0) == 0xF0) {
        AudioThread_ProcessGlobalCmd(cmd);
        return;
    }

    if (cmd->arg0 < gAudioCtx.audioBufferParameters.numSequencePlayers) {
        seqPlayer = &gAudioCtx.seqPlayers[cmd->arg0];
        if (cmd->op & 0x80) {
            AudioThread_ProcessGlobalCmd(cmd);
            return;
        }
        if (cmd->op & 0x40) {
            AudioThread_ProcessSeqPlayerCmd(seqPlayer, cmd);
            return;
        }

        if (cmd->arg1 < SEQ_NUM_CHANNELS) {
            AudioThread_ProcessChannelCmd(seqPlayer->channels[cmd->arg1], cmd);
            return;
        }
        if (cmd->arg1 == AUDIOCMD_ALL_CHANNELS) {
            threadCmdChannelMask = gAudioCtx.threadCmdChannelMask[cmd->arg0];
            for (channelIndex = 0; channelIndex < SEQ_NUM_CHANNELS; channelIndex++) {
                if (threadCmdChannelMask & 1) {
                    AudioThread_ProcessChannelCmd(seqPlayer->channels[channelIndex], cmd);
                }
                threadCmdChannelMask = threadCmdChannelMask >> 1;
            }
        }
    }
}

/**
 * original name: Nap_AudioPortProcess
 */
void AudioThread_ProcessCmds(u32 msg) {
    static u8 sCurCmdRdPos = 0;
    AudioCmd* cmd;
    u8 endPos;

    if (!gAudioCtx.threadCmdQueueFinished) {
        sCurCmdRdPos = msg >> 8;
    }

    while (true) {
        endPos = msg & 0xFF;
        if (sCurCmdRdPos == endPos) {
            gAudioCtx.threadCmdQueueFinished = false;
            return;
        }

        cmd = &gAudioCtx.threadCmdBuf[sCurCmdRdPos++ & 0xFF];
        if (cmd->op == AUDIOCMD_OP_GLOBAL_STOP_AUDIOCMDS) {
            gAudioCtx.threadCmdQueueFinished = true;
            return;
        }

        AudioThread_ProcessCmd(cmd);
        cmd->op = AUDIOCMD_OP_NOOP;
    }
}

u32 func_800E5E20(u32* out) {
    u32 sp1C;

    if (osRecvMesg(&gAudioCtx.externalLoadQueue, (OSMesg*)&sp1C, OS_MESG_NOBLOCK) == -1) {
        *out = 0;
        return 0;
    }
    *out = sp1C & 0xFFFFFF;
    return sp1C >> 0x18;
}

u8* AudioThread_GetFontsForSequence(s32 seqId, u32* outNumFonts) {
    return AudioLoad_GetFontsForSequence(seqId, outNumFonts);
}

void Audio_GetSampleBankIdsOfFont(s32 fontId, u32* sampleBankId1, u32* sampleBankId2) {
    *sampleBankId1 = gAudioCtx.soundFontList[fontId].sampleBankId1;
    *sampleBankId2 = gAudioCtx.soundFontList[fontId].sampleBankId2;
}

/**
 * original name: Nap_CheckSpecChange
 */
s32 func_800E5EDC(void) {
    STACK_PAD(s32);
    s32 specId;

    if (osRecvMesg(gAudioCtx.audioResetQueueP, (OSMesg*)&specId, OS_MESG_NOBLOCK) == -1) {
        return 0;
    } else if (gAudioCtx.specId != specId) {
        return -1;
    } else {
        return 1;
    }
}

/**
 * original name: __ClearSpecChangeQ
 */
void func_800E5F34(void) {
    // macro?
    // clang-format off
    s32 chk = -1; OSMesg msg; do {} while (osRecvMesg(gAudioCtx.audioResetQueueP, &msg, OS_MESG_NOBLOCK) != chk);
    // clang-format on
}

/**
 * original name: Nap_StartSpecChange
 */
s32 AudioThread_ResetAudioHeap(s32 specId) {
    s32 resetStatus;
    OSMesg msg;
    STACK_PAD(s32);

    func_800E5F34();
    resetStatus = gAudioCtx.resetStatus;
    if (resetStatus != 0) {
        AudioThread_ResetCmdQueue();
        if (gAudioCtx.specId == specId) {
            return -2;
        } else if (resetStatus > 2) {
            gAudioCtx.specId = specId;
            return -3;
        } else {
            osRecvMesg(gAudioCtx.audioResetQueueP, &msg, OS_MESG_BLOCK);
        }
    }

    func_800E5F34();
    AUDIOCMD_GLOBAL_RESET_AUDIO_HEAP(specId);

    return AudioThread_ScheduleProcessCmds();
}

/**
 * original name: Nap_StartReset
 */
void AudioThread_PreNMIInternal(void) {
    gAudioCtx.resetTimer = 1;
    if (gAudioContextInitialized) {
        AudioThread_ResetAudioHeap(0);
        gAudioCtx.resetStatus = 0;
    }
}

/**
 * original name: Nap_ReadSubPort
 */
s8 AudioThread_GetChannelIO(s32 seqPlayerIndex, s32 channelIndex, s32 ioPort) {
    SequencePlayer* seqPlayer = &gAudioCtx.seqPlayers[seqPlayerIndex];
    SequenceChannel* channel;

    if (seqPlayer->enabled) {
        channel = seqPlayer->channels[channelIndex];
        return channel->seqScriptIO[ioPort];
    } else {
        return SEQ_IO_VAL_NONE;
    }
}

/**
 * original name: Nap_ReadGrpPort
 */
s8 AudioThread_GetSeqPlayerIO(s32 seqPlayerIndex, s32 ioPort) {
    return gAudioCtx.seqPlayers[seqPlayerIndex].seqScriptIO[ioPort];
}

void AudioThread_InitExternalPool(void* ramAddr, u32 size) {
    AudioHeap_InitPool(&gAudioCtx.externalPool, ramAddr, size);
}

void AudioThread_ResetExternalPool(void) {
    gAudioCtx.externalPool.startRamAddr = NULL;
}

/**
 * original name: __SetGrpParam
 */
void AudioThread_ProcessSeqPlayerCmd(SequencePlayer* seqPlayer, AudioCmd* cmd) {
    f32 fadeVolume;

    switch (cmd->op) {
        case AUDIOCMD_OP_SEQPLAYER_FADE_VOLUME_SCALE:
            if (seqPlayer->fadeVolumeScale != cmd->asFloat) {
                seqPlayer->fadeVolumeScale = cmd->asFloat;
                seqPlayer->recalculateVolume = true;
            }
            break;

        case AUDIOCMD_OP_SEQPLAYER_SET_TEMPO:
            seqPlayer->tempo = cmd->asInt * SEQTICKS_PER_BEAT;
            break;

        case AUDIOCMD_OP_SEQPLAYER_CHANGE_TEMPO:
            seqPlayer->tempoChange = cmd->asInt * SEQTICKS_PER_BEAT;
            break;

        case AUDIOCMD_OP_SEQPLAYER_CHANGE_TEMPO_SEQTICKS:
            seqPlayer->tempoChange = cmd->asInt;
            break;

        case AUDIOCMD_OP_SEQPLAYER_SET_TRANSPOSITION:
            seqPlayer->transposition = cmd->asSbyte;
            break;

        case AUDIOCMD_OP_SEQPLAYER_SET_IO:
            seqPlayer->seqScriptIO[cmd->arg2] = cmd->asSbyte;
            break;

        case AUDIOCMD_OP_SEQPLAYER_FADE_TO_SET_VOLUME:
            fadeVolume = (s32)cmd->arg1 / 127.0f;
            goto apply_fade;

        case AUDIOCMD_OP_SEQPLAYER_FADE_TO_SCALED_VOLUME:
            fadeVolume = ((s32)cmd->arg1 / 100.0f) * seqPlayer->fadeVolume;
        apply_fade:
            if (seqPlayer->state != 2) {
                seqPlayer->volume = seqPlayer->fadeVolume;
                if (cmd->asInt == 0) {
                    seqPlayer->fadeVolume = fadeVolume;
                } else {
                    s32 fadeTimer = cmd->asInt;

                    seqPlayer->state = 0;
                    seqPlayer->fadeTimer = fadeTimer;
                    seqPlayer->fadeVelocity = (fadeVolume - seqPlayer->fadeVolume) / fadeTimer;
                }
            }
            break;

        case AUDIOCMD_OP_SEQPLAYER_RESET_VOLUME:
            if (seqPlayer->state != 2) {
                if (cmd->asInt == 0) {
                    seqPlayer->fadeVolume = seqPlayer->volume;
                } else {
                    s32 fadeTimer = cmd->asInt;

                    seqPlayer->state = 0;
                    seqPlayer->fadeTimer = fadeTimer;
                    seqPlayer->fadeVelocity = (seqPlayer->volume - seqPlayer->fadeVolume) / fadeTimer;
                }
            }
            break;

        case AUDIOCMD_OP_SEQPLAYER_SET_BEND:
            seqPlayer->bend = cmd->asFloat;
            if (seqPlayer->bend == 1.0f) {
                seqPlayer->applyBend = false;
            } else {
                seqPlayer->applyBend = true;
            }
            break;

        default:
            break;
    }
}

/**
 * original name: __SetSubParam
 */
void AudioThread_ProcessChannelCmd(SequenceChannel* channel, AudioCmd* cmd) {
    switch (cmd->op) {
        case AUDIOCMD_OP_CHANNEL_SET_VOL_SCALE:
            if (channel->volumeScale != cmd->asFloat) {
                channel->volumeScale = cmd->asFloat;
                channel->changes.s.volume = true;
            }
            break;

        case AUDIOCMD_OP_CHANNEL_SET_VOL:
            if (channel->volume != cmd->asFloat) {
                channel->volume = cmd->asFloat;
                channel->changes.s.volume = true;
            }
            break;

        case AUDIOCMD_OP_CHANNEL_SET_PAN:
            if (channel->newPan != cmd->asSbyte) {
                channel->newPan = cmd->asSbyte;
                channel->changes.s.pan = true;
            }
            break;

        case AUDIOCMD_OP_CHANNEL_SET_PAN_WEIGHT:
            //! @bug Should compare `asSbyte` to `panChannelWeight`
            if (channel->newPan != cmd->asSbyte) {
                channel->panChannelWeight = cmd->asSbyte;
                channel->changes.s.pan = true;
            }
            break;

        case AUDIOCMD_OP_CHANNEL_SET_FREQ_SCALE:
            if (channel->freqScale != cmd->asFloat) {
                channel->freqScale = cmd->asFloat;
                channel->changes.s.freqScale = true;
            }
            break;

        case AUDIOCMD_OP_CHANNEL_SET_REVERB_VOLUME:
            if (channel->targetReverbVol != cmd->asSbyte) {
                channel->targetReverbVol = cmd->asSbyte;
            }
            break;

        case AUDIOCMD_OP_CHANNEL_SET_IO:
            if (cmd->arg2 < ARRAY_COUNT(channel->seqScriptIO)) {
                channel->seqScriptIO[cmd->arg2] = cmd->asSbyte;
            }
            break;

        case AUDIOCMD_OP_CHANNEL_SET_MUTE:
            channel->muted = cmd->asSbyte;
            break;

        case AUDIOCMD_OP_CHANNEL_SET_MUTE_BEHAVIOR:
            channel->muteBehavior = cmd->asSbyte;
            break;

        case AUDIOCMD_OP_CHANNEL_SET_VIBRATO_DEPTH:
            channel->vibratoDepthTarget = cmd->asUbyte * 8;
            channel->vibratoDepthChangeDelay = 1;
            break;

        case AUDIOCMD_OP_CHANNEL_SET_VIBRATO_RATE:
            channel->vibratoRateTarget = cmd->asUbyte * 32;
            channel->vibratoRateChangeDelay = 1;
            break;

        case AUDIOCMD_OP_CHANNEL_SET_COMB_FILTER_SIZE:
            channel->combFilterSize = cmd->asUbyte;
            break;

        case AUDIOCMD_OP_CHANNEL_SET_COMB_FILTER_GAIN:
            channel->combFilterGain = cmd->asUShort;
            break;

        case AUDIOCMD_OP_CHANNEL_SET_STEREO:
            channel->stereo.asByte = cmd->asUbyte;
            break;

        default:
            break;
    }
}

/**
 * Call an audio-thread command that has no code to process it. Unused.
 */
void AudioThread_Noop1Cmd(s32 arg0, s32 arg1, s32 arg2) {
    AUDIOCMD_GLOBAL_NOOP_1(arg0, arg1, arg2, 1);
}

/**
 * Call an audio-thread command that has no code to process it. Unused.
 */
void AudioThread_Noop1CmdZeroed(void) {
    AUDIOCMD_GLOBAL_NOOP_1(0, 0, 0, 0);
}

/**
 * Call an audio-thread command that has no code to process it. Unused.
 */
void AudioThread_Noop2Cmd(u32 arg0, s32 arg1) {
    AUDIOCMD_GLOBAL_NOOP_2(0, 0, arg1, arg0);
}

/**
 * original name: Nap_WaitVsync
 */
void AudioThread_WaitForAudioTask(void) {
    osRecvMesg(gAudioCtx.taskStartQueueP, NULL, OS_MESG_NOBLOCK);
    osRecvMesg(gAudioCtx.taskStartQueueP, NULL, OS_MESG_BLOCK);
}

s32 func_800E6590(s32 seqPlayerIndex, s32 channelIndex, s32 layerIndex) {
    SequencePlayer* seqPlayer;
    SequenceLayer* layer;
    Note* note;
    TunedSample* tunedSample;
    s32 loopEnd;
    s32 samplePos;

    seqPlayer = &gAudioCtx.seqPlayers[seqPlayerIndex];
    if (seqPlayer->enabled && seqPlayer->channels[channelIndex]->enabled) {
        layer = seqPlayer->channels[channelIndex]->layers[layerIndex];
        if (layer == NULL) {
            return 0;
        }

        if (layer->enabled) {
            if (layer->note == NULL) {
                return 0;
            }

            if (!layer->bit3) {
                return 0;
            }

            note = layer->note;
            if (layer == note->playbackState.parentLayer) {
                tunedSample = note->sampleState.tunedSample;
                if (tunedSample == NULL) {
                    return 0;
                }
                loopEnd = tunedSample->sample->loop->header.end;
                samplePos = note->synthesisState.samplePosInt;
                return loopEnd - samplePos;
            }
            return 0;
        }
    }
    return 0;
}

/**
 * original name possibly "Nap_SilenceCheck"
 */
s32 func_800E6680(void) {
    return func_800E66C0(0);
}

void func_800E66A0(void) {
    func_800E66C0(2);
}

/**
 * original name: Nap_SilenceCheck_Inner
 */
s32 func_800E66C0(s32 flags) {
    s32 phi_v1;
    NotePlaybackState* playbackState;
    NoteSampleState* sampleState;
    s32 i;
    Note* note;
    TunedSample* tunedSample;

    phi_v1 = 0;
    for (i = 0; i < gAudioCtx.numNotes; i++) {
        note = &gAudioCtx.notes[i];
        playbackState = &note->playbackState;
        if (note->sampleState.bitField0.enabled) {
            sampleState = &note->sampleState;
            if (playbackState->adsr.action.s.state != 0) {
                if (flags >= 2) {
                    tunedSample = sampleState->tunedSample;
                    if (tunedSample == NULL || sampleState->bitField1.isSyntheticWave) {
                        continue;
                    }
                    if (tunedSample->sample->medium == MEDIUM_RAM) {
                        continue;
                    }
                }

                phi_v1++;
                if ((flags & 1) == 1) {
                    playbackState->adsr.fadeOutVel = gAudioCtx.audioBufferParameters.ticksPerUpdateInv;
                    playbackState->adsr.action.s.release = 1;
                }
            }
        }
    }
    return phi_v1;
}

/**
 * original name: Nap_GetRandom
 */
u32 AudioThread_NextRandom(void) {
    static u32 sAudioRandom = 0x12345678;

#if FISHMOD_DEBUG
    if (gFishmodAudioLockstep) { /* fish-oot: the audio check: no cycle count (it varies between builds) */
        sAudioRandom = (((sAudioRandom * 0x9E3779B1) + 0x1234567) * (sAudioRandom + gAudioCtx.totalTaskCount));
    } else
#endif
    sAudioRandom = ((osGetCount() + 0x1234567) * (sAudioRandom + gAudioCtx.totalTaskCount));
    sAudioRandom += gAudioCtx.audioRandom;

    return sAudioRandom;
}

/**
 * original name: Nas_InitGAudio
 */
void AudioThread_InitMesgQueues(void) {
    AudioThread_InitMesgQueuesImpl();
}

/*
 * fish-oot: the audio manager's retrace handler for a build with the Expansion Pak's memory (--expak). Here, in the
 * audio driver's own files, and reached through a pointer: code/audio_thread_manager.c is then the same code in every
 * build.
 */
#include "audiomgr.h"
#include "regs.h"
#include "sched.h"
#include "speed_meter.h"

#if FISHMOD_EXPAK
/*
 * fish-oot: the audio's RSP tasks sent with time to spare. The game sends an update's task at the next refresh, and
 * the scheduler takes the RSP from the graphics task for it there and then. But the task is ready as soon as its
 * update is built (a refresh earlier; up to three for updates built ahead: audio/internal/thread.c), and its output
 * isn't wanted until the refresh after the one it's sent on. So, with gFishmodAudioLazy, each task is handed over as
 * soon as it's built, marked OS_SC_FISHMOD_LAZY: the scheduler runs it when the RSP is next free (sched.c), and only
 * if it still hasn't run by the refresh the game would have sent it on is it made urgent, as it always was. The
 * tasks are the same and run in the same order; only when they run changes. Without the switch each task goes at the
 * game's time, urgent. (A task whose update started sample loads from the cart waits for its refresh as before: the
 * loads must land first.)
 *
 * Each task has its own scheduler entry here (the game's one would be overwritten while still queued) and they report
 * done on a queue with room for all of them.
 */
u8 gFishmodAudioLazy = false; // off until the console says it pays (the audio_sched_ab bench switches it by lap)
extern OSScTask* volatile gFishmodSchedUrgent;
static OSScTask sFmScTask[FISHMOD_TASK_RING];
static u8 sFmState[FISHMOD_TASK_RING]; // 0: not sent; 1: sent; 2: done
static OSMesgQueue sFmDoneQueue;
static OSMesg sFmDoneMsg[2 * FISHMOD_TASK_RING];
static AudioTask* sFmPrevDue; // the task that was due a refresh ago: its output is queued by this refresh's update

static void AudioMgr_FmSend(AudioMgr* audioMgr, AudioTask* rspTask, s32 lazy) {
    s32 i = rspTask - gAudioCtx.rspTask;
    OSScTask* task = &sFmScTask[i];

    task->next = NULL;
    task->flags = OS_SC_NEEDS_RSP | (lazy ? OS_SC_FISHMOD_LAZY : 0);
    task->framebuffer = NULL;
    task->list = rspTask->task;
    task->msgQueue = &sFmDoneQueue;
    task->msg = (OSMesg)(i + 1);
    sFmState[i] = 1;
    osSendMesg(&audioMgr->sched->cmdQueue, (OSMesg)task, OS_MESG_BLOCK);
    Sched_Notify(audioMgr->sched);
}

static void AudioMgr_FmDone(s32 block, s32 until) {
    OSMesg msg;

    while (((until >= 0) && (sFmState[until] == 1)) || !block) {
        if (osRecvMesg(&sFmDoneQueue, &msg, block ? OS_MESG_BLOCK : OS_MESG_NOBLOCK) == -1) {
            break;
        }
        sFmState[(u32)msg - 1] = 2;
    }
}

static void AudioMgr_FishmodHandleRetrace(AudioMgr* audioMgr) {
    static s32 sInit;
    AudioTask* rspTask;
    AudioTask* due;
    s32 i;

    if (!sInit) {
        osCreateMesgQueue(&sFmDoneQueue, sFmDoneMsg, ARRAY_COUNT(sFmDoneMsg));
        sInit = true;
    }
    AudioMgr_FmDone(false, -1);
    if (sFmPrevDue != NULL) {
        // (its output buffer is the one this refresh's update queues: by now it has always run)
        AudioMgr_FmDone(true, sFmPrevDue - gAudioCtx.rspTask);
    }

    if (R_AUDIOMGR_ACTIVITY_LEVEL > AUDIOMGR_ACTIVITY_LEVEL_ALL) {
        // Inhibit audio rsp task processing
        audioMgr->rspTask = NULL;
    }
    due = audioMgr->rspTask;
    if (due != NULL) {
        // the game's time for this task: sent now, or, sent earlier with time to spare and still waiting, made urgent
        i = due - gAudioCtx.rspTask;
        if (sFmState[i] == 0) {
            AudioMgr_FmSend(audioMgr, due, false);
        } else if (sFmState[i] == 1) {
            gFishmodSchedUrgent = &sFmScTask[i];
            Sched_Notify(audioMgr->sched);
        }
    }
    sFmPrevDue = due;

    // Update the audio driver
    gAudioThreadUpdateTimeStart = osGetTime();
    if (R_AUDIOMGR_ACTIVITY_LEVEL >= AUDIOMGR_ACTIVITY_LEVEL_NO_UPDATE) {
        rspTask = NULL;
        gFishmodBuiltCount = 0;
    } else {
        rspTask = AudioThread_Update();
    }
    gAudioThreadUpdateTimeAcc += osGetTime() - gAudioThreadUpdateTimeStart;
    gAudioThreadUpdateTimeStart = 0;

    // the tasks just built: not sent yet; and, with time to spare, sent now for the RSP's next gap
    for (i = 0; i < gFishmodBuiltCount; i++) {
        if (gFishmodBuilt[i] != NULL) {
            sFmState[gFishmodBuilt[i] - gAudioCtx.rspTask] = 0;
        }
    }
    if (gFishmodAudioLazy && (R_AUDIOMGR_ACTIVITY_LEVEL == AUDIOMGR_ACTIVITY_LEVEL_ALL) && (gFishmodBuiltDmas == 0)) {
        for (i = 0; (i < gFishmodBuiltCount) && (gFishmodBuilt[i] != NULL); i++) {
            AudioMgr_FmSend(audioMgr, gFishmodBuilt[i], true);
        }
    }

    // Update rsp task to be scheduled on next retrace
    audioMgr->rspTask = rspTask;
}
#endif

#if FISHMOD_EXPAK
void (*gFishmodAudioMgrRetrace)(AudioMgr*) = AudioMgr_FishmodHandleRetrace;
#else
void (*gFishmodAudioMgrRetrace)(AudioMgr*) = NULL;
#endif
