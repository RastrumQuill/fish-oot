/*
 * File: fishmod_water.c
 * Overlay: ovl_En_Fishmod
 * Description: a cache of the ground height under the scene's water boxes. Fish and the fishing line ask for the
 * floor many times a frame, and each collision raycast costs about half a millisecond on the N64, so the floor is
 * raycast once per grid corner (the first time it's needed) and read back from the grid after that.
 */

#include "z_en_fishmod.h"

#include "bgcheck.h"
#include "play_state.h"
#include "segmented_address.h"
#include "zelda_arena.h"
#include "ultra64.h"
#include "ultra64/libc.h"

#define GRID_MAX_BOXES 4
#define GRID_MAX_CORNERS 12000 // all boxes together; 2 bytes each
#define GRID_CELL_MIN 16.0f
// Floors are found by casting down from this far above the water: high enough to find the top of the banks,
// low enough to stay under most bridges.
#define GRID_CAST_HEIGHT 60.0f

#define GRID_UNKNOWN 0x7FFF  // not raycast yet
#define GRID_NO_FLOOR -0x8000 // the raycast found nothing

typedef struct FishmodWaterGrid {
    WaterBox* box;
    f32 x0;
    f32 z0;
    f32 cell;
    s16 width; // corners
    s16 depth;
    s16* floor;
} FishmodWaterGrid;

// Exact floors near banks, cached by 4-unit spot
#define EXACT_CACHE_SIZE 64
typedef struct {
    s16 x;
    s16 z;
    s16 floor;
    s16 used;
} FishmodExactFloor;
static FishmodExactFloor sExactCache[EXACT_CACHE_SIZE];

static FishmodWaterGrid sGrids[GRID_MAX_BOXES];
static s32 sGridCount;
static s32 sGridUsers; // the rod and the fish share one cache: built by the first, freed after the last

void EnFishmod_WaterGridAcquire(PlayState* play) {
    if (sGridUsers++ == 0) {
        EnFishmod_WaterGridInit(play);
    }
}

void EnFishmod_WaterGridRelease(PlayState* play) {
    if ((sGridUsers > 0) && (--sGridUsers == 0)) {
        EnFishmod_WaterGridDestroy(play);
    }
}

void EnFishmod_WaterGridInit(PlayState* play) {
    WaterBox* boxes = SEGMENTED_TO_VIRTUAL(play->colCtx.colHeader->waterBoxes);
    s32 numBoxes = play->colCtx.colHeader->numWaterBoxes;
    s32 cornersLeft = GRID_MAX_CORNERS;
    s32 i;

#if FISHMOD_OPT >= 3
    // with the Expansion Pak there's room for a finer cache over bigger water (Lake Hylia)
    if (osMemSize >= 0x800000) {
        cornersLeft *= 4;
    }
#endif

    EnFishmod_WaterGridDestroy(play);
    bzero(sExactCache, sizeof(sExactCache));

    for (i = 0; (i < numBoxes) && (sGridCount < GRID_MAX_BOXES); i++) {
        FishmodWaterGrid* grid = &sGrids[sGridCount];
        WaterBox* box = &boxes[i];
        f32 cell = GRID_CELL_MIN;
        s32 width;
        s32 depth;
        s32 j;

        // coarser cells for big boxes, to fit the budget
        while (true) {
            width = (s32)(box->xLength / cell) + 2;
            depth = (s32)(box->zLength / cell) + 2;
            if ((width * depth <= cornersLeft) || (cell >= 64.0f)) {
                break;
            }
            cell *= 2.0f;
        }
        if (width * depth > cornersLeft) {
            continue;
        }
        grid->floor = ZeldaArena_Malloc(width * depth * sizeof(s16));
        if (grid->floor == NULL) {
            break;
        }
        for (j = 0; j < width * depth; j++) {
            grid->floor[j] = GRID_UNKNOWN;
        }
        grid->box = box;
        grid->x0 = box->xMin;
        grid->z0 = box->zMin;
        grid->cell = cell;
        grid->width = width;
        grid->depth = depth;
        cornersLeft -= width * depth;
        sGridCount++;
    }
}

void EnFishmod_WaterGridDestroy(PlayState* play) {
    s32 i;

    for (i = 0; i < sGridCount; i++) {
        if (sGrids[i].floor != NULL) {
            ZeldaArena_Free(sGrids[i].floor);
            sGrids[i].floor = NULL;
        }
    }
    sGridCount = 0;
}

static f32 EnFishmod_RaycastFloor(PlayState* play, f32 x, f32 y, f32 z) {
    CollisionPoly* poly;
    Vec3f pos;

    pos.x = x;
    pos.y = y;
    pos.z = z;
    return BgCheck_EntityRaycastDown1(&play->colCtx, &poly, &pos);
}

/** The floor at one grid corner, raycasting it the first time. */
static s32 EnFishmod_GridCorner(PlayState* play, FishmodWaterGrid* grid, s32 ix, s32 iz) {
    s16* corner = &grid->floor[iz * grid->width + ix];

    if (*corner == GRID_UNKNOWN) {
        f32 floor = EnFishmod_RaycastFloor(play, grid->x0 + ix * grid->cell, grid->box->ySurface + GRID_CAST_HEIGHT,
                                           grid->z0 + iz * grid->cell);

        *corner = (floor <= BGCHECK_Y_MIN + 1.0f) ? GRID_NO_FLOOR : (s16)CLAMP(floor, -0x7FF0, 0x7FF0);
    }
    return *corner;
}


static f32 EnFishmod_ExactFloor(PlayState* play, f32 x, f32 z, f32 surface) {
    s16 kx = (s16)((s32)x >> 2);
    s16 kz = (s16)((s32)z >> 2);
    FishmodExactFloor* e = &sExactCache[((kx * 31) ^ kz) & (EXACT_CACHE_SIZE - 1)];

    if (!e->used || (e->x != kx) || (e->z != kz)) {
        f32 floor = EnFishmod_RaycastFloor(play, x, surface + GRID_CAST_HEIGHT, z);

        e->x = kx;
        e->z = kz;
        e->floor = (s16)CLAMP(floor, -0x7FF0, 0x7FF0);
        e->used = true;
    }
    return e->floor;
}

/**
 * Water surface and floor at (x, z). Returns false if there's no water there (the floor is then not set).
 * `conservative` gives the highest of the surrounding grid corners instead of interpolating them: the floor is then
 * never lower than the real one, which keeps fish from swimming into the bed or a bank.
 */
s32 EnFishmod_WaterFloor(PlayState* play, f32 x, f32 z, f32* surface, f32* floor, s32 conservative) {
    WaterBox* waterBox;
    FishmodWaterGrid* grid = NULL;
    s32 i;

    if (!BgCheck_GetWaterSurface(play, &play->colCtx, x, z, surface, &waterBox)) {
        return false;
    }
    for (i = 0; i < sGridCount; i++) {
        if (sGrids[i].box == waterBox) {
            grid = &sGrids[i];
            break;
        }
    }

    if (grid != NULL) {
        f32 gx = (x - grid->x0) / grid->cell;
        f32 gz = (z - grid->z0) / grid->cell;
        s32 ix = CLAMP((s32)gx, 0, grid->width - 2);
        s32 iz = CLAMP((s32)gz, 0, grid->depth - 2);
        s32 c00 = EnFishmod_GridCorner(play, grid, ix, iz);
        s32 c10 = EnFishmod_GridCorner(play, grid, ix + 1, iz);
        s32 c01 = EnFishmod_GridCorner(play, grid, ix, iz + 1);
        s32 c11 = EnFishmod_GridCorner(play, grid, ix + 1, iz + 1);

        if ((c00 != GRID_NO_FLOOR) && (c10 != GRID_NO_FLOOR) && (c01 != GRID_NO_FLOOR) && (c11 != GRID_NO_FLOOR)) {
            s32 hi = MAX(MAX(c00, c10), MAX(c01, c11));
            s32 lo = MIN(MIN(c00, c10), MIN(c01, c11));

            if (conservative && (hi > *surface) && (lo < *surface)) {
                // The cell straddles a bank (some corners under water, some on top of it), so neither its highest
                // corner nor a blend is right next to a steep wall: measure the spot itself, cached nearby.
                *floor = EnFishmod_ExactFloor(play, x, z, *surface);
                return true;
            }
            if (conservative) {
                *floor = MAX(MAX(c00, c10), MAX(c01, c11));
            } else {
                f32 fx = CLAMP(gx - ix, 0.0f, 1.0f);
                f32 fz = CLAMP(gz - iz, 0.0f, 1.0f);

                *floor = (c00 + (c10 - c00) * fx) * (1.0f - fz) + (c01 + (c11 - c01) * fx) * fz;
            }
            return true;
        }
    }

    // not cached (no grid for this box, or an edge with no floor): raycast it
    *floor = EnFishmod_RaycastFloor(play, x, *surface + GRID_CAST_HEIGHT, z);
    return true;
}
