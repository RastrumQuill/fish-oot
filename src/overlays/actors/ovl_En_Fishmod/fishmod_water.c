/*
 * File: fishmod_water.c
 * Overlay: ovl_En_Fishmod
 * Description: a cache of the ground height under the scene's water boxes. Fish and the fishing line ask for the
 * floor many times a frame, and each collision raycast costs about half a millisecond on the N64, so the floor is
 * raycast once per grid corner (the first time it's needed) and read back from the grid after that.
 */

#include "z_en_fishmod.h"
#include "fishmod.h"

#include "bgcheck.h"
#include "play_state.h"
#include "segmented_address.h"
#include "zelda_arena.h"
#include "sys_math.h"
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
    f32 invCell; // 1 / cell
    s16 width; // corners
    s16 depth;
    s16* floor;
#if FISHMOD_OPT >= 5
    u8* open;          // per corner: how many cells to the nearest bank (see EnFishmod_OpenWaterDir)
    f32 openSurface;   // the water level it was worked out for
#endif
} FishmodWaterGrid;

// Exact floors near banks, cached by 4-unit spot
#if FISHMOD_OPT >= 4
#define EXACT_CACHE_SIZE 512 // 4 KB: bank raycasts cost about 0.6 ms each, so misses are expensive
#else
#define EXACT_CACHE_SIZE 64
#endif
typedef struct {
    s16 x;
    s16 z;
    s16 floor;
    s16 used;
} FishmodExactFloor;
static FishmodExactFloor sExactCache[EXACT_CACHE_SIZE];

static f32 EnFishmod_RaycastFloor(PlayState* play, f32 x, f32 y, f32 z);

static FishmodWaterGrid sGrids[GRID_MAX_BOXES];
#if FISHMOD_OPT >= 5
struct FishmodWaterGrid;
static void EnFishmod_OpenBuild(struct FishmodWaterGrid* grid);
#endif
static s32 sGridCount;

#if FISHMOD_OPT >= 5
#define CELL_MEMO_SIZE 128
typedef struct CellMemo {
    FishmodWaterGrid* grid;
    s16 ix;
    s16 iz;
    f32 surface;
    f32 floor;
} CellMemo;
static CellMemo sCellMemo[CELL_MEMO_SIZE];
static CellMemo* sMemoSlot;
#endif
#if FISHMOD_OPT >= 5
// the scene's baked map (fishmod/tools/gen_watermap.py): the grids point into it instead of having their own
static FishmodWaterMapHeader* sWaterMap;
#endif

#if FISHMOD_OPT >= 4
// the last conservative cell answer (see EnFishmod_WaterFloor); good for one frame
static FishmodWaterGrid* sMemoGrid;
static s32 sMemoIx;
static s32 sMemoIz;
static f32 sMemoFloor;
static u32 sMemoFrame;
static u32 sFrame;

/** Called once a frame (by the fish manager): the cell memo only lasts a frame (corners can still fill in). */
void EnFishmod_WaterNewFrame(void) {
    sFrame++;
}
#endif
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
#if FISHMOD_OPT >= 5
    bzero(sCellMemo, sizeof(sCellMemo));
#endif

#if FISHMOD_OPT >= 5
    // Baked at build time from the scene's collision: no raycasts for the corners at all.
    sWaterMap = Fishmod_LoadWaterMap(play->sceneId);
    if (sWaterMap != NULL) {
        s16* floors = (s16*)(sWaterMap + 1);

        for (i = 0; (i < sWaterMap->numBoxes) && (sGridCount < GRID_MAX_BOXES); i++) {
            FishmodWaterMapBox* m = &sWaterMap->boxes[i];
            s32 j;

            for (j = 0; j < numBoxes; j++) {
                WaterBox* box = &boxes[j];

                if ((box->xMin == m->xMin) && (box->zMin == m->zMin) && (box->xLength == m->xLength) &&
                    (box->zLength == m->zLength)) {
                    FishmodWaterGrid* grid = &sGrids[sGridCount++];

                    grid->box = box;
                    grid->x0 = box->xMin;
                    grid->z0 = box->zMin;
                    grid->cell = m->cell;
                    grid->invCell = 1.0f / m->cell;
                    grid->width = m->width;
                    grid->depth = m->depth;
                    grid->floor = floors + m->offset;
                    grid->open = NULL;
                    break;
                }
            }
        }
#if FISHMOD_DEBUG
        // check the bake against the game's own raycasts at a spread of corners (debug text line 10)
        {
            s32 checked = 0;
            s32 bad = 0;
            s32 worst = 0;
            s32 g;

            for (g = 0; g < sGridCount; g++) {
                FishmodWaterGrid* grid = &sGrids[g];
                s32 n = grid->width * grid->depth;
                s32 k;

                for (k = 7; k < n; k += n / 40 + 1) {
                    s32 ix = k % grid->width;
                    s32 iz = k / grid->width;
                    f32 floor = EnFishmod_RaycastFloor(play, grid->x0 + ix * grid->cell,
                                                       grid->box->ySurface + GRID_CAST_HEIGHT,
                                                       grid->z0 + iz * grid->cell);
                    s32 live = (floor <= BGCHECK_Y_MIN + 1.0f) ? GRID_NO_FLOOR : (s16)CLAMP(floor, -0x7FF0, 0x7FF0);
                    s32 diff = ABS(live - grid->floor[k]);

                    checked++;
                    if (diff > 2) {
                        bad++;
                        worst = MAX(worst, diff);
                    }
                }
            }
            Fishmod_DebugPrintf(10, "map %d/%d off, worst %d", bad, checked, worst);
        }
#endif
        // the open water map, now while the scene loads rather than in the middle of play
        for (i = 0; i < sGridCount; i++) {
            FishmodWaterGrid* grid = &sGrids[i];

            grid->open = ZeldaArena_Malloc(grid->width * grid->depth);
            if (grid->open != NULL) {
                EnFishmod_OpenBuild(grid);
            }
        }
        return;
    }
#endif

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
        grid->invCell = 1.0f / cell;
        grid->width = width;
        grid->depth = depth;
#if FISHMOD_OPT >= 5
        grid->open = NULL;
#endif
        cornersLeft -= width * depth;
        sGridCount++;
    }
}

void EnFishmod_WaterGridDestroy(PlayState* play) {
    s32 i;

#if FISHMOD_OPT >= 4
    sMemoGrid = NULL;
#endif
#if FISHMOD_OPT >= 5
    for (i = 0; i < sGridCount; i++) {
        if (sGrids[i].open != NULL) {
            ZeldaArena_Free(sGrids[i].open);
            sGrids[i].open = NULL;
        }
    }
    if (sWaterMap != NULL) {
        ZeldaArena_Free(sWaterMap);
        sWaterMap = NULL;
        sGridCount = 0;
        return;
    }
#endif
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

    f32 floor;

    pos.x = x;
    pos.y = y;
    pos.z = z;
    FISHMOD_PROF(FISHMOD_PROF_U_RAYCAST, floor = BgCheck_EntityRaycastDown1(&play->colCtx, &poly, &pos));
    return floor;
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
#if FISHMOD_DEBUG
static s32 EnFishmod_WaterFloorImpl(PlayState* play, f32 x, f32 z, f32* surface, f32* floor, s32 conservative);

// with a profile timer around it (debug builds only)
s32 EnFishmod_WaterFloor(PlayState* play, f32 x, f32 z, f32* surface, f32* floor, s32 conservative) {
    s32 found;

    FISHMOD_PROF(FISHMOD_PROF_U_WATER, found = EnFishmod_WaterFloorImpl(play, x, z, surface, floor, conservative));
    return found;
}
#else
#define EnFishmod_WaterFloorImpl EnFishmod_WaterFloor
#endif

#if FISHMOD_DEBUG
static
#endif
s32 EnFishmod_WaterFloorImpl(PlayState* play, f32 x, f32 z, f32* surface, f32* floor, s32 conservative) {
    WaterBox* waterBox;
    FishmodWaterGrid* grid = NULL;
    s32 i;

#if FISHMOD_OPT >= 5
    // The grids are the scene's water boxes in the scene's order, so the first one the point is in is the box the
    // engine's search would find, without its walk over every box.
    for (i = 0; i < sGridCount; i++) {
        WaterBox* box = sGrids[i].box;

        if ((x >= box->xMin) && (x <= box->xMin + box->xLength) && (z >= box->zMin) &&
            (z <= box->zMin + box->zLength)) {
            grid = &sGrids[i];
            *surface = box->ySurface;
            break;
        }
    }
    if (grid == NULL) {
        if (!BgCheck_GetWaterSurface(play, &play->colCtx, x, z, surface, &waterBox)) {
            return false;
        }
    }
#else
    if (!BgCheck_GetWaterSurface(play, &play->colCtx, x, z, surface, &waterBox)) {
        return false;
    }
    for (i = 0; i < sGridCount; i++) {
        if (sGrids[i].box == waterBox) {
            grid = &sGrids[i];
            break;
        }
    }
#endif

    if (grid != NULL) {
#if FISHMOD_OPT >= 4
        f32 gx = (x - grid->x0) * grid->invCell;
        f32 gz = (z - grid->z0) * grid->invCell;
#else
        f32 gx = (x - grid->x0) / grid->cell;
        f32 gz = (z - grid->z0) / grid->cell;
#endif
        s32 ix = CLAMP((s32)gx, 0, grid->width - 2);
        s32 iz = CLAMP((s32)gz, 0, grid->depth - 2);
        s32 c00;
        s32 c10;
        s32 c01;
        s32 c11;

#if FISHMOD_OPT >= 5
        // The conservative floor depends only on the cell (and the surface, for cells on a bank), and the corners
        // never change once known: keep recent answers by cell.
        if (conservative) {
            CellMemo* memo = &sCellMemo[((iz * grid->width + ix) * 7 + (grid - sGrids)) & (CELL_MEMO_SIZE - 1)];

            if ((memo->grid == grid) && (memo->ix == ix) && (memo->iz == iz) && (memo->surface == *surface)) {
                *floor = memo->floor;
                return true;
            }
            sMemoSlot = memo;
        }
#elif FISHMOD_OPT >= 4
        // The nose and the middle of a fish, and fish near each other, often ask about the same cell: its
        // conservative floor depends only on the cell (unless it straddles a bank), so the last answer is kept.
        if (conservative && (grid == sMemoGrid) && (ix == sMemoIx) && (iz == sMemoIz) && (sMemoFrame == sFrame)) {
            *floor = sMemoFloor;
            return true;
        }
#endif
        c00 = EnFishmod_GridCorner(play, grid, ix, iz);
        c10 = EnFishmod_GridCorner(play, grid, ix + 1, iz);
        c01 = EnFishmod_GridCorner(play, grid, ix, iz + 1);
        c11 = EnFishmod_GridCorner(play, grid, ix + 1, iz + 1);

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
#if FISHMOD_OPT >= 5
                sMemoSlot->grid = grid;
                sMemoSlot->ix = ix;
                sMemoSlot->iz = iz;
                sMemoSlot->surface = *surface;
                sMemoSlot->floor = *floor;
#endif
#if FISHMOD_OPT >= 4
                sMemoGrid = grid;
                sMemoIx = ix;
                sMemoIz = iz;
                sMemoFloor = *floor;
                sMemoFrame = sFrame;
#endif
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

#if FISHMOD_OPT >= 5
/*
 * Open water: for each grid corner, how many cells it is to the nearest bank (water too shallow for a fish, or no
 * floor), worked out once for the scene's water level with two passes over the grid (a chamfer distance). Its slope
 * points away from the banks: a fish heading into a bank turns that way with one lookup instead of trying headings
 * one after another.
 */
#define OPEN_MIN_DEPTH 12.0f
#define OPEN_MAX 250

static void EnFishmod_OpenBuild(FishmodWaterGrid* grid) {
    s32 w = grid->width;
    s32 d = grid->depth;
    f32 surface = grid->box->ySurface;
    u8* open = grid->open;
    s32 x;
    s32 z;

    // 0 on banks, "far" in water; then forward and backward passes (2 straight, 3 diagonal, halved at the end)
    for (z = 0; z < d; z++) {
        for (x = 0; x < w; x++) {
            s32 f = grid->floor[z * w + x];

            open[z * w + x] =
                ((f == FISHMOD_WATERMAP_NO_FLOOR) || (f == GRID_UNKNOWN) || (f > surface - OPEN_MIN_DEPTH)) ? 0
                                                                                                          : OPEN_MAX;
        }
    }
    for (z = 0; z < d; z++) {
        for (x = 0; x < w; x++) {
            s32 v = open[z * w + x];

            if (v == 0) {
                continue;
            }
            if (x > 0) {
                v = MIN(v, open[z * w + x - 1] + 2);
            }
            if (z > 0) {
                v = MIN(v, open[(z - 1) * w + x] + 2);
                if (x > 0) {
                    v = MIN(v, open[(z - 1) * w + x - 1] + 3);
                }
                if (x < w - 1) {
                    v = MIN(v, open[(z - 1) * w + x + 1] + 3);
                }
            }
            open[z * w + x] = MIN(v, OPEN_MAX);
        }
    }
    for (z = d - 1; z >= 0; z--) {
        for (x = w - 1; x >= 0; x--) {
            s32 v = open[z * w + x];

            if (v == 0) {
                continue;
            }
            if (x < w - 1) {
                v = MIN(v, open[z * w + x + 1] + 2);
            }
            if (z < d - 1) {
                v = MIN(v, open[(z + 1) * w + x] + 2);
                if (x < w - 1) {
                    v = MIN(v, open[(z + 1) * w + x + 1] + 3);
                }
                if (x > 0) {
                    v = MIN(v, open[(z + 1) * w + x - 1] + 3);
                }
            }
            open[z * w + x] = MIN(v, OPEN_MAX);
        }
    }
    grid->openSurface = surface;
}

/**
 * The way to open water from (x, z): away from the nearest banks, as a yaw. Returns false where it can't tell (no
 * grid there, or open water all around).
 */
s32 EnFishmod_OpenWaterDir(PlayState* play, f32 x, f32 z, s16* yaw) {
    s32 i;

    for (i = 0; i < sGridCount; i++) {
        FishmodWaterGrid* grid = &sGrids[i];
        WaterBox* box = grid->box;

        if ((x >= box->xMin) && (x <= box->xMin + box->xLength) && (z >= box->zMin) &&
            (z <= box->zMin + box->zLength)) {
            s32 ix;
            s32 iz;
            s32 w = grid->width;
            s32 gx;
            s32 gz;

            if (grid->open == NULL) {
                grid->open = ZeldaArena_Malloc(grid->width * grid->depth);
                if (grid->open == NULL) {
                    return false;
                }
                grid->openSurface = -32000.0f;
            }
            if (grid->openSurface != box->ySurface) {
                EnFishmod_OpenBuild(grid);
            }
            ix = CLAMP((s32)((x - grid->x0) * grid->invCell + 0.5f), 1, grid->width - 2);
            iz = CLAMP((s32)((z - grid->z0) * grid->invCell + 0.5f), 1, grid->depth - 2);
            gx = (s32)grid->open[iz * w + ix + 1] - grid->open[iz * w + ix - 1];
            gz = (s32)grid->open[(iz + 1) * w + ix] - grid->open[(iz - 1) * w + ix];
            if ((gx == 0) && (gz == 0)) {
                return false;
            }
            *yaw = Math_Atan2S(gz, gx);
            return true;
        }
    }
    return false;
}
#endif
