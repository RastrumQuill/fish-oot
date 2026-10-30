// Part of fishmod_debug.c (#included there, sharing its statics): the LOD jobs: calibration, sweep, tuning, rotation check (FISHMOD_LOD_*).
#ifdef FISHMOD_LOD_CALIBRATE
/*
 * LOD calibration: how far each habitat prop kind has to be drawn, measured on the game's own frames at the N64's
 * resolution. At each tour stop, with everything that moves held still (the clock, the frame count that drives
 * sway and scrolling water; no fish, no particles), the frame with every prop drawn out to 3000 units is the
 * reference, and a few more like it mark the pixels that change by themselves. Then for each kind in view, a
 * binary search finds the shortest distance it can stop at with no more than LOD_PIXELS pixels (not counting the
 * restless ones) differing by more than LOD_THRESHOLD in any 5-bit channel. Pixel by pixel, not averaged: a frame
 * that is mostly fog can't hide a prop that shows. Each kind keeps the furthest any stop needs, above water and
 * under it, and each stop is checked again with every kind at those distances together. The results go to SRAM
 * (0x7A40, after the save slots) for fishmod/tools/lod_calibrate.py, which writes include/fishmod_lod.h.
 */
#define LOD_THRESHOLD 2 // a pixel differs if a channel moves by more than this (of 31; dither moves them by 1)
#define LOD_PIXELS 2    // pixels a kind may change by stopping short
#define LOD_STEP 100    // distances tried: steps of this...
#define LOD_STEPS 30    // ...out to LOD_STEP * LOD_STEPS (the reference)
#define LOD_HOLD 5      // frames each test is drawn before its frame is read (the display runs a frame or two behind)
#define LOD_SETTLE 40   // frames after the camera arrives before the reference
#define LOD_NOISE_REFS 3

typedef enum LodState {
    LOD_ARRIVE,
    LOD_REF,
    LOD_NOISE,
    LOD_NEXT_KIND,
    LOD_SEARCH,
    LOD_VERIFY,
    LOD_DONE
} LodState;

typedef struct LodResults {
    /* 0x000 */ u32 magic; // 'FLOD'
    /* 0x004 */ u8 kinds;
    /* 0x005 */ u8 stops;   // stops done so far
    /* 0x006 */ u8 threshold;
    /* 0x007 */ u8 pixels;
    /* 0x008 */ u16 need[2][FISHMOD_LOD_KINDS]; // [above, under] water: the furthest any stop needs, units (0: never seen)
    /* 0x0C8 */ u16 noise[32];  // per stop: pixels that changed by themselves
    /* 0x108 */ u16 verify[32]; // per stop: pixels changed with every kind at its distance
    /* 0x148 */ u8 under[32];   // per stop: the camera was under water
    /* 0x168 */ u16 logCount;
    /* 0x16A */ u16 log[64][3]; // the first tests: kind, distance, pixels changed (for checking the method)
} LodResults;

static LodResults sLod;
static u16* sLodRef;   // the reference frame
static u8* sLodNoise;  // pixels that change by themselves, a bit each
static u16 sLodDrawn[FISHMOD_LOD_KINDS]; // props of each kind in the reference
static s32 sLodState = LOD_ARRIVE;
static s32 sLodTimer;
static s32 sLodPass;
static s32 sLodKind;
static s32 sLodLo;
static s32 sLodHi;
static u32 sLodFrames;
static u16 sLodTime;
static f32 sLodSkyRot;
static u32 sLodStateFrames;

static u16* sLodCur;   // the frame being tested, copied off the screen

static GraphicsContext* sLodGfxCtx;

/**
 * Copies a finished frame to `dst`: the buffer this frame is about to be drawn into, which still holds the frame
 * from two updates ago, whole (its drawing ended before the last one began, and this one's starts after this
 * update). Not the buffer on the screen: the next frame can already be being drawn into that one.
 */
static s32 Lod_Grab(u16* dst) {
    u16* fb = (u16*)OS_PHYSICAL_TO_K0(OS_K0_TO_PHYSICAL(sLodGfxCtx->curFrameBuffer));

    // through the cache (lines at a time: several times faster than one read each), the stale lines dropped first
    osInvalDCache(fb, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
    bcopy(fb, dst, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
    return true;
}

/** Pixels differing from the reference by more than LOD_THRESHOLD; with `markNoise`, they're marked restless. */
static s32 Lod_Compare(s32 markNoise) {
    u16* fb = sLodCur;
    s32 count = 0;
    s32 i;

    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        u16 a = fb[i];
        u16 b = sLodRef[i];
        s32 dr;
        s32 dg;
        s32 db;

        if (a == b) {
            continue;
        }
        dr = ABS((s32)((a >> 11) & 31) - (s32)((b >> 11) & 31));
        dg = ABS((s32)((a >> 6) & 31) - (s32)((b >> 6) & 31));
        db = ABS((s32)((a >> 1) & 31) - (s32)((b >> 1) & 31));
        if ((dr > LOD_THRESHOLD) || (dg > LOD_THRESHOLD) || (db > LOD_THRESHOLD)) {
            if (markNoise == 2) {
                if (!(sLodNoise[i >> 3] & (1 << (i & 7)))) {
                    fb[i] = 0xF801;
                }
            } else if (markNoise) {
                sLodNoise[i >> 3] |= 1 << (i & 7);
                count++;
            } else if (!(sLodNoise[i >> 3] & (1 << (i & 7)))) {
                count++;
            }
        }
    }
    return count;
}

static u16 sLodPending[FISHMOD_LOD_KINDS]; // the cut to test once its fresh reference is taken
static s32 sLodFresh;                      // a fresh reference is being drawn

/**
 * Draws everything for LOD_HOLD frames and takes that as the reference, then the cut in sLodPending for LOD_HOLD
 * frames: each test is compared with a frame from just before it, so nothing that drifts slowly (light settling,
 * the sky) is counted.
 */
static void Lod_TestPending(void) {
    bzero(gFishmodLodTest.dist, sizeof(gFishmodLodTest.dist));
    bzero(gFishmodLodTest.imp, sizeof(gFishmodLodTest.imp));
    bzero(gFishmodLodTest.half, sizeof(gFishmodLodTest.half));
    sLodFresh = true;
    sLodTimer = LOD_HOLD;
}

static void Lod_Test(s32 kind, s32 dist) {
    bzero(sLodPending, sizeof(sLodPending));
    if (kind < 0) {
        bzero(gFishmodLodTest.dist, sizeof(gFishmodLodTest.dist));
        bzero(gFishmodLodTest.imp, sizeof(gFishmodLodTest.imp));
        bzero(gFishmodLodTest.half, sizeof(gFishmodLodTest.half));
        sLodTimer = LOD_HOLD;
        return;
    }
    sLodPending[kind] = dist;
    Lod_TestPending();
}

static void Lod_Save(void) {
    sLod.magic = 0x464C4F44; // 'FLOD'
    sLod.kinds = gFishmodLodTest.kindCount;
    sLod.threshold = LOD_THRESHOLD;
    sLod.pixels = LOD_PIXELS;
    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_BIG), &sLod, sizeof(sLod), OS_WRITE);
}

#ifdef FISHMOD_LOD_SWEEP
/*
 * The LOD sweep: the calibration over many viewpoints (fishmod/tools/gen_lod_views.py: the shores, the surface, the
 * bed, each looked from several ways), for draw distances per zone. Group testing keeps it quick: each view is first
 * tested once with every kind cut to its zone's distances so far; only if that changes the frame are the kinds
 * split in halves (each half cut alone) down to the ones that show, and only those searched for the distance they
 * need here. Most views, once the tables have grown, pass on the first test. Results to SRAM 0x0000 (over the save
 * slots: this build never saves) for fishmod/tools/lod_calibrate.py --sweep.
 */
#define SWEEP_ZONES 16
#ifdef FISHMOD_LOD_TUNE
/*
 * The LOD tuning (lod_tune.inc.c): the sweep, but over every view at each of TUNE_TIMES times of day, and for three
 * settings per kind, each against the same full-detail reference: how far it's drawn (the cut), from how far its
 * single card is used, from how far its half-size texture is. Each the shortest distance that changes no view (as
 * the sweep counts it), per time of day and above or under water, in TUNE_STEP units. The views are shared out
 * between parts run side by side; the part comes from SRAM 0x7C00 ('FMPT', part, parts: fishmod/pc/run_jobs.py
 * writes it into each job's save), so one ROM serves every part.
 */
#define TUNE_TIMES 4
#define TUNE_KNOBS 3 // the cut, the single card, the half-size texture
#define SW_STEP 50
#define SW_STEPS 60 // out to 3000 (DIST_CALIBRATE)
static const u16 sTuneTimes[TUNE_TIMES] = { CLOCK_TIME(6, 30), CLOCK_TIME(12, 0), CLOCK_TIME(18, 30), CLOCK_TIME(0, 0) };
static s32 sTuneKnob;
#define SWEEP_SLOTS ((s32)ARRAY_COUNT(sLodViews) * TUNE_TIMES)
#define SWEEP_SLOT_VIEW(slot) ((slot) / TUNE_TIMES)
#define SW_KNOB_DONE SW_NEXT_KNOB
#else
#define SW_STEP LOD_STEP
#define SW_STEPS LOD_STEPS
#define SWEEP_SLOTS ((s32)ARRAY_COUNT(sLodViews))
#define SWEEP_SLOT_VIEW(slot) (slot)
#define SW_KNOB_DONE SW_NEXT_VIEW
#endif
static s32 sSweepPart = FISHMOD_LOD_PART;
static s32 sSweepParts = FISHMOD_LOD_PARTS;
#define SWEEP_SETTLE 45
#define SWEEP_HOLD 3       // frames a test is drawn before it's read
#define SWEEP_GRAB_TRIES 30 // a view whose frame can't be copied whole this many times is skipped
#define SWEEP_RETRIES 3

Vec3f gFishmodLodSweepEye;
Vec3f gFishmodLodSweepAt;

typedef struct SweepResults {
    /* 0x000 */ u32 magic; // 'FLSW'
    /* 0x004 */ u16 views;
    /* 0x006 */ u16 done;   // views measured so far (in this part)
    /* 0x008 */ u16 noisy;  // views skipped: the frame changed by itself
    /* 0x00A */ u16 empty;  // views with no props in sight
    /* 0x00C */ u32 tests;
    /* 0x010 */ u8 kinds;
    /* 0x011 */ u8 zones;
    /* 0x012 */ u8 part;
    /* 0x013 */ u8 parts;
#ifdef FISHMOD_LOD_TUNE
    /* 0x014 */ u8 need[TUNE_TIMES][2][TUNE_KNOBS][FISHMOD_LOD_KINDS]; // in SW_STEP units; 0: never in sight
#else
    /* 0x014 */ u8 need[SWEEP_ZONES][2][FISHMOD_LOD_KINDS]; // in LOD_STEP units; 0: never in sight
#endif
    u16 passFirst; // views that passed on their first test
    u16 splits;    // views that needed the kinds split
} SweepResults;

typedef enum SweepState {
    SW_ARRIVE,
    SW_REF,
    SW_NOISE,
    SW_ALL,
    SW_SPLIT_NEXT,
    SW_SPLIT_CHECK,
    SW_SEARCH,
    SW_NEXT_VIEW,
    SW_NEXT_KNOB,
    SW_DONE
} SweepState;

static SweepResults sSweep;
static s32 sSweepState = SW_ARRIVE;
static s32 sSweepView = FISHMOD_LOD_PART; // (the slot: the view, and with the tuning the time of day)
static s32 sSweepRetry;
static s32 sSweepFirstTest;
static u64 sSweepStack[64];
static s32 sSweepTop;
static u64 sSweepMask; // being tested in a split
static u64 sSweepCulprits;
static u64 sSweepWhole; // every kind cut, if that changed the frame
static s32 sSweepSettle;
static s32 sSweepGrabFails;

// each part takes its own run of the views (one stretch of the lake each, its times of day together)
static s32 Sweep_PartStart(void) {
    return (s32)((s64)SWEEP_SLOTS * sSweepPart / sSweepParts);
}

static s32 Sweep_PartEnd(void) {
    return (s32)((s64)SWEEP_SLOTS * (sSweepPart + 1) / sSweepParts);
}

static u8* Sweep_Need(s32 kind) {
#ifdef FISHMOD_LOD_TUNE
    return &sSweep.need[sSweepView % TUNE_TIMES][gFishmodLodTest.under][sTuneKnob][kind];
#else
    FishmodLodView* view = &sLodViews[sSweepView];

    return &sSweep.need[view->zone % SWEEP_ZONES][gFishmodLodTest.under][kind];
#endif
}

/** The test's distances for the setting being measured (the cut, or with the tuning one of its others). */
static s16* Sweep_Knob(void) {
    bzero(gFishmodLodTest.dist, sizeof(gFishmodLodTest.dist));
    bzero(gFishmodLodTest.imp, sizeof(gFishmodLodTest.imp));
    bzero(gFishmodLodTest.half, sizeof(gFishmodLodTest.half));
#ifdef FISHMOD_LOD_TUNE
    if (sTuneKnob == 1) {
        return gFishmodLodTest.imp;
    }
    if (sTuneKnob == 2) {
        return gFishmodLodTest.half;
    }
#endif
    return gFishmodLodTest.dist;
}

/** The cut to test: the kinds in `mask` at their zone's distances, the rest drawn in full. */
static void Sweep_Test(u64 mask) {
    s32 k;

    s16* knob = Sweep_Knob();

    for (k = 0; k < gFishmodLodTest.kindCount; k++) {
        if (mask & (1ULL << k)) {
            knob[k] = MAX(*Sweep_Need(k), 1) * SW_STEP;
        }
    }
    sSweep.tests++;
    // one reference per view (taken after it settles, checked for anything moving): no fresh one per test
    sLodTimer = SWEEP_HOLD;
}

/** One kind cut at `dist`, the rest drawn in full. */
static void Sweep_TestKind(s32 kind, s32 dist) {
    Sweep_Knob()[kind] = dist;
    sLodTimer = SWEEP_HOLD;
}

static void Sweep_Save(void) {
    sSweep.magic = 0x464C5357; // 'FLSW'
    sSweep.views = SWEEP_SLOTS;
    sSweep.kinds = gFishmodLodTest.kindCount;
    sSweep.zones = SWEEP_ZONES;
    sSweep.part = sSweepPart;
    sSweep.parts = sSweepParts;
    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_BENCH), &sSweep, sizeof(sSweep), OS_WRITE);
}

static void Sweep_Place(void) {
    FishmodLodView* view = &sLodViews[SWEEP_SLOT_VIEW(sSweepView)];

    gFishmodLodSweepEye.x = view->eye.x;
    gFishmodLodSweepEye.y = view->eye.y;
    gFishmodLodSweepEye.z = view->eye.z;
    gFishmodLodSweepAt.x = view->at.x;
    gFishmodLodSweepAt.y = view->at.y;
    gFishmodLodSweepAt.z = view->at.z;
#ifdef FISHMOD_LOD_TUNE
    sLodTime = sTuneTimes[sSweepView % TUNE_TIMES];
#endif
}

static void Fishmod_LodUpdate(PlayState* play) {
    sLodGfxCtx = play->state.gfxCtx;
    if (sLodRef == NULL) {
        sLodRef = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        sLodNoise = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT / 8, "../fishmod_debug.c", __LINE__);
        sLodCur = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        sLodFrames = play->gameplayFrames;
        sLodTime = gSaveContext.save.dayTime;
        sLodSkyRot = play->skyboxCtx.rot.y;
        sLodStateFrames = play->state.frames;
        gFishmodLodTest.active = true;
        sSweepSettle = SWEEP_SETTLE + 200; // the first: the scene's lights are still coming up
        // which part this is, if the runner said: in the ROM's header (the name field, which nothing checks), else
        // in the save (fishmod/pc/run_jobs.py writes both)
        {
            static u32 pt[4] ALIGNED(16);

            DmaMgr_RequestSync(pt, 0x20, sizeof(pt));
            if (!((pt[0] == 0x464D5054) && (pt[2] > 0) && (pt[1] < pt[2]))) { // 'FMPT'
                SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_FAULT), pt, 12, OS_READ);
            }
            if ((pt[0] == 0x464D5054) && (pt[2] > 0) && (pt[1] < pt[2])) {
                sSweepPart = pt[1];
                sSweepParts = pt[2];
            }
        }
        // carry on from an earlier run (the save file kept what it measured)
        SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_BENCH), &sSweep, sizeof(sSweep), OS_READ);
        if ((sSweep.magic != 0x464C5357) || (sSweep.part != sSweepPart) || (sSweep.parts != sSweepParts)) {
            bzero(&sSweep, sizeof(sSweep));
        }
        sSweepView = Sweep_PartStart() + sSweep.done;
        if (sSweepView >= Sweep_PartEnd()) {
            sSweepState = SW_DONE;
            gFishmodLodTest.active = false;
            return;
        }
        Sweep_Place();
        if ((sLodRef == NULL) || (sLodNoise == NULL) || (sLodCur == NULL)) {
            sSweepState = SW_DONE;
        }
    }
    play->gameplayFrames = sLodFrames;
    gSaveContext.save.dayTime = sLodTime;
    play->skyboxCtx.rot.y = sLodSkyRot;
    play->state.frames = sLodStateFrames;
    play->haltAllActors = gFishmodLodTest.active;
    if (sSweepState == SW_DONE) {
        Fishmod_DebugPrintf(2, "LOD sweep done: %d views", sSweep.done);
        return;
    }
    Fishmod_DebugPrintf(2, "LOD part %d/%d: %d of %d", sSweepPart + 1, sSweepParts, sSweepView - Sweep_PartStart() + 1,
                        Sweep_PartEnd() - Sweep_PartStart());
    if (sSweepState == SW_ARRIVE) {
        if (--sSweepSettle > 0) {
            return;
        }
        Lod_Test(-1, 0);
        sSweepState = SW_REF;
        return;
    }
    if (--sLodTimer > 0) {
        return;
    }
    if (sLodFresh) {
        if (!Lod_Grab(sLodRef)) {
            sLodTimer = 1;
            return;
        }
        bcopy(sLodPending, gFishmodLodTest.dist, sizeof(sLodPending));
        sLodFresh = false;
        sLodTimer = LOD_HOLD;
        return;
    }
    if (!Lod_Grab((sSweepState == SW_REF) ? sLodRef : sLodCur)) {
        sLodTimer = 1; // the screen swapped while it was copied: again next frame
        if (++sSweepGrabFails < SWEEP_GRAB_TRIES) {
            return;
        }
        sSweep.noisy++;
        sSweepState = SW_NEXT_VIEW;
    }
    sSweepGrabFails = 0;

    switch (sSweepState) {
        case SW_REF: {
            s32 k;
            s32 any = false;

            bcopy(gFishmodLodTest.drawn, sLodDrawn, sizeof(sLodDrawn));
            for (k = 0; k < gFishmodLodTest.kindCount; k++) {
                any |= (sLodDrawn[k] != 0);
            }
            if (!any) {
                sSweep.empty++;
                sSweepState = SW_NEXT_VIEW;
                sLodTimer = 1;
                break;
            }
            bzero(sLodNoise, SCREEN_WIDTH * SCREEN_HEIGHT / 8);
            Lod_Test(-1, 0);
            sSweepState = SW_NOISE;
            break;
        }

        case SW_NOISE:
            if (Lod_Compare(true) > 0) {
#ifdef FISHMOD_LOD_SHOW
                // diagnosis: the first noisy view's moving pixels (and its number) to SRAM 0x1000
                static s32 sShown;

                if (!sShown) {
                    u32 v = sSweepView;

                    sShown = true;
                    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_LOD_DIAG), &v, 4, OS_WRITE);
                    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_LOD_DIAG) + 0x10, sLodNoise,
                                     SCREEN_WIDTH * SCREEN_HEIGHT / 8, OS_WRITE);
                    // and row 200 of the reference and of this frame
                    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_LOD_DIAG) + 0x2800, &sLodRef[200 * SCREEN_WIDTH],
                                     SCREEN_WIDTH * 2, OS_WRITE);
                    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_LOD_DIAG) + 0x2800 + SCREEN_WIDTH * 2,
                                     &sLodCur[200 * SCREEN_WIDTH], SCREEN_WIDTH * 2, OS_WRITE);
                }
#endif
                sSweep.noisy++;
                sSweepState = SW_NEXT_VIEW;
                sLodTimer = 1;
                break;
            }
            sSweepRetry = 0;
            sSweepFirstTest = true;
            // fall through
        case SW_ALL: {
            u64 mask = 0;
            s32 k;

            for (k = 0; k < gFishmodLodTest.kindCount; k++) {
#ifdef FISHMOD_LOD_TUNE
                // (only the kinds that have the setting being measured)
                if (((sTuneKnob == 1) && !(gFishmodLodTest.hasImp & (1ULL << k))) ||
                    ((sTuneKnob == 2) && !(gFishmodLodTest.hasHalf & (1ULL << k)))) {
                    continue;
                }
#endif
                if (sLodDrawn[k] != 0) {
                    *Sweep_Need(k) = MAX(*Sweep_Need(k), 1); // seen in this zone (0: never seen, no measure)
                    if (*Sweep_Need(k) < SW_STEPS) {
                        mask |= 1ULL << k;
                    }
                }
            }
            if (mask == 0) {
                sSweepState = SW_KNOB_DONE;
                sLodTimer = 1;
                break;
            }
            sSweepTop = 0;
            sSweepStack[sSweepTop++] = mask;
            sSweepCulprits = 0;
            sSweepMask = mask;
            Sweep_Test(mask);
            sSweepState = SW_SPLIT_CHECK;
            break;
        }

        case SW_SPLIT_CHECK: {
            s32 changed = Lod_Compare(false);

            sSweepTop--; // the mask just tested was on top
            if (sSweepTop == 0) {
                sSweepWhole = (changed > LOD_PIXELS) ? sSweepMask : 0;
            }
            if (changed > LOD_PIXELS) {
                if (sSweepFirstTest) {
                    sSweep.splits++;
                }
                if ((sSweepMask & (sSweepMask - 1)) == 0) {
                    sSweepCulprits |= sSweepMask;
                } else {
                    // halves: alternate bits
                    u64 a = 0;
                    u64 b = 0;
                    u64 m = sSweepMask;
                    s32 odd = 0;

                    while (m != 0) {
                        u64 bit = m & (~m + 1);

                        if (odd) {
                            a |= bit;
                        } else {
                            b |= bit;
                        }
                        odd ^= 1;
                        m &= m - 1;
                    }
                    sSweepStack[sSweepTop++] = a;
                    sSweepStack[sSweepTop++] = b;
                }
            } else if (sSweepFirstTest) {
                sSweep.passFirst++;
            }
            sSweepFirstTest = false;
        }
            // fall through
        case SW_SPLIT_NEXT:
            if (sSweepTop > 0) {
                sSweepMask = sSweepStack[sSweepTop - 1];
                Sweep_Test(sSweepMask);
                sSweepState = SW_SPLIT_CHECK;
                break;
            }
            if (sSweepCulprits == 0) {
                if (sSweepWhole != 0) {
                    // the kinds together change the frame, but none alone: each a step further, and again
                    s32 k;

                    for (k = 0; k < gFishmodLodTest.kindCount; k++) {
                        if (sSweepWhole & (1ULL << k)) {
                            *Sweep_Need(k) = MIN(MAX(*Sweep_Need(k), 1) + 1, SW_STEPS);
                        }
                    }
                    if (++sSweepRetry < SWEEP_RETRIES) {
                        sSweepState = SW_ALL;
                        sLodTimer = 1;
                        break;
                    }
                }
                sSweepState = SW_KNOB_DONE;
                sLodTimer = 1;
                break;
            }
            sLodKind = -1;
            // fall through
        case SW_SEARCH:
            if (sLodKind >= 0) {
                s32 mid = (sLodLo + sLodHi) / 2;

                if (Lod_Compare(false) <= LOD_PIXELS) {
                    sLodHi = mid;
                } else {
                    sLodLo = mid + 1;
                }
                if (sLodLo < sLodHi) {
                    Sweep_TestKind(sLodKind, ((sLodLo + sLodHi) / 2) * SW_STEP);
                    sSweep.tests++;
                    sSweepState = SW_SEARCH;
                    break;
                }
                *Sweep_Need(sLodKind) = MAX(*Sweep_Need(sLodKind), sLodHi);
                sSweepCulprits &= ~(1ULL << sLodKind);
            }
            if (sSweepCulprits != 0) {
                s32 k = 0;

                while (!(sSweepCulprits & (1ULL << k))) {
                    k++;
                }
                sLodKind = k;
                sLodLo = MAX(*Sweep_Need(k), 1) + 1;
                sLodHi = SW_STEPS;
                if (sLodLo >= sLodHi) {
                    *Sweep_Need(k) = SW_STEPS;
                    sSweepCulprits &= ~(1ULL << k);
                    sLodKind = -1;
                    sLodTimer = 1;
                    sSweepState = SW_SEARCH;
                    break;
                }
                Sweep_TestKind(k, ((sLodLo + sLodHi) / 2) * SW_STEP);
                sSweep.tests++;
                sSweepState = SW_SEARCH;
                break;
            }
            // the culprits raised: the whole view again
            if (++sSweepRetry < SWEEP_RETRIES) {
                sSweepState = SW_ALL;
                sLodTimer = 1;
                break;
            }
            sSweepState = SW_KNOB_DONE;
            sLodTimer = 1;
            break;

        case SW_NEXT_KNOB:
#ifdef FISHMOD_LOD_TUNE
            // the next setting, against the same reference
            if (++sTuneKnob < TUNE_KNOBS) {
                sSweepRetry = 0;
                sSweepFirstTest = true;
                sSweepState = SW_ALL;
                sLodTimer = 1;
                break;
            }
#endif
            // fall through
        case SW_NEXT_VIEW:
#ifdef FISHMOD_LOD_TUNE
            sTuneKnob = 0;
#endif
            sSweep.done++;
            Sweep_Save();
            sSweepView++;
            if (sSweepView >= Sweep_PartEnd()) {
                Sweep_Save();
                sSweepState = SW_DONE;
                gFishmodLodTest.active = false;
                break;
            }
            Sweep_Place();
            Lod_Test(-1, 0);
            sSweepSettle = SWEEP_SETTLE;
            sSweepState = SW_ARRIVE;
            break;
    }
}
#elif defined(FISHMOD_LOD_ROTATE)
/*
 * The rotation check: at each tour stop the camera turns all the way round (16 directions, about the point it looks
 * at, as the stick turns it), and at each the game's frame is compared with two references: the same props with a
 * much wider view cone (what the cone wrongly leaves out), and everything out to DIST_CALIBRATE with that cone (what
 * any cut leaves out: the cone and the distances). Pixels differing, per stop and direction, to SRAM 0x7A40 for
 * fishmod/tools/lod_calibrate.py --rotate.
 */
#define ROT_YAWS 16
#define ROT_STOPS 22

typedef struct RotResults {
    u32 magic; // 'FROT'
    u16 stops;
    u16 yaws;
    u16 cone[ROT_STOPS][ROT_YAWS];
    u16 all[ROT_STOPS][ROT_YAWS];
} RotResults;

static RotResults sRot;
static s32 sRotState; // 0 arriving; then per direction: 1 the game's frame, 2 the wide cone, 3 everything
static s32 sRotYaw;

static void Rot_Config(s32 state) {
    gFishmodLodTest.asGame = (state != 3);
    gFishmodLodTest.wide = (state >= 2);
    bzero(gFishmodLodTest.dist, sizeof(gFishmodLodTest.dist));
    sLodTimer = LOD_HOLD;
}

static void Fishmod_LodUpdate(PlayState* play) {
    s32 stop = sTourStop;

    sLodGfxCtx = play->state.gfxCtx;
    if (sLodRef == NULL) {
        sLodRef = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        sLodNoise = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT / 8, "../fishmod_debug.c", __LINE__);
        sLodCur = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        if ((sLodRef == NULL) || (sLodNoise == NULL) || (sLodCur == NULL)) {
            return;
        }
        bzero(sLodNoise, SCREEN_WIDTH * SCREEN_HEIGHT / 8);
        sLodFrames = play->gameplayFrames;
        sLodTime = gSaveContext.save.dayTime;
        sLodSkyRot = play->skyboxCtx.rot.y;
        sLodStateFrames = play->state.frames;
        gFishmodLodTest.active = true;
        Rot_Config(1);
    }
    play->gameplayFrames = sLodFrames;
    gSaveContext.save.dayTime = sLodTime;
    play->skyboxCtx.rot.y = sLodSkyRot;
    play->state.frames = sLodStateFrames;
    play->haltAllActors = gFishmodLodTest.active;
    if (!gFishmodLodTest.active) {
        Fishmod_DebugPrintf(2, "rotation check done");
        return;
    }
    Fishmod_DebugPrintf(2, "rotation stop %d dir %d", stop + 1, sRotYaw);
    sTourYaw = sRotYaw * (2.0f * M_PI / ROT_YAWS);
    if (sRotState == 0) {
        if (sTourFrame >= TOUR_GLIDE + LOD_SETTLE + ((stop == 0) ? 200 : 0)) {
            sRotState = 1;
            sRotYaw = 0;
            Rot_Config(1);
        }
        return;
    }
    if (--sLodTimer > 0) {
        return;
    }
    {
        // a direction that takes the camera out of the water or into it: wait for the under-water look to finish
        // changing (it eases in and out), or the frames compared would differ by it
        static s32 sHazeWait;

        if ((gFishmodHaze > 0.001f) && (gFishmodHaze < 0.999f) && (sHazeWait < 600)) {
            sHazeWait++;
            return;
        }
        sHazeWait = 0;
    }
    switch (sRotState) {
        case 1:
            Lod_Grab(sLodRef); // the game's frame
            Rot_Config(2);
            sRotState = 2;
            break;
        case 2:
            Lod_Grab(sLodCur);
            sRot.cone[stop][sRotYaw] = MIN(Lod_Compare(false), 0xFFFF);
            Rot_Config(3);
            sRotState = 3;
            break;
        default:
            Lod_Grab(sLodCur);
            sRot.all[stop][sRotYaw] = MIN(Lod_Compare(false), 0xFFFF);
            if (++sRotYaw >= ROT_YAWS) {
                sRot.magic = 0x46524F54; // 'FROT'
                sRot.stops = stop + 1;
                sRot.yaws = ROT_YAWS;
                SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_LOD_ROTATE), &sRot, sizeof(sRot), OS_WRITE);
                sRotYaw = 0;
                if (stop + 1 >= ROT_STOPS) {
                    gFishmodLodTest.active = false;
                    gFishmodLodTest.asGame = gFishmodLodTest.wide = false;
                    break;
                }
                Fishmod_TourGo(stop + 1);
                sRotState = 0;
                break;
            }
            Rot_Config(1);
            sRotState = 1;
            break;
    }
}
#else
static void Fishmod_LodUpdate(PlayState* play) {
    s32 under = gFishmodLodTest.under;
    s32 stop = sTourStop;

    sLodGfxCtx = play->state.gfxCtx;

    if (sLodRef == NULL) {
        sLodRef = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        sLodNoise = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT / 8, "../fishmod_debug.c", __LINE__);
        sLodCur = SYSTEM_ARENA_MALLOC(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), "../fishmod_debug.c", __LINE__);
        sLodSkyRot = play->skyboxCtx.rot.y;
        sLodStateFrames = play->state.frames;
        sLodFrames = play->gameplayFrames;
        sLodTime = gSaveContext.save.dayTime;
        gFishmodLodTest.active = true;
        if ((sLodRef == NULL) || (sLodNoise == NULL) || (sLodCur == NULL)) {
            sLodState = LOD_DONE;
        }
    }
    // everything that moves, held still
    play->gameplayFrames = sLodFrames;
    gSaveContext.save.dayTime = sLodTime;
    play->skyboxCtx.rot.y = sLodSkyRot; // the sky turns slowly (and shows through the water from below)
    play->state.frames = sLodStateFrames; // the lake's water scrolls by this count
    play->haltAllActors = gFishmodLodTest.active; // Link, the NPCs, birds: still (drawn, not updated)
    if (sLodState == LOD_DONE) {
        Fishmod_DebugPrintf(2, "LOD calibration done: %d stops", sLod.stops);
        return;
    }
    Fishmod_DebugPrintf(2, "LOD stop %d kind %d [%d-%d] %s", stop + 1, sLodKind, sLodLo * LOD_STEP, sLodHi * LOD_STEP,
                        under ? "under" : "above");
    if ((sLodState != LOD_ARRIVE) && (--sLodTimer > 0)) {
        return;
    }
    if (sLodFresh) {
        if (!Lod_Grab(sLodRef)) {
            sLodTimer = 1; // torn: again next frame
            return;
        }
        bcopy(sLodPending, gFishmodLodTest.dist, sizeof(sLodPending));
        sLodFresh = false;
        sLodTimer = LOD_HOLD;
        return;
    }

    // every state past this reads the frame (the noise passes, the tests, the check)
    if ((sLodState == LOD_NOISE) || (sLodState == LOD_SEARCH) || (sLodState == LOD_VERIFY)) {
        if (!Lod_Grab(sLodCur)) {
            sLodTimer = 1;
            return;
        }
    }

    switch (sLodState) {
        case LOD_ARRIVE:
            // the first stop waits longer: the scene's lights are still coming up after it loads
            if (sTourFrame >= TOUR_GLIDE + LOD_SETTLE + ((stop == 0) ? 200 : 0)) {
                Lod_Test(-1, 0);
                sLodState = LOD_REF;
            }
            break;

        case LOD_REF:
            if (!Lod_Grab(sLodRef)) {
                sLodTimer = 1;
                return;
            }
            bcopy(gFishmodLodTest.drawn, sLodDrawn, sizeof(sLodDrawn));
            bzero(sLodNoise, SCREEN_WIDTH * SCREEN_HEIGHT / 8);
            sLodPass = 0;
            sLod.noise[stop] = 0;
            Lod_Test(-1, 0);
            sLodState = LOD_NOISE;
            break;

        case LOD_NOISE:
            sLod.noise[stop] += Lod_Compare(true);
            if (++sLodPass < LOD_NOISE_REFS) {
                Lod_Test(-1, 0);
                break;
            }
            sLodKind = -1;
            sLodState = LOD_NEXT_KIND;
            // fall through
        case LOD_NEXT_KIND:
            do {
                sLodKind++;
            } while ((sLodKind < gFishmodLodTest.kindCount) && (sLodDrawn[sLodKind] == 0));
            if (sLodKind >= gFishmodLodTest.kindCount) {
                s32 k;

                // every kind at the distances found so far, together
                for (k = 0; k < gFishmodLodTest.kindCount; k++) {
                    sLodPending[k] = MAX(sLod.need[under][k], LOD_STEP);
                }
                Lod_TestPending();
                sLodState = LOD_VERIFY;
                break;
            }
            sLodLo = 1;
            sLodHi = LOD_STEPS;
            Lod_Test(sLodKind, ((sLodLo + sLodHi) / 2) * LOD_STEP);
            sLodState = LOD_SEARCH;
            break;

        case LOD_SEARCH: {
            s32 mid = (sLodLo + sLodHi) / 2;

            s32 changed = Lod_Compare(false);

#ifdef FISHMOD_LOD_SHOW
            // diagnosis: masks of what differs, to SRAM (over the save slots): the first small failure, the noise
            // mask, the first large failure
            {
                static s32 sDumped;
                s32 slot = (changed > 3000) ? 2 : (changed > LOD_PIXELS) ? 0 : -1;

                if ((slot >= 0) && !(sDumped & (1 << slot))) {
                    static u8 sMask[SCREEN_WIDTH * SCREEN_HEIGHT / 8];
                    u16* fb = sLodCur;
                    s32 i;

                    sDumped |= 1 << slot;
                    bzero(sMask, sizeof(sMask));
                    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
                        u16 x = fb[i];
                        u16 y = sLodRef[i];

                        if ((ABS((s32)((x >> 11) & 31) - (s32)((y >> 11) & 31)) > LOD_THRESHOLD) ||
                            (ABS((s32)((x >> 6) & 31) - (s32)((y >> 6) & 31)) > LOD_THRESHOLD) ||
                            (ABS((s32)((x >> 1) & 31) - (s32)((y >> 1) & 31)) > LOD_THRESHOLD)) {
                            sMask[i >> 3] |= 1 << (i & 7);
                        }
                    }
                    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_BENCH) + slot * sizeof(sMask), sMask, sizeof(sMask),
                                     OS_WRITE);
                    SsSram_ReadWrite(FISHMOD_SRAM_ADDR(FISHMOD_SRAM_BENCH) + sizeof(sMask), sLodNoise, sizeof(sMask),
                                     OS_WRITE);
                }
            }
#endif

            if (sLod.logCount < ARRAY_COUNT(sLod.log)) {
                sLod.log[sLod.logCount][0] = sLodKind;
                sLod.log[sLod.logCount][1] = mid * LOD_STEP;
                sLod.log[sLod.logCount][2] = changed;
                sLod.logCount++;
            }
            if (changed <= LOD_PIXELS) {
                sLodHi = mid; // stopping there changes nothing that shows
            } else {
                sLodLo = mid + 1;
            }
            if (sLodLo < sLodHi) {
                Lod_Test(sLodKind, ((sLodLo + sLodHi) / 2) * LOD_STEP);
            } else {
                sLod.need[under][sLodKind] = MAX(sLod.need[under][sLodKind], sLodHi * LOD_STEP);
                sLodState = LOD_NEXT_KIND;
                sLodTimer = 1;
            }
            break;
        }

        case LOD_VERIFY:
            sLod.verify[stop] = Lod_Compare(false);
            sLod.under[stop] = under;
            sLod.stops = stop + 1;
            Lod_Save();
            if (stop + 1 >= (s32)ARRAY_COUNT(sTourStops)) {
                sLodState = LOD_DONE;
                gFishmodLodTest.active = false;
            } else {
                Fishmod_TourGo(stop + 1);
                Lod_Test(-1, 0);
                sLodState = LOD_ARRIVE;
            }
            break;
    }
}
#endif // FISHMOD_LOD_SWEEP
#endif
