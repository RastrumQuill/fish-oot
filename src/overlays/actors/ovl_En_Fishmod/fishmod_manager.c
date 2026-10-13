/*
 * File: fishmod_manager.c
 * Overlay: ovl_En_Fishmod
 * Description: the fish manager (FISHMOD_OPT 4), type FISHMOD_TYPE_MANAGER: one per scene, spawned by the first
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

#if FISHMOD_OPT >= 4

#define ROSTER_WAKE_DIST 1400.0f  // a fish appears at its home when Link comes this close
#define ROSTER_SLEEP_DIST 1800.0f // and is removed when he's this far away (while it's just cruising)
#define ROSTER_CHECKS_PER_FRAME 48 // (FISHMOD_OPT 5) roster records looked at per frame
#define ROSTER_SPAWNS_PER_FRAME 2  // (FISHMOD_OPT 5) fish woken per frame
#define THINK_BUDGET 16           // full AI updates per frame for fish cruising far from Link and the bobber
#define THINK_FAR_DIST 300.0f
#if FISHMOD_OPT >= 5
#define LOD_FULL_DIST 300.0f      // closer than this: the full rig (and whiskers); then the reduced rig to the far shape
#else
#define LOD_FULL_DIST 200.0f      // closer than this: the full rig (and whiskers)
#endif
#define DRAW_WHISKERS 0           // catfish whiskers are off for now: too thin to see (to be redone)

typedef struct FishmodRosterFish {
    Vec3f home;
    f32 length;
    s16 yaw;
    u8 species;
    u8 caught;
    EnFishmod* fish;
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
#if FISHMOD_OPT >= 5
        // attention follows the camera, not Link (a NaN guard: a camera being set up can give one for a frame)
        if ((play->view.eye.x == play->view.eye.x) && (play->view.eye.z == play->view.eye.z) &&
            (fabsf(play->view.eye.x) < 30000.0f) && (fabsf(play->view.eye.z) < 30000.0f)) {
            sShared.camPos = play->view.eye;
        }
#endif
        sShared.rosterFocus = sShared.camPos;
        sShared.bobberIn = EnFishmod_RodBobberInWater();
        sShared.bobberPos = gFishmodRodState.bobberPos;
        sShared.thinksLeft = THINK_BUDGET;
#if FISHMOD_OPT >= 5
        sShared.light = Fishmod_Light(play);
#endif
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
        rec->fish = NULL;
    }
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
#if FISHMOD_OPT >= 5
    if (spawns->fish > 0) {
        count = MIN(spawns->fish, FISHMOD_ROSTER_MAX);
    }
#endif
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
#if FISHMOD_OPT >= 5
        // most fish live by cover (weed beds, rocks, reeds, pads), the rest out in the open water
        if ((Rand_ZeroOne() < 0.75f) && EnFishmod_ScatterRandomCover(&center)) {
            radius = 120.0f;
        }
#endif
        if (EnFishmod_FindWaterPoint(play, &center, radius, 25.0f, &rec->home, &surface, &floor)) {
            rec->species = s;
            rec->length = lo + (hi - lo) * f;
            rec->yaw = (s16)Rand_ZeroFloat(0xFFFF);
            rec->caught = Fishmod_RosterIsCaught(play->sceneId, gSaveContext.save.totalDays, sRosterCount);
            rec->fish = NULL;
            sRosterCount++;
        }
    }
    Rand_Seed((u32)osGetTime());
}

/** Spawns the fish whose homes Link is near and removes the ones he's left far behind. */
static void Manager_UpdateRoster(EnFishmod* this, PlayState* play) {
    EnFishmodShared* shared = EnFishmod_Shared(play);
    s32 i;
#if FISHMOD_OPT >= 5
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
#else
    for (i = 0; i < sRosterCount; i++) {
        FishmodRosterFish* rec = &sRoster[i];
        f32 dist = Math_Vec3f_DistXZ(&shared->rosterFocus, &rec->home);

        if (rec->caught) {
            continue;
        }
        if ((rec->fish == NULL) && (dist < ROSTER_WAKE_DIST)) {
#endif
#if FISHMOD_OPT >= 5
            // the record's index rides in the spawn roll (the params only have room for 31)
            Actor* fish = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, rec->home.x, rec->home.y, rec->home.z,
                                      0, rec->yaw, i + 1, FISHMOD_PARAMS(FISHMOD_TYPE_FISH, rec->species));
#else
            Actor* fish = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FISHMOD, rec->home.x, rec->home.y, rec->home.z,
                                      0, rec->yaw, 0, FISHMOD_PARAMS_ROSTER(rec->species, i));
#endif

            if (fish != NULL) {
                fish->room = -1;
                rec->fish = (EnFishmod*)fish;
            }
        } else if ((rec->fish != NULL) && (dist > ROSTER_SLEEP_DIST) &&
                   (rec->fish->fishState == FISH_STATE_WANDER) && (gFishmodRodState.fish != rec->fish)) {
            // it swims back home while nobody's looking
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
#if FISHMOD_OPT >= 5
    EnFishmod_ScatterLoad(play);
#endif
    Manager_BuildRoster(play);
}

void EnFishmod_ManagerDestroy(EnFishmod* this, PlayState* play) {
    s32 i;

    for (i = 0; i < sRosterCount; i++) {
        sRoster[i].fish = NULL;
    }
    sRosterCount = 0;
    EnFishmod_RigFreeAll();
#if FISHMOD_OPT >= 5
    EnFishmod_ScatterUnload();
#endif
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
}

/**
 * How far a fish is from attention: the camera (FISHMOD_OPT 5; Link before) or the bobber in the water, whichever is
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
#if FISHMOD_OPT >= 5
    if (!gFishmodPhoto.active) {
        FISHMOD_PROF(FISHMOD_PROF_D_SCATTER, EnFishmod_ScatterDraw(play));
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

#if FISHMOD_OPT >= 5
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
#else
    for (i = 0; i < farCount; i++) {
        FISHMOD_PROF(FISHMOD_PROF_D_BILLBOARD, EnFishmod_DrawBillboard(far[i], play));
    }
#endif
}

#endif
