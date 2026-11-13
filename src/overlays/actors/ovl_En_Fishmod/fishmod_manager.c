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
    u8 look;          // its looks: the same fish every visit (from its slot's stream)
    u16 slot;         // which of the day's fish it is: what the caught list keys it by
    Vec3s feed;       // where it goes at its active times of day (its home, the cover, the rest of the day)
    Vec3s pos;        // while it isn't spawned: where it is, on its way to where its day says it should be
    u32 posFrame;     // the roster clock when `pos` was last moved on
} FishmodRosterFish;

#define ROSTER_TRAVEL 0.5f // units a frame an unspawned fish moves toward its place (a cruise with rests)

static FishmodRosterFish sRoster[FISHMOD_ROSTER_MAX];
static s32 sRosterCount;
static u32 sRosterClock; // manager updates so far: unspawned fish travel by it
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
        Fishmod_RosterSetCaught(play->sceneId, gSaveContext.save.totalDays, rec->slot);
    }
}

/** A fish is going away (removed by the manager, kept, or the scene is ending). */
void EnFishmod_RosterForget(EnFishmod* fish) {
    FishmodRosterFish* rec = Manager_Record(fish);

    if ((rec != NULL) && (rec->fish == fish)) {
        rec->mood = fish->mood;
        rec->pos.x = fish->actor.world.pos.x;
        rec->pos.y = fish->actor.world.pos.y;
        rec->pos.z = fish->actor.world.pos.z;
        rec->posFrame = sRosterClock;
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
/** The seed of a roster slot's stream: the scene, the in-game day and the slot. */
static u32 Roster_Seed(PlayState* play, s32 slot) {
    return (u32)play->sceneId * 7919 + (u32)gSaveContext.save.totalDays * 104729 + (u32)slot * 2654435761u + 17;
}

static void Manager_PlanDays(PlayState* play, FishmodRng* rng);
static void Manager_FindSchools(void);
static s32 sSchoolCount;

static void Manager_BuildRoster(PlayState* play) {
    FishmodRng rng;
    const FishmodSceneSpawns* spawns = Fishmod_GetSceneSpawns(play->sceneId);
    WaterBox* boxes;
    s32 numBoxes = play->colCtx.colHeader->numWaterBoxes;
    s32 totalWeight = 0;
    s32 count;
    s32 i;
    s32 n;

    sRosterCount = 0;
    sSchoolCount = 0; // (none until this roster's are found: a scene without fish keeps none)
    if ((spawns == NULL) || (numBoxes == 0) || FISHMOD_PHOTO_ON) {
        return;
    }
    boxes = SEGMENTED_TO_VIRTUAL(play->colCtx.colHeader->waterBoxes);
    for (i = 0; i < spawns->count; i++) {
        totalWeight += spawns->entries[i].weight;
    }

    count = FISHMOD_FISH_COUNT;
    if (spawns->fish > 0) {
        count = MIN(spawns->fish, FISHMOD_ROSTER_MAX);
    }
#if FISHMOD_DEBUG
    if (Fishmod_DebugRosterCount() > 0) {
        count = MIN(Fishmod_DebugRosterCount(), FISHMOD_ROSTER_MAX);
    }
#endif
    // each slot's fish from a stream of its own (the scene, the day, the slot): the same fish every visit that day,
    // whatever happens to the others
    gFishmodRngNow = &rng;
    for (n = 0; n < count; n++) {
        WaterBox* box;
        FishmodRosterFish* rec = &sRoster[sRosterCount];
        const FishmodSpecies* species;
        Vec3f center;
        f32 radius;
        f32 surface;
        f32 floor;
        s32 roll;
        s32 s = spawns->entries[0].species;
        f32 lo;
        f32 hi;
        f32 mode;
        f32 u;
        f32 f;

        Fishmod_RandSeed(&rng, Roster_Seed(play, n));
        box = &boxes[(s32)Fishmod_RandZeroFloat(gFishmodRngNow, numBoxes - 0.01f)];
        roll = (s32)Fishmod_RandZeroFloat(gFishmodRngNow, totalWeight - 0.01f);
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
        u = Fishmod_RandZeroOne(gFishmodRngNow);
        f = (u < mode) ? sqrtf(u * mode) : 1.0f - sqrtf((1.0f - u) * (1.0f - mode));

        center.x = box->xMin + box->xLength * 0.5f;
        center.y = box->ySurface;
        center.z = box->zMin + box->zLength * 0.5f;
        radius = CLAMP_MIN(box->xLength, box->zLength) * 0.6f;
        // most fish live by cover (weed beds, rocks, reeds, pads), the rest out in the open water
        // (a few tries for cover that isn't crowded with homes already; else the open water)
        if (Fishmod_RandZeroOne(gFishmodRngNow) < 0.75f) {
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
            rec->slot = n;
            rec->length = lo + (hi - lo) * f;
            rec->yaw = (s16)Fishmod_RandZeroFloat(gFishmodRngNow, 0xFFFF);
            rec->caught = Fishmod_RosterIsCaught(play->sceneId, gSaveContext.save.totalDays, n);
            rec->fish = NULL;
            rec->look = (u8)Fishmod_RandZeroFloat(gFishmodRngNow, 256.0f);
            // some hungry, some not (from the index, not the shared random numbers)
            rec->mood.hunger = 40 + ((sRosterCount * 97 + play->sceneId * 31) % 170);
            rec->mood.fear = 0;
            rec->mood.shy = 0;
            rec->leader = sRosterCount;
            sRosterCount++;
            // schooling fish: the rest of its school around it, about its size (they count toward the total)
            if (species->schoolMax > 0) {
                s32 leader = sRosterCount - 1;
                s32 more = species->schoolMin - 1 +
                           (s32)Fishmod_RandZeroFloat(gFishmodRngNow, species->schoolMax - species->schoolMin + 0.99f);

                while ((more-- > 0) && (n + 1 < count) && (sRosterCount < FISHMOD_ROSTER_MAX)) {
                    FishmodRosterFish* mate = &sRoster[sRosterCount];

                    *mate = sRoster[leader];
                    Fishmod_RandSeed(&rng, Roster_Seed(play, n + 1)); // (its own slot's stream)
                    if (!EnFishmod_FindWaterPoint(play, &sRoster[leader].home, 30.0f, 15.0f, &mate->home, &surface,
                                                  &floor)) {
                        continue;
                    }
                    mate->length = CLAMP(sRoster[leader].length * (0.8f + Fishmod_RandZeroOne(gFishmodRngNow) * 0.4f), lo, hi);
                    mate->slot = n + 1;
                    mate->caught = Fishmod_RosterIsCaught(play->sceneId, gSaveContext.save.totalDays, n + 1);
                    mate->look = (u8)Fishmod_RandZeroFloat(gFishmodRngNow, 256.0f);
                    mate->mood.hunger = 40 + ((sRosterCount * 97 + play->sceneId * 31) % 170);
                    mate->leader = leader;
                    sRosterCount++;
                    n++;
                }
            }
        }
    }
    Manager_PlanDays(play, &rng);
    Manager_FindSchools();
    gFishmodRngNow = &gFishmodRng;
}

/** The time of day, in the four parts the species' rhythms use. */
static s32 Manager_TimeOfDay(void) {
    s32 time = gSaveContext.save.dayTime;

    return ((time >= CLOCK_TIME(4, 0)) && (time < CLOCK_TIME(8, 0)))    ? FISHMOD_TIME_DAWN
           : ((time >= CLOCK_TIME(8, 0)) && (time < CLOCK_TIME(17, 0))) ? FISHMOD_TIME_DAY
           : ((time >= CLOCK_TIME(17, 0)) && (time < CLOCK_TIME(20, 0))) ? FISHMOD_TIME_DUSK
                                                                         : FISHMOD_TIME_NIGHT;
}

/**
 * Where a fish's day says it should be now: its feeding place at the species' active times (2 or 3 of 3), its home
 * by cover otherwise. A school goes together: its members keep their places around the first fish's (its leader at
 * the start of the day).
 */
static void Roster_Place(FishmodRosterFish* rec, Vec3f* out) {
    FishmodRosterFish* root = &sRoster[rec->leader];

    *out = rec->home;
    if (FISHMOD_ACTIVITY(&gFishmodSpecies[rec->species], Manager_TimeOfDay()) >= 2) {
        out->x += root->feed.x - root->home.x;
        out->y += root->feed.y - root->home.y;
        out->z += root->feed.z - root->home.z;
    }
}

/**
 * An unspawned fish carries on with its day: from where it was (where Link last saw it, or where it was going) on
 * toward its place, at a cruising pace. So it appears where it got to, not back at home; and one chased off and left
 * is still on its way when he comes back. Only arithmetic: the water is looked at once, when it's about to appear
 * (Roster_SpawnSpot; looking every pass cost raycasts along the shores).
 */
static void Roster_Travel(PlayState* play, FishmodRosterFish* rec) {
    Vec3f place;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 dist;
    f32 step = (sRosterClock - rec->posFrame) * ROSTER_TRAVEL;

    rec->posFrame = sRosterClock;
    Roster_Place(rec, &place);
    dx = place.x - rec->pos.x;
    dy = place.y - rec->pos.y;
    dz = place.z - rec->pos.z;
    dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
    if (dist <= step) {
        rec->pos.x = place.x;
        rec->pos.y = place.y;
        rec->pos.z = place.z;
        return;
    }
    if (step <= 0.0f) {
        return;
    }
    rec->pos.x += (s16)(dx * step / dist);
    rec->pos.y += (s16)(dy * step / dist);
    rec->pos.z += (s16)(dz * step / dist);
}

/** Where an unspawned fish appears: where it got to, if that's water deep enough; else at its place. */
static void Roster_SpawnSpot(PlayState* play, FishmodRosterFish* rec) {
    f32 surface;
    f32 floor;

    if (!EnFishmod_WaterFloor(play, rec->pos.x, rec->pos.z, &surface, &floor, true) || (surface - floor < 20.0f)) {
        Vec3f place;

        Roster_Place(rec, &place);
        rec->pos.x = place.x;
        rec->pos.y = place.y;
        rec->pos.z = place.z;
    } else {
        rec->pos.y = CLAMP(rec->pos.y, floor + 8.0f, surface - 8.0f);
    }
}

/**
 * Each school's leader this frame: the first of its fish that's spawned and free (cruising, or bolting with the
 * rest). A fish that's off at the bobber, hooked or caught isn't with its school; the next one leads the rest, and
 * it rejoins when it's free again. So a school is never caught whole: only the fish that took the bait.
 */
#define SCHOOLS_MAX 128
static u16 sSchoolFirst[SCHOOLS_MAX]; // each school's records: sSchoolFirst[n] up to sSchoolEnd[n] (made with the roster)
static u16 sSchoolEnd[SCHOOLS_MAX];

/** The schools' record ranges, once the roster is made (a school's records are together, its first one leading). */
static void Manager_FindSchools(void) {
    s32 i = 0;

    sSchoolCount = 0;
    while (i < sRosterCount) {
        s32 end = i + 1;

        while ((end < sRosterCount) && (sRoster[end].leader == i)) {
            end++;
        }
        if ((end - i > 1) && (sSchoolCount < SCHOOLS_MAX)) {
            sSchoolFirst[sSchoolCount] = i;
            sSchoolEnd[sSchoolCount] = end;
            sSchoolCount++;
        }
        i = end;
    }
}

static void Manager_UpdateSchools(void) {
    s32 n;

    // (frames12: from the schools' ranges, not a walk of the whole roster every frame (300 records, the data cache's
    // size and more: the manager's update was 70% waiting on memory), and every 4th frame: a school's lead changes
    // hands a few frames later at most)
    if ((sRosterClock & 3) != 0) {
        return;
    }
    for (n = 0; n < sSchoolCount; n++) {
        s32 i = sSchoolFirst[n];
        s32 end = sSchoolEnd[n];
        s32 lead = -1;
        s32 k;

        {
            for (k = i; k < end; k++) {
                EnFishmod* fish = sRoster[k].fish;

                if ((fish != NULL) && !sRoster[k].caught &&
                    ((fish->fishState == FISH_STATE_WANDER) || (fish->fishState == FISH_STATE_FLEE))) {
                    lead = k;
                    break;
                }
            }
            for (k = i; k < end; k++) {
                if (sRoster[k].fish != NULL) {
                    sRoster[k].fish->leaderIndex = ((lead < 0) || (k == lead)) ? FISHMOD_NO_ROSTER : lead;
                }
            }
        }
    }
}

/**
 * Each fish's day (from a stream of its own, after everything else of its slot's, so the day's fish are the ones they
 * were): a feeding place, other cover a little way off (or open water near home if there's none), where it goes at
 * its active times. And where each is right now: where its day says (arriving at the lake, the fish are already where
 * they'd be).
 */
static void Manager_PlanDays(PlayState* play, FishmodRng* rng) {
    s32 i;

    for (i = 0; i < sRosterCount; i++) {
        FishmodRosterFish* rec = &sRoster[i];
        Vec3f place;

        if (rec->leader == i) {
            Vec3f cover;
            Vec3f point;
            f32 surface;
            f32 floor;
            s32 tries;
            s32 found = false;

            Fishmod_RandSeed(rng, Roster_Seed(play, rec->slot) ^ 0x5EEDF00Du);
            for (tries = 0; (tries < 6) && !found; tries++) {
                f32 dSq;

                if (!EnFishmod_ScatterRandomCover(&cover)) {
                    break;
                }
                dSq = SQ(cover.x - rec->home.x) + SQ(cover.z - rec->home.z);
                if ((dSq > SQ(150.0f)) && (dSq < SQ(900.0f)) &&
                    EnFishmod_FindWaterPoint(play, &cover, 120.0f, 25.0f, &point, &surface, &floor)) {
                    found = true;
                }
            }
            if (!found && !EnFishmod_FindWaterPoint(play, &rec->home, 400.0f, 25.0f, &point, &surface, &floor)) {
                point = rec->home;
            }
            rec->feed.x = point.x;
            rec->feed.y = point.y;
            rec->feed.z = point.z;
        }
        Roster_Place(rec, &place);
        rec->pos.x = place.x;
        rec->pos.y = place.y;
        rec->pos.z = place.z;
        rec->posFrame = sRosterClock;
    }
}

/**
 * What the fish round about should know, told to them by the manager instead of each fish looking for it every time
 * it thinks (fishmod/docs: the stimuli broadcast):
 * - Link wading or swimming: every 8 frames, to the fish within reach (each one decides by its own nerves).
 * - Natural bait soaking (larvae, bugs, worms, fish): every 20 frames, its scent to the fish within a range that grows
 *   the longer it's in (to about 140 units after a minute), above or below it too; they come to it, seen or not.
 * The bobber's plop and a hooked fish's thrashing are told by EnFishmod_Disturb.
 */
static void Manager_Broadcast(PlayState* play) {
    static u16 sSoak;
    EnFishmodShared* shared = EnFishmod_Shared(play);
    Player* player = GET_PLAYER(play);
    u8 bait = gFishmodRodState.bait;
    s32 i;

    if (shared->bobberIn && ((bait == FISHMOD_BAIT_BEE_LARVA) || (bait == FISHMOD_BAIT_BUG) ||
                             (bait == FISHMOD_BAIT_WORM) || (bait == FISHMOD_BAIT_FISH))) {
        sSoak = CLAMP_MAX(sSoak + 1, 1200);
    } else {
        sSoak = 0;
    }
    if (((play->gameplayFrames & 7) == 0) && (player->actor.depthInWater > 0.0f)) {
        for (i = 0; i < sRosterCount; i++) {
            EnFishmod* fish = sRoster[i].fish;

            if ((fish != NULL) && (SQ(fish->actor.world.pos.x - shared->linkPos.x) +
                                   SQ(fish->actor.world.pos.z - shared->linkPos.z)) < SQ(400.0f)) {
                EnFishmod_HearLink(fish, play, &shared->linkPos, player->actor.speed);
            }
        }
    }
    if ((sSoak > 0) && ((play->gameplayFrames % 20) == 0)) {
        f32 radius = 20.0f + CLAMP_MAX(sSoak * 0.1f, 120.0f);
        Vec3f baitPos = gFishmodRodState.bobberPos;

        baitPos.y = FISHMOD_ROD_BAIT_Y(&gFishmodRodState); // (where the bait hangs, under the float)

        for (i = 0; i < sRosterCount; i++) {
            EnFishmod* fish = sRoster[i].fish;

            if ((fish != NULL) &&
                (Math_Vec3f_DistXYZ(&fish->actor.world.pos, &baitPos) < radius)) {
                fish->scentFrame = play->gameplayFrames;
            }
        }
    }
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
    s32 tod = Manager_TimeOfDay();

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
        Vec3f at;
        f32 d;

        if (rec->fish != NULL) {
            at = rec->fish->actor.world.pos;
        } else {
            at.x = rec->pos.x;
            at.y = rec->pos.y;
            at.z = rec->pos.z;
        }
        d = Math_Vec3f_DistXYZ(&at, pos);

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
#if FISHMOD_DEBUG
    s32 awake = 0; // (the fish stress bench caps the fish in the water at its step's count)

    if (gFishmodStress == 1) {
        for (n = 0; n < sRosterCount; n++) {
            if (sRoster[n].fish != NULL) {
                if (awake >= gFishmodStressCount) {
                    // one too many after a pass starts over: it goes home
                    sRoster[n].mood = sRoster[n].fish->mood;
                    Actor_Kill(&sRoster[n].fish->actor);
                    sRoster[n].fish = NULL;
                } else {
                    awake++;
                }
            }
        }
        spawns = MIN(spawns, gFishmodStressCount - awake);
    }
#endif

    for (n = 0; n < checks; n++) {
        FishmodRosterFish* rec;
        f32 dist;

        i = sNext;
        sNext = (sNext + 1 < sRosterCount) ? sNext + 1 : 0;
        rec = &sRoster[i];
        if (rec->caught) {
            continue;
        }
        if (rec->fish == NULL) {
            Vec3f at;

            Roster_Travel(play, rec);
            at.x = rec->pos.x;
            at.y = rec->pos.y;
            at.z = rec->pos.z;
            dist = Math_Vec3f_DistXZ(&shared->rosterFocus, &at);
        } else {
            Vec3f place;

            // where it's headed today: its wandering stays around that
            Roster_Place(rec, &place);
            rec->fish->actor.home.pos = place;
            dist = Math_Vec3f_DistXZ(&shared->rosterFocus, &rec->fish->actor.world.pos);
        }
        if ((rec->fish == NULL) && (dist < ROSTER_WAKE_DIST) && (spawns-- > 0)) {
            // the record's index rides in the spawn roll (the params only have room for 31)
            Actor* fish;

            Roster_SpawnSpot(play, rec);
            fish = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, rec->pos.x, rec->pos.y, rec->pos.z,
                                      0, rec->yaw, i + 1, FISHMOD_PARAMS(FISHMOD_TYPE_FISH, rec->species));

            if (fish != NULL) {
                fish->room = -1;
                rec->fish = (EnFishmod*)fish;
                rec->fish->look = rec->look;
                rec->fish->mood = rec->mood;
                rec->fish->leaderIndex = (rec->leader != i) ? rec->leader : FISHMOD_NO_ROSTER;
                Roster_Place(rec, &rec->fish->actor.home.pos);
            }
        } else if ((rec->fish != NULL) && (dist > ROSTER_SLEEP_DIST) &&
                   (rec->fish->fishState == FISH_STATE_WANDER) && (gFishmodRodState.fish != rec->fish)) {
            // it carries on with its day from here while nobody's looking (Roster_Travel)
            rec->mood = rec->fish->mood;
            rec->pos.x = rec->fish->actor.world.pos.x;
            rec->pos.y = rec->fish->actor.world.pos.y;
            rec->pos.z = rec->fish->actor.world.pos.z;
            rec->posFrame = sRosterClock;
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
    Actor_ChangeCategory(play, &play->actorCtx, &this->actor, ACTORCAT_FISHMGR);
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
    sSchoolCount = 0;
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
    sRosterClock++;
    Manager_UpdateSchools();
    Manager_Broadcast(play);
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

    if ((this->fishState != FISH_STATE_WANDER) || FISHMOD_PHOTO_ON ||
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

    if (FISHMOD_PHOTO_ON && (gFishmodPhoto.backdropFrame != play->gameplayFrames)) {
        gFishmodPhoto.backdropFrame = play->gameplayFrames;
        EnFishmod_DrawPhotoBackdrop(play);
    }
    if (!FISHMOD_PHOTO_ON) {
#if FISHMOD_DEBUG
        gFishmodScatterDrawn = 0;
        if (!(gFishmodBench & FISHMOD_BENCH_NOPROPS))
#endif
        {
            FISHMOD_PROF(FISHMOD_PROF_D_SCATTER, EnFishmod_ScatterDraw(play));
        }
#if FISHMOD_DEBUG
        if (gFishmodStress == 2) {
            FISHMOD_PROF(FISHMOD_PROF_D_SCATTER, EnFishmod_StressPlantsDraw(play, gFishmodStressCount));
        }
#endif
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
    for (actor = play->actorCtx.actorLists[ACTORCAT_FISH].head; actor != NULL; actor = actor->next) {
        EnFishmod* fish = (EnFishmod*)actor;
        f32 dist;
        s32 close;

        if ((actor->id != ACTOR_EN_FISHMOD) || (fish->type != FISHMOD_TYPE_FISH) || (actor->init != NULL) ||
            !(actor->flags & (ACTOR_FLAG_INSIDE_CULLING_VOLUME | ACTOR_FLAG_DRAW_CULLING_DISABLED)) ||
            !fish->rigReady) {
            continue;
        }
        dist = Math_Vec3f_DistXYZ(&actor->world.pos, &play->view.eye);
        close = FISHMOD_PHOTO_ON || (fish->fishState == FISH_STATE_CAUGHT);
        if (!close && (dist > FISHMOD_LOD_DIST)) {
            if (farCount < DRAW_MAX) {
                far[farCount++] = fish;
            }
        } else if (fullCount < DRAW_MAX) {
            ManagerDrawItem* item = &full[fullCount++];

            item->fish = fish;
            fish->drawnFull = 3; // (its swim wave keeps running while this lasts)
            item->variant = (close || ((dist < LOD_FULL_DIST) && (fish->lengthInches >= FISHMOD_SMALL_RIG_LENGTH)))
                                ? FISH_RIG_FULL
                                : FISH_RIG_REDUCED;
            if (FISHMOD_STUDIO_ON) {
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

        Fishmod_ClaimDisplayLists(play->state.gfxCtx); // (the lists' next lines, before this fish is written)

        if (species != lastSpecies) {
#if FISHMOD_DEBUG
            profStart = FISHMOD_CLOCK();
#endif
            Gfx_SetupDL_25Opa(play->state.gfxCtx);
            EnFishmod_SetSpeciesTextures(fish, play);
#if FISHMOD_DEBUG
            gFishmodProf[FISHMOD_PROF_D_SETUP] += FISHMOD_SINCE(profStart);
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
                          full[i].whiskers || FISHMOD_PHOTO_ON || (fish->fishState == FISH_STATE_CAUGHT));
    }
    if (fullCount != 0) {
        EnFishmod_RigRestoreSegments(play);
    }

    for (i = 0; i < fullCount; i++) {
        if (full[i].whiskers) {
            FISHMOD_PROF(FISHMOD_PROF_D_WHISKERS, EnFishmod_DrawWhiskersRigid(full[i].fish, play));
        }
        // the camera frames the fish (in the animation studio, the middle one of the row)
        if (FISHMOD_PHOTO_ON && (!FISHMOD_STUDIO_ON || (&full[i].fish->actor == gFishmodStudio.fish[1]))) {
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

