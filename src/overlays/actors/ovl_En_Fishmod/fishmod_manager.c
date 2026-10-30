/*
 * File: fishmod_manager.c
 * Overlay: ovl_En_Fishmod
 * Description: the fish manager, type FISHMOD_TYPE_MANAGER: one per scene, spawned by the first
 * fishmod actor that needs it.
 *
 * - Roster: the scene's fish are records (species, size, home) made from a seed of the scene and the in-game day, so
 *   the same fish are there every visit that day. A fish only exists as an actor while Link is near its home; when
 *   he goes far away it is removed and comes back at its home later. Fish that were kept are gone for the day.
 * - Drawing: fish actors have no draw function of their own. The manager draws them all at once, sorted by species,
 *   with one light setup, choosing the full rig, the reduced rig or the flat far shape by distance.
 * - A frame budget for the AI of fish that are just cruising far from Link and the bobber.
 */

#include "z_en_fishmod.h"
#include "gfx.h"
#include "gfx_setupdl.h"
#include "rand.h"
#include "libc64/math64.h"
#include "libc64/qrand.h"
#include "segmented_address.h"
#include "sys_matrix.h"
#include "z_lib.h"
#include "play_state.h"
#include "player.h"
#include "save.h"
#include "bgcheck.h"
#include "scene.h"
#include "libc/math.h"


#define ROSTER_WAKE_DIST 1400.0f  // a fish appears at its home when Link comes this close
#define ROSTER_SLEEP_DIST 1800.0f // and is removed when he's this far away (while it's just cruising)
#define ROSTER_CHECKS_PER_FRAME 48 // roster records looked at per frame
#define ROSTER_SPAWNS_PER_FRAME 2  // fish woken per frame
#define HOME_CROWD_DIST 300.0f    // a fish's home by cover is skipped if this close to
#define HOME_CROWD_MAX 6          // this many other homes already (dense weed drew too many fish to one spot)
#define THINK_BUDGET 16           // full AI updates per frame for fish cruising far from Link and the bobber
#define THINK_FAR_DIST 300.0f
#define LOD_FULL_DIST 300.0f      // closer than this: the full rig (and whiskers); then the reduced rig to the far shape
#define DRAW_WHISKERS 0           // catfish whiskers are off for now: too thin to see (to be redone)

typedef struct FishmodRosterFish {
    Vec3f home;
    f32 length;
    s16 yaw;
    u8 species;
    u8 caught;
    EnFishmod* fish;
    FishmodMood mood; // while the fish isn't spawned (spawned, the fish's own is the one that counts)
    u16 leader;       // the record of its school's leader (its own for a leader or a fish on its own)
    u8 look;          // its looks: the same fish every visit (from the roster's seed)
} FishmodRosterFish;

static FishmodRosterFish sRoster[FISHMOD_ROSTER_MAX];
static s32 sRosterCount;
static EnFishmod* sManager;
static u8 sManagerSpawning;
static EnFishmodShared sShared;

/* ---------------------------------------------------------------------------------------------------------------
 * Shared per-frame information
 */

EnFishmodShared* EnFishmod_Shared(PlayState* play) {
    if (sShared.frame != play->gameplayFrames) {
        Player* player = GET_PLAYER(play);

        sShared.frame = play->gameplayFrames;
        sShared.linkPos = player->actor.world.pos;
        sShared.camPos = sShared.linkPos;
        // attention follows the camera, not Link (a NaN guard: a camera being set up can give one for a frame)
        if ((play->view.eye.x == play->view.eye.x) && (play->view.eye.z == play->view.eye.z) &&
            (fabsf(play->view.eye.x) < 30000.0f) && (fabsf(play->view.eye.z) < 30000.0f)) {
            sShared.camPos = play->view.eye;
        }
        sShared.rosterFocus = sShared.camPos;
        sShared.bobberIn = EnFishmod_RodBobberInWater();
        sShared.bobberPos = gFishmodRodState.bobberPos;
        sShared.thinksLeft = THINK_BUDGET;
        sShared.light = Fishmod_Light(play);
        sShared.raining = (Fishmod_Weather(play) == FISHMOD_WEATHER_RAIN);
    }
    return &sShared;
}

/* ---------------------------------------------------------------------------------------------------------------
 * Roster
 */

/** The record of a fish the manager spawned, or NULL. */
static FishmodRosterFish* Manager_Record(EnFishmod* fish) {
    if ((fish->rosterIndex == FISHMOD_NO_ROSTER) || (fish->rosterIndex >= sRosterCount)) {
        return NULL;
    }
    return &sRoster[fish->rosterIndex];
}

/** The spawned fish of a roster record, or NULL. */
EnFishmod* EnFishmod_RosterFish(s32 index) {
    return ((index >= 0) && (index < sRosterCount)) ? sRoster[index].fish : NULL;
}

f32 EnFishmod_RosterLength(s32 index) {
    return ((index >= 0) && (index < sRosterCount)) ? sRoster[index].length : 0.0f;
}

/** The fish was kept (put in a bottle): it's gone from its home water for the rest of the day. */
void EnFishmod_RosterCaught(PlayState* play, EnFishmod* fish) {
    FishmodRosterFish* rec = Manager_Record(fish);

    if (rec != NULL) {
        rec->caught = true;
        rec->fish = NULL;
        Fishmod_RosterSetCaught(play->sceneId, gSaveContext.save.totalDays, fish->rosterIndex);
    }
}

/** A fish is going away (removed by the manager, kept, or the scene is ending). */
void EnFishmod_RosterForget(EnFishmod* fish) {
    FishmodRosterFish* rec = Manager_Record(fish);

    if ((rec != NULL) && (rec->fish == fish)) {
        rec->mood = fish->mood;
        rec->fish = NULL;
    }
}

/** How many of the roster's homes so far are within HOME_CROWD_DIST of a point (across the ground). */
static s32 Roster_Crowd(Vec3f* pos) {
    s32 n = 0;
    s32 i;

    for (i = 0; i < sRosterCount; i++) {
        f32 dx = sRoster[i].home.x - pos->x;
        f32 dz = sRoster[i].home.z - pos->z;

        if (SQ(dx) + SQ(dz) < SQ(HOME_CROWD_DIST)) {
            n++;
        }
    }
    return n;
}

/**
 * The scene's fish for today: the same as Rod_SpawnFish chose them before (a random water box, a spot in it, a
 * species by weight, a size), but from a seed of the scene and the day so they're the same every visit.
 */
static void Manager_BuildRoster(PlayState* play) {
    const FishmodSceneSpawns* spawns = Fishmod_GetSceneSpawns(play->sceneId);
    WaterBox* boxes;
    s32 numBoxes = play->colCtx.colHeader->numWaterBoxes;
    s32 totalWeight = 0;
    s32 count;
    s32 i;
    s32 n;

    sRosterCount = 0;
    if ((spawns == NULL) || (numBoxes == 0) || gFishmodPhoto.active) {
        return;
    }
    boxes = SEGMENTED_TO_VIRTUAL(play->colCtx.colHeader->waterBoxes);
    for (i = 0; i < spawns->count; i++) {
        totalWeight += spawns->entries[i].weight;
    }

    // the shared random numbers, seeded for the roster and reseeded from the clock afterwards
    Rand_Seed((u32)play->sceneId * 7919 + (u32)gSaveContext.save.totalDays * 104729 + 17);
    count = FISHMOD_FISH_COUNT;
    if (spawns->fish > 0) {
        count = MIN(spawns->fish, FISHMOD_ROSTER_MAX);
    }
#if FISHMOD_DEBUG
    if (Fishmod_DebugRosterCount() > 0) {
        count = MIN(Fishmod_DebugRosterCount(), FISHMOD_ROSTER_MAX);
    }
#endif
    for (n = 0; n < count; n++) {
        WaterBox* box = &boxes[(s32)Rand_ZeroFloat(numBoxes - 0.01f)];
        FishmodRosterFish* rec = &sRoster[sRosterCount];
        const FishmodSpecies* species;
        Vec3f center;
        f32 radius;
        f32 surface;
        f32 floor;
        s32 roll = (s32)Rand_ZeroFloat(totalWeight - 0.01f);
        s32 s = spawns->entries[0].species;
        f32 lo;
        f32 hi;
        f32 mode;
        f32 u;
        f32 f;

        for (i = 0; i < spawns->count; i++) {
            if (roll < spawns->entries[i].weight) {
                s = spawns->entries[i].species;
                break;
            }
            roll -= spawns->entries[i].weight;
        }
        species = &gFishmodSpecies[s];

        // size: triangular between min and max, peaking at the typical size (as EnFishmod_Init)
        lo = species->lengthMin;
        hi = species->lengthMax;
        mode = (species->lengthTypical - lo) / (hi - lo);
        u = Rand_ZeroOne();
        f = (u < mode) ? sqrtf(u * mode) : 1.0f - sqrtf((1.0f - u) * (1.0f - mode));

        center.x = box->xMin + box->xLength * 0.5f;
        center.y = box->ySurface;
        center.z = box->zMin + box->zLength * 0.5f;
        radius = CLAMP_MIN(box->xLength, box->zLength) * 0.6f;
        // most fish live by cover (weed beds, rocks, reeds, pads), the rest out in the open water
        // (a few tries for cover that isn't crowded with homes already; else the open water)
        if (Rand_ZeroOne() < 0.75f) {
            Vec3f cover;
            s32 tries;

            for (tries = 0; tries < 4; tries++) {
                if (!EnFishmod_ScatterRandomCover(&cover)) {
                    break;
                }
                if (Roster_Crowd(&cover) < HOME_CROWD_MAX) {
                    center = cover;
                    radius = 120.0f;
                    break;
                }
            }
        }
        if (EnFishmod_FindWaterPoint(play, &center, radius, 25.0f, &rec->home, &surface, &floor)) {
            rec->species = s;
            rec->length = lo + (hi - lo) * f;
            rec->yaw = (s16)Rand_ZeroFloat(0xFFFF);
            rec->caught = Fishmod_RosterIsCaught(play->sceneId, gSaveContext.save.totalDays, sRosterCount);
            rec->fish = NULL;
            rec->look = (u8)Rand_ZeroFloat(256.0f);
            // some hungry, some not (from the index, not the shared random numbers)
            rec->mood.hunger = 40 + ((sRosterCount * 97 + play->sceneId * 31) % 170);
            rec->mood.fear = 0;
            rec->leader = sRosterCount;
            sRosterCount++;
            // schooling fish: the rest of its school around it, about its size (they count toward the total)
            if (species->schoolMax > 0) {
                s32 leader = sRosterCount - 1;
                s32 more = species->schoolMin - 1 +
                           (s32)Rand_ZeroFloat(species->schoolMax - species->schoolMin + 0.99f);

                while ((more-- > 0) && (n + 1 < count) && (sRosterCount < FISHMOD_ROSTER_MAX)) {
                    FishmodRosterFish* mate = &sRoster[sRosterCount];

                    *mate = sRoster[leader];
                    if (!EnFishmod_FindWaterPoint(play, &sRoster[leader].home, 30.0f, 15.0f, &mate->home, &surface,
                                                  &floor)) {
                        continue;
                    }
                    mate->length = CLAMP(sRoster[leader].length * (0.8f + Rand_ZeroOne() * 0.4f), lo, hi);
                    mate->caught = Fishmod_RosterIsCaught(play->sceneId, gSaveContext.save.totalDays, sRosterCount);
                    mate->look = (u8)Rand_ZeroFloat(256.0f);
                    mate->mood.hunger = 40 + ((sRosterCount * 97 + play->sceneId * 31) % 170);
                    mate->leader = leader;
                    sRosterCount++;
                    n++;
                }
            }
        }
    }
    Rand_Seed(FISHMOD_TIME_SEED());
}

#define MOOD_SLICE 16 // roster records whose mood moves on each frame: 300 fish come round about once a second

/** The mood that counts for a record: the fish's own while it's spawned. */
static FishmodMood* Manager_Mood(FishmodRosterFish* rec) {
    return (rec->fish != NULL) ? &rec->fish->mood : &rec->mood;
}

/**
 * Moods move on (a slice of the roster each frame). Hunger rises by 1 to 4 a turn with how active the species is
 * at this time of day, and a fish feeds by itself when it gets to 200 (so from fed to hungry takes under a minute to
 * a few minutes, and hunger cycles round the middle). Fear fades by 3 a turn (terrified to calm in under 90 s).
 */
static void Manager_UpdateMoods(void) {
    static s32 sNext;
    s32 n = MIN(sRosterCount, MOOD_SLICE);
    s32 time = gSaveContext.save.dayTime;
    s32 tod = ((time >= CLOCK_TIME(4, 0)) && (time < CLOCK_TIME(8, 0)))    ? FISHMOD_TIME_DAWN
              : ((time >= CLOCK_TIME(8, 0)) && (time < CLOCK_TIME(17, 0))) ? FISHMOD_TIME_DAY
              : ((time >= CLOCK_TIME(17, 0)) && (time < CLOCK_TIME(20, 0))) ? FISHMOD_TIME_DUSK
                                                                            : FISHMOD_TIME_NIGHT;

    while (n-- > 0) {
        FishmodRosterFish* rec;
        FishmodMood* mood;
        s32 hunger;

        if (sNext >= sRosterCount) {
            sNext = 0;
        }
        rec = &sRoster[sNext++];
        mood = Manager_Mood(rec);
        hunger = mood->hunger + 1 + FISHMOD_ACTIVITY(&gFishmodSpecies[rec->species], tod);
        if (hunger >= 200) {
            hunger -= 150; // it found something to eat (so hunger cycles round the middle, not up to starving)
        }
        mood->hunger = CLAMP_MAX(hunger, 255);
        mood->fear = CLAMP_MIN(mood->fear - 3, 0);
    }
}

/**
 * Something disturbed the water at `pos` (a bobber landing, a fish hooked and thrashing): fish within `radius` get
 * up to `fear` more of it, less the further off they are. Once per event over the roster, not every fish every frame.
 */
void EnFishmod_Disturb(PlayState* play, Vec3f* pos, f32 radius, s32 fear) {
    EnFishmodShared* shared = EnFishmod_Shared(play);
    s32 i;

    if (fear <= 0) {
        // only a plop, which the hungry may come to see (EnFishmod_Update's notice check)
        shared->splashPos = *pos;
        shared->splashFrame = play->gameplayFrames;
        return;
    }
    for (i = 0; i < sRosterCount; i++) {
        FishmodRosterFish* rec = &sRoster[i];
        f32 d = Math_Vec3f_DistXYZ((rec->fish != NULL) ? &rec->fish->actor.world.pos : &rec->home, pos);

        if (d < radius) {
            FishmodMood* mood = Manager_Mood(rec);

            mood->fear = CLAMP_MAX(mood->fear + (s32)(fear * (1.0f - d / radius)), 255);
        }
    }
}

/** Spawns the fish whose homes Link is near and removes the ones he's left far behind. */
static void Manager_UpdateRoster(EnFishmod* this, PlayState* play) {
    EnFishmodShared* shared = EnFishmod_Shared(play);
    s32 i;
    // Staggered: a slice of the roster each frame, and only a few new fish at a time, so a big roster never costs
    // one frame a lot (arriving at the lake would otherwise spawn dozens of fish at once).
    static s32 sNext;
    s32 checks = MIN(sRosterCount, ROSTER_CHECKS_PER_FRAME);
    s32 spawns = ROSTER_SPAWNS_PER_FRAME;
    s32 n;

    for (n = 0; n < checks; n++) {
        FishmodRosterFish* rec;
        f32 dist;

        i = sNext;
        sNext = (sNext + 1 < sRosterCount) ? sNext + 1 : 0;
        rec = &sRoster[i];
        dist = Math_Vec3f_DistXZ(&shared->rosterFocus, &rec->home);
        if (rec->caught) {
            continue;
        }
        if ((rec->fish == NULL) && (dist < ROSTER_WAKE_DIST) && (spawns-- > 0)) {
            // the record's index rides in the spawn roll (the params only have room for 31)
            Actor* fish = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, rec->home.x, rec->home.y, rec->home.z,
                                      0, rec->yaw, i + 1, FISHMOD_PARAMS(FISHMOD_TYPE_FISH, rec->species));

            if (fish != NULL) {
                fish->room = -1;
                rec->fish = (EnFishmod*)fish;
                rec->fish->look = rec->look;
                rec->fish->mood = rec->mood;
                rec->fish->leaderIndex = (rec->leader != i) ? rec->leader : FISHMOD_NO_ROSTER;
            }
        } else if ((rec->fish != NULL) && (dist > ROSTER_SLEEP_DIST) &&
                   (rec->fish->fishState == FISH_STATE_WANDER) && (gFishmodRodState.fish != rec->fish)) {
            // it swims back home while nobody's looking
            rec->mood = rec->fish->mood;
            Actor_Kill(&rec->fish->actor);
            rec->fish = NULL;
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Actor
 */

/** Spawns the manager if the scene doesn't have one yet. */
void EnFishmod_ManagerEnsure(PlayState* play) {
    if ((sManager == NULL) && !sManagerSpawning) {
        Player* player = GET_PLAYER(play);

        sManagerSpawning = true;
        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, player->actor.world.pos.x, player->actor.world.pos.y,
                    player->actor.world.pos.z, 0, 0, 0, FISHMOD_PARAMS(FISHMOD_TYPE_MANAGER, 0));
    }
}

void EnFishmod_ManagerInit(EnFishmod* this, PlayState* play) {
    sManager = this;
    sManagerSpawning = false;
    this->actor.room = -1;
    this->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    // after the fish in the actor lists, so their culling is already worked out when the manager draws them
    Actor_ChangeCategory(play, &play->actorCtx, &this->actor, ACTORCAT_MISC);
    EnFishmod_WaterGridAcquire(play);
    EnFishmod_ScatterLoad(play);
    Manager_BuildRoster(play);
}

void EnFishmod_ManagerDestroy(EnFishmod* this, PlayState* play) {
    s32 i;

    for (i = 0; i < sRosterCount; i++) {
        sRoster[i].fish = NULL;
    }
    sRosterCount = 0;
    EnFishmod_RigFreeAll();
    EnFishmod_ScatterUnload();
    EnFishmod_WaterGridRelease(play);
    if (sManager == this) {
        sManager = NULL;
    }
}

void EnFishmod_ManagerUpdate(EnFishmod* this, PlayState* play) {
    EnFishmodShared* shared = EnFishmod_Shared(play);

    // point lights for the fish are bound at the manager, so it follows Link
    this->actor.world.pos = shared->linkPos;
    EnFishmod_WaterNewFrame();
    if ((this->timer & 7) == 0) {
        Manager_UpdateRoster(this, play);
    }
    Manager_UpdateMoods();
    EnFishmod_ParticlesUpdate(play);
}

/**
 * How far a fish is from attention: the camera or the bobber in the water, whichever is
 * nearer. The AI does less the farther away it is (the drawing's detail goes by the camera alone).
 */
f32 EnFishmod_AttentionDist(EnFishmod* this, PlayState* play) {
    EnFishmodShared* shared = EnFishmod_Shared(play);
    f32 dist = Math_Vec3f_DistXZ(&this->actor.world.pos, &shared->camPos);

    if (shared->bobberIn) {
        dist = CLAMP_MAX(dist, Math_Vec3f_DistXZ(&this->actor.world.pos, &shared->bobberPos));
    }
    return dist;
}

/** May this fish run its full AI this frame? Cruising fish far from Link and the bobber share a budget. */
s32 EnFishmod_ThinkAllowed(EnFishmod* this, PlayState* play) {
    EnFishmodShared* shared = EnFishmod_Shared(play);

    if ((this->fishState != FISH_STATE_WANDER) || gFishmodPhoto.active ||
        (Math_Vec3f_DistXZ(&this->actor.world.pos, &shared->camPos) < THINK_FAR_DIST) ||
        (shared->bobberIn && (Math_Vec3f_DistXZ(&this->actor.world.pos, &shared->bobberPos) < THINK_FAR_DIST))) {
        return true;
    }
    // nobody waits more than a few frames, whatever the budget says
    if ((shared->thinksLeft > 0) || ((s16)(this->timer - this->lastThink) >= 4)) {
        shared->thinksLeft--;
        this->lastThink = this->timer;
        return true;
    }
    return false;
}

#define DRAW_MAX 128

typedef struct ManagerDrawItem {
    EnFishmod* fish;
    u8 variant;
    u8 whiskers;
} ManagerDrawItem;

void EnFishmod_ManagerDraw(EnFishmod* this, PlayState* play) {
    // static: too big for the graphics thread's stack
    static ManagerDrawItem full[DRAW_MAX];
    static EnFishmod* far[DRAW_MAX];
    s32 fullCount = 0;
    s32 farCount = 0;
    Actor* actor;
    s32 i;
    s32 j;
    s32 lastSpecies = -1;
    s32 lastVariant = -1;
#if FISHMOD_DEBUG
    OSTime profStart;
#endif

    if (gFishmodPhoto.active && (gFishmodPhoto.backdropFrame != play->gameplayFrames)) {
        gFishmodPhoto.backdropFrame = play->gameplayFrames;
        EnFishmod_DrawPhotoBackdrop(play);
    }
    if (!gFishmodPhoto.active) {
#if FISHMOD_DEBUG
        gFishmodScatterDrawn = 0;
        if (!(gFishmodBench & FISHMOD_BENCH_NOPROPS))
#endif
        {
            FISHMOD_PROF(FISHMOD_PROF_D_SCATTER, EnFishmod_ScatterDraw(play));
        }
#if FISHMOD_DEBUG
        if (gFishmodLodTest.active) {
            return; // the LOD calibration compares frames: nothing moving but the props
        }
#endif
        FISHMOD_PROF(FISHMOD_PROF_D_PARTICLES, EnFishmod_ParticlesDraw(play));
    }
#if FISHMOD_DEBUG
    if (gFishmodBench & FISHMOD_BENCH_NOFISH) {
        return; // the hardware bench, timing the rest without the fish
    }
#endif

    // which fish, and how
    for (actor = play->actorCtx.actorLists[ACTORCAT_NPC].head; actor != NULL; actor = actor->next) {
        EnFishmod* fish = (EnFishmod*)actor;
        f32 dist;
        s32 close;

        if ((actor->id != ACTOR_EN_FISHMOD) || (fish->type != FISHMOD_TYPE_FISH) || (actor->init != NULL) ||
            !(actor->flags & (ACTOR_FLAG_INSIDE_CULLING_VOLUME | ACTOR_FLAG_DRAW_CULLING_DISABLED)) ||
            !fish->rigReady) {
            continue;
        }
        dist = Math_Vec3f_DistXYZ(&actor->world.pos, &play->view.eye);
        close = gFishmodPhoto.active || (fish->fishState == FISH_STATE_CAUGHT);
        if (!close && (dist > FISHMOD_LOD_DIST)) {
            if (farCount < DRAW_MAX) {
                far[farCount++] = fish;
            }
        } else if (fullCount < DRAW_MAX) {
            ManagerDrawItem* item = &full[fullCount++];

            item->fish = fish;
            item->variant = (close || ((dist < LOD_FULL_DIST) && (fish->lengthInches >= FISHMOD_SMALL_RIG_LENGTH)))
                                ? FISH_RIG_FULL
                                : FISH_RIG_REDUCED;
            if (gFishmodStudio.active) {
                // the animation studio picks the rig per fish
                s32 k;

                for (k = 0; k < FISHMOD_STUDIO_MAX; k++) {
                    if ((gFishmodStudio.fish[k] == actor) && (gFishmodStudio.rig[k] != 0)) {
                        item->variant = gFishmodStudio.rig[k] - 1;
                    }
                }
            }
            item->whiskers = DRAW_WHISKERS && (fish->species->lookFlags & FISHMOD_LOOK_WHISKERS) &&
                             (item->variant == FISH_RIG_FULL) && (close || (dist < LOD_FULL_DIST));
        }
    }

    // sorted by species and rig, so each species' textures and vertices are set up once
    for (i = 1; i < fullCount; i++) {
        ManagerDrawItem item = full[i];
        s32 key = (item.fish->species - gFishmodSpecies) * 2 + item.variant;

        for (j = i - 1; j >= 0; j--) {
            if ((full[j].fish->species - gFishmodSpecies) * 2 + full[j].variant <= key) {
                break;
            }
            full[j + 1] = full[j];
        }
        full[j + 1] = item;
    }

    for (i = 0; i < fullCount; i++) {
        EnFishmod* fish = full[i].fish;
        s32 species = fish->species - gFishmodSpecies;

        if (species != lastSpecies) {
#if FISHMOD_DEBUG
            profStart = osGetTime();
#endif
            Gfx_SetupDL_25Opa(play->state.gfxCtx);
            EnFishmod_SetSpeciesTextures(fish, play);
#if FISHMOD_DEBUG
            gFishmodProf[FISHMOD_PROF_D_SETUP] += osGetTime() - profStart;
            gFishmodProfCount[FISHMOD_PROF_D_SETUP]++;
#endif
            lastSpecies = species;
            lastVariant = -1;
        }
        if (full[i].variant != lastVariant) {
            EnFishmod_RigSetSpecies(fish, play, full[i].variant);
            lastVariant = full[i].variant;
        }
        EnFishmod_RigDraw(fish, play, full[i].variant,
                          full[i].whiskers || gFishmodPhoto.active || (fish->fishState == FISH_STATE_CAUGHT));
    }
    if (fullCount != 0) {
        EnFishmod_RigRestoreSegments(play);
    }

    for (i = 0; i < fullCount; i++) {
        if (full[i].whiskers) {
            FISHMOD_PROF(FISHMOD_PROF_D_WHISKERS, EnFishmod_DrawWhiskersRigid(full[i].fish, play));
        }
        // the camera frames the fish (in the animation studio, the middle one of the row)
        if (gFishmodPhoto.active && (!gFishmodStudio.active || (&full[i].fish->actor == gFishmodStudio.fish[1]))) {
            EnFishmod_PhotoBounds(full[i].fish);
        }
    }

    // far shapes by species, so each species' pattern and palette are loaded once
    for (i = 1; i < farCount; i++) {
        EnFishmod* fish = far[i];

        for (j = i - 1; (j >= 0) && (far[j]->species > fish->species); j--) {
            far[j + 1] = far[j];
        }
        far[j + 1] = fish;
    }
    for (i = 0; i < farCount; i++) {
        if ((i == 0) || (far[i]->species != far[i - 1]->species)) {
            EnFishmod_BillboardMaterial(far[i], play);
        }
        FISHMOD_PROF(FISHMOD_PROF_D_BILLBOARD, EnFishmod_DrawBillboard(far[i], play));
    }
    if (farCount > 0) {
        OPEN_DISPS(play->state.gfxCtx, "../fishmod_manager.c", __LINE__);
        gSPTexture(POLY_OPA_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        CLOSE_DISPS(play->state.gfxCtx, "../fishmod_manager.c", __LINE__);
    }
}

