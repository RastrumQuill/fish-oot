/*
 * fish-oot: what the mod's collision code (z_bgcheck.c) needs beyond the game's, for bgcheck_check: the scenes' sizes
 * for their prop collision, as Fishmod_SizeCollision (src/code/fishmod.c) gives them.
 */
#include "ultra64.h"
#include "array_count.h"
#include "scene.h"
#include "z_math.h"
#include "ultra64/gbi.h"

void abort(void);

typedef struct FishmodCollisionSize {
    s16 sceneId;
    u8 gridCells;
    u32 extraMem;
} FishmodCollisionSize;

static const FishmodCollisionSize sPropCollisionSizes[] = {
#define FISHMOD_COLLISION(sceneId, segment, extraMem, gridCells) { sceneId, gridCells, extraMem },
#include "fishmod_collision_scenes.h"
#undef FISHMOD_COLLISION
};

void Fishmod_SizeCollision(s16 sceneId, u32* memSize, Vec3i* subdivAmount) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sPropCollisionSizes); i++) {
        if (sPropCollisionSizes[i].sceneId == sceneId) {
            *memSize += sPropCollisionSizes[i].extraMem;
            if (sPropCollisionSizes[i].gridCells != 0) {
                subdivAmount->x = sPropCollisionSizes[i].gridCells;
                subdivAmount->z = sPropCollisionSizes[i].gridCells;
            }
            return;
        }
    }
}

// (z_skin_matrix.c's display-list conversion calls it; the collision code never does)
void Matrix_MtxFToMtx(MtxF* src, Mtx* dest) {
    abort();
}
