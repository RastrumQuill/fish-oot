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
