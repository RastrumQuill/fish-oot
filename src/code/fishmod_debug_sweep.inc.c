// The audio sweep's legs other than the scenes (fishmod/docs/audio-sweep.md), played standing in the start scene.
// Which leg a run plays is in the ROM's header with its other options (fishmod_debug.c, FISHMOD_SWEEP_SCENES):
//   1 sequences: every sequence in the table on the music player, a fanfare over some, faded out; then every
//     nature ambience
//   2 effects: every sound effect of the seven banks, round and away from the camera, with bursts of 24 at once
//   3 ocarina: played from the controller (the ocarina reads the controller itself: its read is handed these
//     inputs, audio/game/general.c), free notes bent and shaken, every song played right and played back, other
//     instruments, the scarecrow's songs recorded and played back, the memory game
#include "audio.h"
#include "ocarina.h"
#include "sfx.h"

#undef DEFINE_SFX
#define DEFINE_SFX(_0, enum, _2, _3, _4, _5) enum,
static const u16 sSweepSfxIds[] = {
#include "tables/sfx/playerbank_table.h"
#include "tables/sfx/itembank_table.h"
#include "tables/sfx/environmentbank_table.h"
#include "tables/sfx/enemybank_table.h"
#include "tables/sfx/systembank_table.h"
#include "tables/sfx/ocarinabank_table.h"
#include "tables/sfx/voicebank_table.h"
};
#undef DEFINE_SFX

#define SWEEP_SEQ_FIRST 2 // (0 is the sound effects' own sequence, 1 the nature ambience's, played below by its ids)
#define SWEEP_SEQ_COUNT (0x6E - SWEEP_SEQ_FIRST)
#define SWEEP_NATURE_COUNT 0x13
#define SWEEP_SEQ_FRAMES 240
#define SWEEP_NATURE_FRAMES 100

extern void (*gFishmodOcarinaPad)(Input* input);
static u16 sSweepOcaButtons;
static s8 sSweepOcaStickX;
static s8 sSweepOcaStickY;

static void Sweep_OcarinaPad(Input* input) {
    input->cur.button = sSweepOcaButtons;
    input->rel.stick_x = sSweepOcaStickX;
    input->rel.stick_y = sSweepOcaStickY;
}

typedef enum SweepOcaAct {
    OCA_NONE,
    OCA_INSTRUMENT, // arg: the instrument (0 off)
    OCA_START,      // arg: AudioOcarina_Start's flags
    OCA_PLAYBACK,   // arg: song + 1 (0 stops)
    OCA_RECORD,     // arg: the recording state
    OCA_MEMORY,     // arg: 0 a new game, 1 three more notes
    OCA_LEG         // arg: the leg's mark
} SweepOcaAct;

typedef struct SweepOcaStep {
    u8 frames;
    u8 act;
    u16 arg;
    u16 buttons;
    s8 stickX;
    s8 stickY;
} SweepOcaStep;

#define OA BTN_A
#define OD BTN_CDOWN
#define OR BTN_CRIGHT
#define OL BTN_CLEFT
#define OU BTN_CUP
#define NOTE(b) { 7, OCA_NONE, 0, b, 0, 0 }, { 3, OCA_NONE, 0, 0, 0, 0 }
#define BENT(b, x, y) { 10, OCA_NONE, 0, b, x, y }, { 3, OCA_NONE, 0, 0, 0, 0 }
#define REST(n) { n, OCA_NONE, 0, 0, 0, 0 }
#define DO(act, arg) { 1, act, arg, 0, 0, 0 }
#define SONG(i, ...) DO(OCA_LEG, 0x3010 + (i)), DO(OCA_START, 0x0FFF), REST(5), __VA_ARGS__, REST(40), \
    DO(OCA_PLAYBACK, (i) + 1), REST(180), DO(OCA_PLAYBACK, 0), REST(10)
static const SweepOcaStep sSweepOcaSteps[] = {
    DO(OCA_LEG, 0x3001), DO(OCA_INSTRUMENT, OCARINA_INSTRUMENT_DEFAULT), DO(OCA_START, 0x0FFF), REST(10),
    NOTE(OA), NOTE(OD), NOTE(OR), NOTE(OL), NOTE(OU),
    BENT(OA, 0, 70), BENT(OD, 0, -70), BENT(OR, 70, 0), BENT(OL, -70, 0), BENT(OU, 50, 50), BENT(OA, -50, -50),
    BENT(OA | BTN_R, 0, 0), BENT(OD | BTN_Z, 0, 0), BENT(OR | BTN_R, 0, 30), BENT(OL | BTN_Z, 30, 0),
    BENT(OU | BTN_R | BTN_Z, 0, 0), BENT(OR | OL, 0, 0), BENT(OA | OU, 0, 0),
    { 40, OCA_NONE, 0, OA, 0, 0 }, { 20, OCA_NONE, 0, OA, 0, 60 }, { 20, OCA_NONE, 0, OA, 60, -60 }, REST(20),
    SONG(OCARINA_SONG_MINUET, NOTE(OA), NOTE(OU), NOTE(OL), NOTE(OR), NOTE(OL), NOTE(OR)),
    SONG(OCARINA_SONG_BOLERO, NOTE(OD), NOTE(OA), NOTE(OD), NOTE(OA), NOTE(OR), NOTE(OD), NOTE(OR), NOTE(OD)),
    SONG(OCARINA_SONG_SERENADE, NOTE(OA), NOTE(OD), NOTE(OR), NOTE(OR), NOTE(OL)),
    SONG(OCARINA_SONG_REQUIEM, NOTE(OA), NOTE(OD), NOTE(OA), NOTE(OR), NOTE(OD), NOTE(OA)),
    SONG(OCARINA_SONG_NOCTURNE, NOTE(OL), NOTE(OR), NOTE(OR), NOTE(OA), NOTE(OL), NOTE(OR), NOTE(OD)),
    SONG(OCARINA_SONG_PRELUDE, NOTE(OU), NOTE(OR), NOTE(OU), NOTE(OR), NOTE(OL), NOTE(OU)),
    SONG(OCARINA_SONG_SARIAS, NOTE(OD), NOTE(OR), NOTE(OL), NOTE(OD), NOTE(OR), NOTE(OL)),
    SONG(OCARINA_SONG_EPONAS, NOTE(OU), NOTE(OL), NOTE(OR), NOTE(OU), NOTE(OL), NOTE(OR)),
    SONG(OCARINA_SONG_LULLABY, NOTE(OL), NOTE(OU), NOTE(OR), NOTE(OL), NOTE(OU), NOTE(OR)),
    SONG(OCARINA_SONG_SUNS, NOTE(OR), NOTE(OD), NOTE(OU), NOTE(OR), NOTE(OD), NOTE(OU)),
    SONG(OCARINA_SONG_TIME, NOTE(OR), NOTE(OA), NOTE(OD), NOTE(OR), NOTE(OA), NOTE(OD)),
    SONG(OCARINA_SONG_STORMS, NOTE(OA), NOTE(OD), NOTE(OU), NOTE(OA), NOTE(OD), NOTE(OU)),
    // a song that is none of them, and the songs without the staff (free play that still listens)
    DO(OCA_LEG, 0x3030), DO(OCA_START, 0x0FFF), REST(5),
    NOTE(OA), NOTE(OA), NOTE(OU), NOTE(OU), NOTE(OD), NOTE(OL), NOTE(OA), NOTE(OR), REST(30),
    DO(OCA_START, 0x4FFF), REST(5), NOTE(OL), NOTE(OU), NOTE(OR), NOTE(OL), NOTE(OU), NOTE(OR), REST(30),
    // the other instruments
    DO(OCA_LEG, 0x3040), DO(OCA_INSTRUMENT, 0), REST(5), DO(OCA_INSTRUMENT, OCARINA_INSTRUMENT_MALON), DO(OCA_START, 0x0FFF),
    NOTE(OA), NOTE(OD), NOTE(OR), NOTE(OL), NOTE(OU), REST(10),
    DO(OCA_INSTRUMENT, 0), REST(5), DO(OCA_INSTRUMENT, OCARINA_INSTRUMENT_WHISTLE), DO(OCA_START, 0x0FFF),
    NOTE(OA), NOTE(OD), NOTE(OR), NOTE(OL), NOTE(OU), REST(10),
    DO(OCA_INSTRUMENT, 0), REST(5), DO(OCA_INSTRUMENT, OCARINA_INSTRUMENT_HARP), DO(OCA_START, 0x0FFF),
    NOTE(OA), NOTE(OD), NOTE(OR), NOTE(OL), NOTE(OU), REST(10),
    DO(OCA_INSTRUMENT, 0), REST(5), DO(OCA_INSTRUMENT, OCARINA_INSTRUMENT_GRIND_ORGAN), DO(OCA_START, 0x0FFF),
    NOTE(OA), NOTE(OD), NOTE(OR), NOTE(OL), NOTE(OU), REST(10),
    DO(OCA_INSTRUMENT, 0), REST(5), DO(OCA_INSTRUMENT, OCARINA_INSTRUMENT_FLUTE), DO(OCA_START, 0x0FFF),
    NOTE(OA), NOTE(OD), NOTE(OR), NOTE(OL), NOTE(OU), REST(10),
    // the scarecrow's long song recorded and played back, then his eight-note song
    DO(OCA_LEG, 0x3050), DO(OCA_INSTRUMENT, 0), REST(5), DO(OCA_INSTRUMENT, OCARINA_INSTRUMENT_DEFAULT),
    DO(OCA_START, 0xA000), DO(OCA_RECORD, OCARINA_RECORD_SCARECROW_LONG), REST(5),
    NOTE(OA), BENT(OD, 0, 60), NOTE(OR), BENT(OL, 60, 0), NOTE(OU), NOTE(OA), BENT(OU | BTN_R, 0, 0), NOTE(OD), REST(60),
    DO(OCA_RECORD, OCARINA_RECORD_OFF), REST(10), DO(OCA_PLAYBACK, OCARINA_SONG_SCARECROW_LONG + 1), REST(200),
    DO(OCA_PLAYBACK, 0), REST(10),
    DO(OCA_START, 0xA000), DO(OCA_RECORD, OCARINA_RECORD_SCARECROW_SPAWN), REST(5),
    NOTE(OA), NOTE(OD), NOTE(OR), NOTE(OL), NOTE(OU), NOTE(OA), NOTE(OR), NOTE(OD), REST(40),
    DO(OCA_RECORD, OCARINA_RECORD_OFF), REST(10), DO(OCA_PLAYBACK, OCARINA_SONG_SCARECROW_SPAWN + 1), REST(150),
    DO(OCA_PLAYBACK, 0), REST(10),
    DO(OCA_START, 0x1FFF), REST(5), NOTE(OA), NOTE(OD), NOTE(OR), NOTE(OL), NOTE(OU), NOTE(OA), NOTE(OR), NOTE(OD), REST(40),
    // the memory game: its song grown twice and played back each time
    DO(OCA_LEG, 0x3060), DO(OCA_MEMORY, 0), DO(OCA_PLAYBACK, OCARINA_SONG_MEMORY_GAME + 1), REST(150), DO(OCA_PLAYBACK, 0),
    DO(OCA_MEMORY, 1), DO(OCA_PLAYBACK, OCARINA_SONG_MEMORY_GAME + 1), REST(250), DO(OCA_PLAYBACK, 0),
    DO(OCA_MEMORY, 1), DO(OCA_PLAYBACK, OCARINA_SONG_MEMORY_GAME + 1), REST(250), DO(OCA_PLAYBACK, 0),
    DO(OCA_INSTRUMENT, 0), REST(60),
};

// The states leg: the calls the game makes to the audio code outside plain "play this sequence, play that effect"
// (battle music, the indoor and underwater filters, pause, fanfares over music, Saria's and Malon's songs by
// distance, the river's and waterfall's pitch, tempo changes, the cutscene flag, the effect-stopping calls, every
// sequence command and every command to the audio thread the game has, a reset into each of the 18 session
// settings, and at the very end the reset button's fade), each with music and effects sounding. They are made
// directly, as the game's own code makes them, so the leg doesn't depend on reaching each place in the game.
#include "audiothread_cmd.h"
void Audio_DisableAllSeq(void);
void Audio_PlaySfxGeneralIfNotInCutscene(u16 sfxId, Vec3f* pos, u8 arg2, f32* freqScale, f32* arg4, s8* reverbAdd);
void Audio_SplitBgmChannels(s8 volSplit);
void func_800F4578(Vec3f* pos, u16 sfxId, f32 arg2);
void func_800F45D0(f32 arg0);
void func_800F7208(void);
enum { SWEEP_STATES_BASE = __COUNTER__ + 1 };
// (while sSeek is set the steps are passed over: that's how the second part starts at the split)
#define STEP                                \
    case (__COUNTER__ - SWEEP_STATES_BASE): \
        if (sSeek) {                        \
            break;                          \
        }
#define WAIT(n)   \
    sWait = (n);  \
    break
// (the step again next frame, n frames in all; sRep counts them)
#define FRAMES(n)          \
    if (++sRep < (n)) {    \
        sStep--;           \
    } else {               \
        sRep = 0;          \
    }                      \
    break
#define MARK(n) gFishmodSweepLeg = 0x4000 + (n)

static void Sweep_StatesCustomUpdate(void) {
}

// The leg is longer than the sound log, so it's run in two parts: the first ends at the split, the second starts
// there (with one part asked for it runs whole).
static s32 Sweep_States(s32 parts, s32 part) {
    static s8 sSeek = -1;
    static s32 sStep;
    static s32 sWait;
    static s32 sRep;
    static Vec3f sP = { 120.0f, 0.0f, 400.0f };
    static Vec3f sQ = { -300.0f, 40.0f, 150.0f };
    static u8 sSemitones;
    static u8 sSpec;
    s32 i;

    if (sSeek < 0) {
        sSeek = (parts > 1) && (part > 0);
    }
    if (sWait > 0) {
        sWait--;
        return false;
    }
    do {
    switch (sStep++) {
        // battle music in and out, standing still
        STEP MARK(0); Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); WAIT(200);
        STEP Audio_SetSequenceMode(SEQ_MODE_ENEMY); Audio_SetBgmEnemyVolume(450.0f); WAIT(60);
        STEP Audio_SetBgmEnemyVolume(450.0f - (sRep * 4.0f)); FRAMES(100);
        STEP Audio_SetSequenceMode(SEQ_MODE_STILL); WAIT(120);
        STEP Audio_SetSequenceMode(SEQ_MODE_DEFAULT); WAIT(120);
        STEP Audio_SetSequenceMode(SEQ_MODE_IGNORE); WAIT(30);
        STEP Audio_SetSequenceMode(SEQ_MODE_DEFAULT); WAIT(30);
        // the filters (indoors, underwater), reverbs, pause
        STEP MARK(1); Audio_SetBaseFilter(0x20); WAIT(60);
        STEP Audio_SetExtraFilter(0x20); WAIT(60);
        STEP Audio_SetBaseFilter(0); WAIT(40);
        STEP Audio_SetExtraFilter(0); WAIT(40);
        STEP Audio_SetEnvReverb(40); Audio_SetCodeReverb(30); WAIT(60);
        STEP Audio_SetEnvReverb(0); Audio_SetCodeReverb(0); WAIT(30);
        STEP func_800F64E0(1); WAIT(60);
        STEP func_800F64E0(0); WAIT(60);
        // fanfares over the music, the music's volume
        STEP MARK(2); Audio_PlayFanfare(NA_BGM_ITEM_GET); WAIT(260);
        STEP Audio_SetBgmVolumeOffDuringFanfare(); Audio_PlayFanfare(NA_BGM_SMALL_ITEM_GET); WAIT(120);
        STEP Audio_SetBgmVolumeOnDuringFanfare(); WAIT(60);
        STEP Audio_SetMainBgmVolume(40, 10); WAIT(40);
        STEP Audio_SetMainBgmVolume(0x7F, 10); WAIT(40);
        STEP Audio_LowerMainBgmVolume(60); WAIT(30);
        STEP Audio_LowerMainBgmVolume(0x7F); WAIT(30);
        STEP Audio_SetMainBgmTempoFreqAfterFanfare(1.3f, 10); Audio_PlayFanfare(NA_BGM_HEART_GET); WAIT(260);
        STEP Audio_SetFastTempoForTimedMinigame(); WAIT(120);
        STEP Audio_QueueSeqCmdMute(11 /* SFX_CHANNEL_SYSTEM0 */); WAIT(30);
        STEP Audio_ClearBGMMute(11 /* SFX_CHANNEL_SYSTEM0 */); WAIT(30);
        // Ganon's tower's music by floor; the mini-boss music and back; ambience and back
        STEP MARK(3); Audio_PlaySceneSequence(NA_BGM_GANON_TOWER); WAIT(120);
        STEP Audio_SetGanonsTowerBgmVolumeLevel(sRep / 30); FRAMES(240);
        STEP Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); WAIT(150);
        STEP func_800F5ACC(NA_BGM_MINI_BOSS); WAIT(200);
        STEP func_800F5B58(); WAIT(150);
        STEP func_800F5BF0(NATURE_ID_GENERAL_NIGHT); WAIT(150);
        STEP func_800F5C2C(); WAIT(150);
        STEP Audio_PlaySequenceInCutscene(NA_BGM_ITEM_GET); WAIT(150);
        STEP (void)Audio_IsSequencePlaying(NA_BGM_ITEM_GET); Audio_StopSequenceInCutscene(NA_BGM_ITEM_GET); WAIT(60);
        STEP Audio_PlaySequenceInCutscene(NA_BGM_ENEMY); WAIT(150);
        STEP Audio_StopSequenceInCutscene(NA_BGM_ENEMY); WAIT(60);
        STEP Audio_PlayMorningSceneSequence(NA_BGM_FIELD_MORNING); WAIT(200);
        STEP Audio_PlayWindmillBgm(); WAIT(200);
        STEP Audio_PlaySequenceWithSeqPlayerIO(SEQ_PLAYER_BGM_MAIN, NA_BGM_KAKARIKO_KID, 0, 7, 1); WAIT(150);
        // Saria's song and Malon's by distance and side
        STEP MARK(4); Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); WAIT(100);
        STEP sP.x = -400.0f + (sRep * 5.0f); Audio_PlaySariaBgm(&sP, NA_BGM_SARIA_THEME, 700); FRAMES(160);
        STEP Audio_ClearSariaBgmAtPos(&sP); WAIT(60);
        STEP sP.x = 300.0f - (sRep * 4.0f); func_800F4E30(&sP, 500.0f - (sRep * 3.0f)); FRAMES(150);
        STEP func_800F4E30(&sQ, 100.0f); FRAMES(30);
        STEP Audio_ClearSariaBgm(); Audio_ClearSariaBgm2(); WAIT(60);
        STEP Audio_SplitBgmChannels(sRep - 40); FRAMES(80);
        STEP Audio_SplitBgmChannels(0); WAIT(30);
        STEP Audio_UpdateMalonSinging(800.0f - (sRep * 5.0f), NA_BGM_LONLON); FRAMES(160);
        STEP Audio_ToggleMalonSinging(true); WAIT(60);
        STEP Audio_ToggleMalonSinging(false); Audio_UpdateMalonSinging(100.0f, NA_BGM_LONLON); FRAMES(60);
        STEP Audio_UpdateMalonSinging(2000.0f, NA_BGM_LONLON); FRAMES(60);
        // the effects' special calls
        STEP MARK(5); Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); sP.x = 120.0f; WAIT(60);
        STEP Audio_PlaySfxRiver(&sP, 0.6f + (sRep * 0.01f)); Audio_PlaySfxWaterfall(&sQ, 1.4f - (sRep * 0.01f)); FRAMES(120);
        STEP func_800F4010(&sP, NA_SE_EV_WIND_TRAP - SFX_FLAG, 0.5f + (sRep * 0.02f)); FRAMES(60);
        STEP func_800F4138(&sP, NA_SE_EV_WIND_TRAP - SFX_FLAG, 0.3f + (sRep * 0.02f)); FRAMES(60);
        STEP func_800F4190(&sQ, NA_SE_EV_BOMB_BOUND); WAIT(20);
        STEP Audio_PlaySfxRandom(&sP, NA_SE_VO_LI_SWORD_N, 4); FRAMES(8);
        STEP Audio_PlaySwordChargeSfx(&sP, sRep >> 5); FRAMES(64);
        STEP func_800F436C(&sP, NA_SE_EV_FLYING_AIR - SFX_FLAG, 0.5f + (sRep * 0.02f)); FRAMES(60);
        STEP func_800F4414(&sP, NA_SE_IT_FISHING_REEL_SLOW - SFX_FLAG, sRep * 0.02f); FRAMES(60);
        STEP func_800F44EC(sRep, 64 - sRep); func_800F4524(&sP, NA_SE_EV_FLYING_AIR - SFX_FLAG, sRep); FRAMES(60);
        STEP func_800F4578(&sP, NA_SE_EV_FLYING_AIR - SFX_FLAG, sRep * 0.015f); FRAMES(60);
        STEP func_800F45D0(sRep * 0.03f); FRAMES(60);
        STEP Audio_PlaySfxIncreasinglyTransposed(&sP, NA_SE_SY_GET_RUPY, &sSemitones); FRAMES(20);
        STEP Audio_ResetIncreasingTranspose(); Audio_PlaySfxTransposed(&sP, NA_SE_SY_GET_RUPY, 5); WAIT(20);
        STEP Audio_PlaySfxTransposed(&sQ, NA_SE_SY_GET_RUPY, -7); WAIT(20);
        STEP func_800F4C58(&sP, NA_SE_EV_BOMB_BOUND, sRep); FRAMES(8);
        STEP Audio_PlayCutsceneEffectsSequence(sRep / 40); FRAMES(160);
        // the cutscene flag; stopping effects each way; banks muted
        STEP MARK(6); Audio_SetCutsceneFlag(1); Audio_PlaySfxIfNotInCutscene(NA_SE_SY_DECIDE);
        Audio_PlaySfxGeneralIfNotInCutscene(NA_SE_SY_CANCEL, &sP, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale,
                                            &gSfxDefaultReverb);
        WAIT(30);
        STEP Audio_SetCutsceneFlag(0); Audio_PlaySfxIfNotInCutscene(NA_SE_SY_DECIDE);
        Audio_PlaySfxGeneralIfNotInCutscene(NA_SE_SY_CANCEL, &sP, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale,
                                            &gSfxDefaultReverb);
        WAIT(30);
        STEP for (i = 0; i < 40; i++) {
            Audio_PlaySfxGeneral(sSweepSfxIds[(sRep * 40) + i], (i & 1) ? &sP : &sQ, 4 + (i & 1), &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        }
        switch (sRep) {
            case 0: Audio_StopSfxByPos(&sP); break;
            case 1: Audio_StopSfxByPosAndId(&sQ, sSweepSfxIds[41]); break;
            case 2: Audio_StopSfxByTokenAndId(5, sSweepSfxIds[81]); break;
            case 3: Audio_StopSfxById(sSweepSfxIds[122]); break;
            case 4: Audio_StopSfxByPosAndBank(BANK_PLAYER, &sP); break;
            case 5: Audio_StopSfxByBank(BANK_PLAYER); Audio_StopSfxByBank(BANK_ENV); break;
            case 6: (void)Audio_IsSfxPlaying(sSweepSfxIds[240]); Audio_SetSfxBanksMute(0x6F); break;
            case 7: Audio_SetSfxBanksMute(0); break;
        }
        sWait = 12;
        FRAMES(8);
        // the split
        case (__COUNTER__ - SWEEP_STATES_BASE):
            if (sSeek) {
                sSeek = false;
            } else if (parts > 1) {
                return true;
            }
            break;
        // every sequence command
        STEP MARK(7); Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); WAIT(120);
        STEP SEQCMD_SET_SEQPLAYER_VOLUME(SEQ_PLAYER_BGM_MAIN, 10, 60); SEQCMD_SET_SEQPLAYER_FREQ(SEQ_PLAYER_BGM_MAIN, 10, 1200); WAIT(40);
        STEP SEQCMD_SET_SEQPLAYER_VOLUME(SEQ_PLAYER_BGM_MAIN, 10, 0x7F); SEQCMD_SET_SEQPLAYER_FREQ(SEQ_PLAYER_BGM_MAIN, 10, 1000); WAIT(40);
        STEP SEQCMD_SET_CHANNEL_FREQ(SEQ_PLAYER_BGM_MAIN, 2, 10, 800); SEQCMD_SET_CHANNEL_VOLUME(SEQ_PLAYER_BGM_MAIN, 3, 10, 30); WAIT(40);
        STEP SEQCMD_SET_CHANNEL_FREQ(SEQ_PLAYER_BGM_MAIN, 2, 10, 1000); SEQCMD_SET_CHANNEL_VOLUME(SEQ_PLAYER_BGM_MAIN, 3, 10, 0x7F); WAIT(40);
        STEP SEQCMD_SET_SEQPLAYER_IO(SEQ_PLAYER_BGM_MAIN, 3, 1); SEQCMD_SET_CHANNEL_IO(SEQ_PLAYER_BGM_MAIN, 1, 2, 1); WAIT(30);
        STEP SEQCMD_SET_CHANNEL_IO_DISABLE_MASK(SEQ_PLAYER_BGM_MAIN, 0x0003); SEQCMD_SET_CHANNEL_IO(SEQ_PLAYER_BGM_MAIN, 1, 2, 0); WAIT(30);
        STEP SEQCMD_SET_CHANNEL_IO_DISABLE_MASK(SEQ_PLAYER_BGM_MAIN, 0); SEQCMD_SET_CHANNEL_DISABLE_MASK(SEQ_PLAYER_BGM_MAIN, 0x00F0); WAIT(40);
        STEP SEQCMD_SET_CHANNEL_DISABLE_MASK(SEQ_PLAYER_BGM_MAIN, 0); WAIT(40);
        STEP SEQCMD_SET_TEMPO(SEQ_PLAYER_BGM_MAIN, 10, 180); WAIT(40);
        STEP SEQCMD_SPEED_UP_TEMPO(SEQ_PLAYER_BGM_MAIN, 10, 30); WAIT(40);
        STEP SEQCMD_SLOW_DOWN_TEMPO(SEQ_PLAYER_BGM_MAIN, 10, 60); WAIT(40);
        STEP SEQCMD_SCALE_TEMPO(SEQ_PLAYER_BGM_MAIN, 10, 150); WAIT(40);
        STEP SEQCMD_RESET_TEMPO(SEQ_PLAYER_BGM_MAIN, 10); WAIT(40);
        STEP SEQCMD_QUEUE_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 5, 1, NA_BGM_KAKARIKO_ADULT); WAIT(120);
        STEP SEQCMD_QUEUE_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 5, 2, NA_BGM_GORON_CITY); WAIT(120);
        STEP SEQCMD_UNQUEUE_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 5, NA_BGM_GORON_CITY); WAIT(120);
        STEP SEQCMD_UNQUEUE_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 5, NA_BGM_KAKARIKO_ADULT); WAIT(60);
        STEP Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); WAIT(100);
        STEP SEQCMD_SETUP_RESTORE_SEQPLAYER_VOLUME(SEQ_PLAYER_FANFARE, SEQ_PLAYER_BGM_MAIN, 10);
        SEQCMD_SETUP_SCALE_TEMPO(SEQ_PLAYER_FANFARE, SEQ_PLAYER_BGM_MAIN, 10, 130);
        SEQCMD_SETUP_SET_SEQPLAYER_FREQ(SEQ_PLAYER_FANFARE, SEQ_PLAYER_BGM_MAIN, 10, 1100);
        SEQCMD_SETUP_SET_CHANNEL_DISABLE_MASK(SEQ_PLAYER_FANFARE, SEQ_PLAYER_BGM_MAIN, 0x0300);
        SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_FANFARE, 0, 0, NA_BGM_SMALL_ITEM_GET); WAIT(200);
        STEP SEQCMD_SETUP_RESET_TEMPO(SEQ_PLAYER_FANFARE, SEQ_PLAYER_BGM_MAIN, 10);
        SEQCMD_SETUP_RESTART_SEQUENCE(SEQ_PLAYER_FANFARE, SEQ_PLAYER_BGM_MAIN);
        SEQCMD_SETUP_SET_CHANNEL_DISABLE_MASK(SEQ_PLAYER_FANFARE, SEQ_PLAYER_BGM_MAIN, 0);
        SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_FANFARE, 0, 0, NA_BGM_SMALL_ITEM_GET); WAIT(200);
        STEP SEQCMD_SETUP_PLAY_SEQUENCE(SEQ_PLAYER_FANFARE, SEQ_PLAYER_BGM_MAIN, NA_BGM_KAKARIKO_KID);
        SEQCMD_SETUP_SET_FADE_IN_TIMER(SEQ_PLAYER_BGM_MAIN, 20);
        SEQCMD_SETUP_POP_PERSISTENT_CACHE(SEQ_PLAYER_FANFARE, 0x30);
        SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_FANFARE, 0, 0, NA_BGM_SMALL_ITEM_GET); WAIT(250);
        STEP SEQCMD_SETUP_UNQUEUE_SEQUENCE(SEQ_PLAYER_FANFARE);
        SEQCMD_SETUP_RESTORE_SEQPLAYER_VOLUME_WITH_SCALE_INDEX(SEQ_PLAYER_FANFARE, SEQ_PLAYER_BGM_MAIN, 1, 10);
        SEQCMD_SETUP_RESTORE_SEQPLAYER_VOLUME_IF_QUEUED(SEQ_PLAYER_FANFARE, SEQ_PLAYER_BGM_MAIN, 10, 10);
        SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_FANFARE, 0, 0, NA_BGM_SMALL_ITEM_GET); WAIT(200);
        STEP SEQCMD_RESET_SETUP_CMDS(SEQ_PLAYER_FANFARE); SEQCMD_DISABLE_PLAY_SEQUENCES(true);
        SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, 0, NA_BGM_GORON_CITY); WAIT(60);
        STEP SEQCMD_DISABLE_PLAY_SEQUENCES(false); SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, 0, NA_BGM_GORON_CITY); WAIT(120);
        STEP SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_SUB, 10, 0, NA_BGM_LONLON); WAIT(120);
        STEP SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_SUB, 20); WAIT(60);
        // every command to the audio thread
        STEP MARK(8); Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); WAIT(120);
        STEP AUDIOCMD_CHANNEL_SET_VOL_SCALE(SEQ_PLAYER_BGM_MAIN, 0, 0.5f); AUDIOCMD_CHANNEL_SET_VOL(SEQ_PLAYER_BGM_MAIN, 1, 0.6f);
        AUDIOCMD_CHANNEL_SET_PAN(SEQ_PLAYER_BGM_MAIN, 2, 20); AUDIOCMD_CHANNEL_SET_FREQ_SCALE(SEQ_PLAYER_BGM_MAIN, 3, 1.2f);
        AUDIOCMD_CHANNEL_SET_REVERB_VOLUME(SEQ_PLAYER_BGM_MAIN, 4, 90); AUDIOCMD_CHANNEL_SET_IO(SEQ_PLAYER_BGM_MAIN, 5, 1, 1);
        AUDIOCMD_CHANNEL_SET_PAN_WEIGHT(SEQ_PLAYER_BGM_MAIN, 2, 60); WAIT(40);
        STEP AUDIOCMD_CHANNEL_SET_MUTE(SEQ_PLAYER_BGM_MAIN, 0, true); AUDIOCMD_CHANNEL_SET_MUTE_BEHAVIOR(SEQ_PLAYER_BGM_MAIN, 1, 0x40);
        AUDIOCMD_CHANNEL_SET_VIBRATO_DEPTH(SEQ_PLAYER_BGM_MAIN, 2, 40); AUDIOCMD_CHANNEL_SET_VIBRATO_RATE(SEQ_PLAYER_BGM_MAIN, 2, 30);
        AUDIOCMD_CHANNEL_SET_COMB_FILTER_SIZE(SEQ_PLAYER_BGM_MAIN, 3, 0x40); AUDIOCMD_CHANNEL_SET_COMB_FILTER_GAIN(SEQ_PLAYER_BGM_MAIN, 3, 0x4000);
        AUDIOCMD_CHANNEL_SET_STEREO(SEQ_PLAYER_BGM_MAIN, 4, 0x20); WAIT(60);
        STEP AUDIOCMD_CHANNEL_SET_MUTE(SEQ_PLAYER_BGM_MAIN, 0, false); AUDIOCMD_CHANNEL_SET_VOL_SCALE(SEQ_PLAYER_BGM_MAIN, 0, 1.0f);
        AUDIOCMD_CHANNEL_SET_VOL(SEQ_PLAYER_BGM_MAIN, 1, 1.0f); AUDIOCMD_CHANNEL_SET_FREQ_SCALE(SEQ_PLAYER_BGM_MAIN, 3, 1.0f);
        AUDIOCMD_CHANNEL_SET_COMB_FILTER_SIZE(SEQ_PLAYER_BGM_MAIN, 3, 0); AUDIOCMD_CHANNEL_SET_COMB_FILTER_GAIN(SEQ_PLAYER_BGM_MAIN, 3, 0);
        AUDIOCMD_CHANNEL_SET_VIBRATO_DEPTH(SEQ_PLAYER_BGM_MAIN, 2, 0); AUDIOCMD_CHANNEL_SET_VIBRATO_RATE(SEQ_PLAYER_BGM_MAIN, 2, 0); WAIT(40);
        STEP AUDIOCMD_SEQPLAYER_FADE_VOLUME_SCALE(SEQ_PLAYER_BGM_MAIN, 0.5f); AUDIOCMD_SEQPLAYER_SET_IO(SEQ_PLAYER_BGM_MAIN, 1, 1);
        AUDIOCMD_SEQPLAYER_SET_TEMPO(SEQ_PLAYER_BGM_MAIN, 140); AUDIOCMD_SEQPLAYER_SET_TRANSPOSITION(SEQ_PLAYER_BGM_MAIN, 2); WAIT(50);
        STEP AUDIOCMD_SEQPLAYER_CHANGE_TEMPO(SEQ_PLAYER_BGM_MAIN, 20); AUDIOCMD_SEQPLAYER_CHANGE_TEMPO_SEQTICKS(SEQ_PLAYER_BGM_MAIN, 10);
        AUDIOCMD_SEQPLAYER_FADE_TO_SET_VOLUME(SEQ_PLAYER_BGM_MAIN, 50, 20); WAIT(50);
        STEP AUDIOCMD_SEQPLAYER_FADE_TO_SCALED_VOLUME(SEQ_PLAYER_BGM_MAIN, 100, 20); AUDIOCMD_SEQPLAYER_SET_BEND(SEQ_PLAYER_BGM_MAIN, 1.1f); WAIT(50);
        STEP AUDIOCMD_SEQPLAYER_RESET_VOLUME(SEQ_PLAYER_BGM_MAIN, 20); AUDIOCMD_SEQPLAYER_SET_BEND(SEQ_PLAYER_BGM_MAIN, 1.0f);
        AUDIOCMD_SEQPLAYER_SET_TRANSPOSITION(SEQ_PLAYER_BGM_MAIN, 0); AUDIOCMD_SEQPLAYER_CHANGE_TEMPO(SEQ_PLAYER_BGM_MAIN, 0);
        AUDIOCMD_SEQPLAYER_CHANGE_TEMPO_SEQTICKS(SEQ_PLAYER_BGM_MAIN, 0); WAIT(50);
        STEP AUDIOCMD_GLOBAL_SET_CHANNEL_MASK(SEQ_PLAYER_BGM_MAIN, 0x00FF); AUDIOCMD_CHANNEL_SET_VOL_SCALE(SEQ_PLAYER_BGM_MAIN, AUDIOCMD_ALL_CHANNELS, 0.5f); WAIT(40);
        STEP AUDIOCMD_GLOBAL_SET_CHANNEL_MASK(SEQ_PLAYER_BGM_MAIN, 0xFFFF); AUDIOCMD_CHANNEL_SET_VOL_SCALE(SEQ_PLAYER_BGM_MAIN, AUDIOCMD_ALL_CHANNELS, 1.0f); WAIT(40);
        STEP AUDIOCMD_GLOBAL_MUTE(); WAIT(40);
        STEP AUDIOCMD_GLOBAL_UNMUTE(true); WAIT(40);
        STEP AUDIOCMD_GLOBAL_NOOP_1(0, 0, 0, 0); AUDIOCMD_GLOBAL_NOOP_2(0, 0, 0, 0);
        AUDIOCMD_GLOBAL_SET_CUSTOM_UPDATE_FUNCTION(Sweep_StatesCustomUpdate); WAIT(40);
        STEP AUDIOCMD_GLOBAL_SET_CUSTOM_UPDATE_FUNCTION(NULL); AUDIOCMD_GLOBAL_DISABLE_SEQPLAYER(SEQ_PLAYER_BGM_MAIN, 20); WAIT(60);
        // loads asked for outright: with nothing in memory, in the background, in parts; then started by the thread's
        // own commands
        STEP MARK(9); AUDIOCMD_GLOBAL_POP_PERSISTENT_CACHE(SEQUENCE_TABLE); AUDIOCMD_GLOBAL_POP_PERSISTENT_CACHE(FONT_TABLE);
        AUDIOCMD_GLOBAL_POP_PERSISTENT_CACHE(SAMPLE_TABLE); WAIT(30);
        STEP AUDIOCMD_GLOBAL_ASYNC_LOAD_SEQ(NA_BGM_FIELD_LOGIC + sRep, 1 + sRep); AUDIOCMD_GLOBAL_ASYNC_LOAD_FONT(3 + sRep, 0x21 + sRep);
        AUDIOCMD_GLOBAL_ASYNC_LOAD_SAMPLE_BANK(sRep & 3, 0x41 + sRep); sWait = 30; FRAMES(8);
        STEP AUDIOCMD_GLOBAL_DISCARD_SEQ_FONTS(NA_BGM_FIELD_LOGIC + sRep); sWait = 10; FRAMES(8);
        STEP AUDIOCMD_GLOBAL_SYNC_LOAD_SEQ_PARTS(NA_BGM_KAKARIKO_ADULT, 3); AUDIOCMD_GLOBAL_SYNC_LOAD_INSTRUMENT(3, 5, 0x7F);
        AUDIOCMD_GLOBAL_SYNC_LOAD_INSTRUMENT(3, 0x7F, 3); WAIT(40);
        STEP AUDIOCMD_GLOBAL_INIT_SEQPLAYER(SEQ_PLAYER_BGM_MAIN, NA_BGM_KAKARIKO_ADULT, 10); WAIT(120);
        STEP AUDIOCMD_GLOBAL_INIT_SEQPLAYER_SKIP_TICKS(SEQ_PLAYER_BGM_MAIN, NA_BGM_GORON_CITY, 500); WAIT(120);
        STEP AUDIOCMD_GLOBAL_DISABLE_ALL_SEQPLAYERS(0); WAIT(40);
        STEP func_800F6B3C(); Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); WAIT(100);
        // a reset into each of the session settings, music and effects played in each
        STEP MARK(10 + sSpec); SEQCMD_RESET_AUDIO_HEAP(0, sSpec); WAIT(30);
        STEP if (D_80133418 != 0) { sStep--; } break;
        STEP SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, 0, NA_BGM_FIELD_LOGIC + (sSpec * 3)); WAIT(60);
        STEP Audio_PlaySfxGeneral(sSweepSfxIds[(sSpec * 37) + sRep], &sP, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale,
                                  &gSfxDefaultReverb);
        sWait = 2; FRAMES(30);
        STEP if (++sSpec < 18) { sStep -= 5; } WAIT(20);
        // stopping everything each way the game does; then the reset button
        STEP MARK(40); SEQCMD_RESET_AUDIO_HEAP(0, 0); WAIT(30);
        STEP if (D_80133418 != 0) { sStep--; } break;
        STEP Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); WAIT(120);
        STEP Audio_StopBgmAndFanfare(20); WAIT(60);
        STEP Audio_PlaySceneSequence(NA_BGM_KAKARIKO_ADULT); WAIT(120);
        STEP func_800F6964(30); WAIT(80);
        STEP func_800F7208(); Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); WAIT(120);
        STEP Audio_DisableAllSeq(); WAIT(40);
        STEP func_800F71BC(0); func_800F7170(); WAIT(40);
        STEP Audio_PlaySceneSequence(NA_BGM_FIELD_LOGIC); WAIT(120);
        STEP MARK(41); Audio_PreNMI(); WAIT(90);
        default:
            return true;
    }
    } while (sSeek);
    return false;
}

// One frame of the leg; true once it's over.
static s32 Sweep_Leg(s32 kind, s32 parts, s32 part) {
    static s32 sItem = -1;
    static s32 sTimer;
    static s32 sStepFrame;
    static Vec3f sPos[8];
    static const u16 sFanfares[] = { NA_BGM_ITEM_GET, NA_BGM_HEART_GET, NA_BGM_SMALL_ITEM_GET, NA_BGM_OPEN_TRE_BOX };
    s32 i;

    if (kind == 1) {
        if (sItem < 0) {
            sItem = part;
            sTimer = 0;
        }
        if (sItem >= SWEEP_SEQ_COUNT + SWEEP_NATURE_COUNT) {
            return true;
        }
        if (sItem < SWEEP_SEQ_COUNT) {
            if (sTimer == 0) {
                gFishmodSweepLeg = 0x1000 + SWEEP_SEQ_FIRST + sItem;
                SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, 0, SWEEP_SEQ_FIRST + sItem);
            } else if ((sTimer == 120) && ((sItem & 3) == 0)) {
                Audio_PlayFanfare(sFanfares[(sItem >> 2) & 3]);
            } else if (sTimer == 200) {
                SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 20);
            }
            if (++sTimer >= SWEEP_SEQ_FRAMES) {
                sTimer = 0;
                sItem += parts;
            }
        } else {
            if (sTimer == 0) {
                gFishmodSweepLeg = 0x1800 + sItem - SWEEP_SEQ_COUNT;
                Audio_PlayNatureAmbienceSequence(sItem - SWEEP_SEQ_COUNT);
            }
            if (++sTimer >= SWEEP_NATURE_FRAMES) {
                sTimer = 0;
                sItem += parts;
            }
        }
        return false;
    }
    if (kind == 2) {
        // (the positions are the effects' as projected: x right, y up, z ahead)
        static const Vec3f sAt[8] = { { 0.0f, 0.0f, 30.0f },    { -300.0f, 0.0f, 100.0f }, { 300.0f, 0.0f, 100.0f },
                                      { 0.0f, 200.0f, 600.0f }, { 0.0f, 0.0f, -200.0f },   { -800.0f, 0.0f, 800.0f },
                                      { 900.0f, 50.0f, 300.0f }, { 0.0f, 0.0f, 1500.0f } };

        if (sItem < 0) {
            sItem = 0;
            for (i = 0; i < 8; i++) {
                sPos[i] = sAt[i];
            }
        }
        // (dealt in blocks of 64 effects)
        while ((sItem < (s32)ARRAY_COUNT(sSweepSfxIds)) && (((sItem >> 6) % parts) != part)) {
            sItem = (sItem | 63) + 1;
        }
        if (sItem >= (s32)ARRAY_COUNT(sSweepSfxIds)) {
            return ++sTimer > 100;
        }
        if ((sItem & 63) == 0) {
            gFishmodSweepLeg = 0x2000 + (sItem >> 6);
        }
        if ((++sStepFrame % 3) == 0) {
            Audio_PlaySfxGeneral(sSweepSfxIds[sItem], &sPos[sItem & 7], 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            if ((sItem & 63) == 40) {
                // a burst: the 24 before it again, all at once (more voices asked for than there are)
                for (i = 1; i <= 24; i++) {
                    Audio_PlaySfxGeneral(sSweepSfxIds[sItem - i], &sPos[(sItem - i) & 7], 4, &gSfxDefaultFreqAndVolScale,
                                         &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
                }
            }
            sItem++;
        }
        return false;
    }
    if (kind == 4) {
        return Sweep_States(parts, part);
    }
    if (kind == 3) {
        const SweepOcaStep* step;

        if (sItem < 0) {
            sItem = 0;
            sStepFrame = 0;
            gFishmodOcarinaPad = Sweep_OcarinaPad;
        }
        if (sItem >= (s32)ARRAY_COUNT(sSweepOcaSteps)) {
            sSweepOcaButtons = 0;
            return true;
        }
        step = &sSweepOcaSteps[sItem];
        if (sStepFrame == 0) {
            switch (step->act) {
                case OCA_INSTRUMENT:
                    AudioOcarina_SetInstrument(step->arg);
                    break;
                case OCA_START:
                    AudioOcarina_Start(step->arg);
                    break;
                case OCA_PLAYBACK:
                    AudioOcarina_SetPlaybackSong(step->arg, 1);
                    break;
                case OCA_RECORD:
                    AudioOcarina_SetRecordingState(step->arg);
                    break;
                case OCA_MEMORY:
                    if (step->arg == 0) {
                        AudioOcarina_MemoryGameInit(0);
                    } else {
                        for (i = 0; i < 3; i++) {
                            AudioOcarina_MemoryGameNextNote();
                        }
                    }
                    break;
                case OCA_LEG:
                    gFishmodSweepLeg = step->arg;
                    break;
            }
        }
        sSweepOcaButtons = step->buttons;
        sSweepOcaStickX = step->stickX;
        sSweepOcaStickY = step->stickY;
        if (++sStepFrame >= step->frames) {
            sStepFrame = 0;
            sItem++;
        }
        return false;
    }
    return true;
}
