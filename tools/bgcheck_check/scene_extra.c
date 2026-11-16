/*
 * fish-oot: Lake Hylia's collision as the mod loads it, for bgcheck_check: the scene's own followed by its props'
 * (src/fishmod_data/fishmod_col_spot06.c, from gen_collision.py), joined as Fishmod_AddPropCollision (fishmod.c) does.
 */
#include "ultra64.h"
#include "bgcheck.h"
#include "scene.h"
#include "fishmod_collision.h"
#include "bgcheck_check.h"
#include "src/fishmod_data/fishmod_col_spot06.c"

// (the C library's, with the game's own headers in the way of its own)
void* calloc(unsigned int count, unsigned int size);
int printf(const char* fmt, ...);

void BgCheckCheck_PrepareScene(BgCheckCheckScene* scene) {
    CollisionHeader* colHeader = scene->col;
    FishmodColHeader* col = &gFishmodCol_spot06.header;
    Vec3s* extraVtx = gFishmodCol_spot06.vtx;
    FishmodColPoly* extraPoly = gFishmodCol_spot06.polys;
    Vec3s* vtx;
    CollisionPoly* poly;
    s32 n;

    if ((scene->sceneId != SCENE_LAKE_HYLIA) || (col->magic != FISHMOD_COL_MAGIC)) {
        return;
    }
    vtx = calloc(colHeader->numVertices + col->numVertices, sizeof(Vec3s));
    poly = calloc(colHeader->numPolygons + col->numPolygons, sizeof(CollisionPoly));
    bcopy(colHeader->vtxList, vtx, colHeader->numVertices * sizeof(Vec3s));
    bcopy(extraVtx, vtx + colHeader->numVertices, col->numVertices * sizeof(Vec3s));
    bcopy(colHeader->polyList, poly, colHeader->numPolygons * sizeof(CollisionPoly));
    for (n = 0; n < col->numPolygons; n++) {
        CollisionPoly* p = &poly[colHeader->numPolygons + n];
        FishmodColPoly* e = &extraPoly[n];

        p->type = e->type;
        p->vtxData[0] = COLPOLY_VTX(colHeader->numVertices + e->vtx[0], 0);
        p->vtxData[1] = COLPOLY_VTX(colHeader->numVertices + e->vtx[1], 0);
        p->vtxData[2] = colHeader->numVertices + e->vtx[2];
        p->normal = e->normal;
        p->dist = e->dist;
    }
    colHeader->vtxList = vtx;
    colHeader->polyList = poly;
    colHeader->numVertices += col->numVertices;
    colHeader->numPolygons += col->numPolygons;
    printf("%s: the props' %d polygons added (%d in all)\n", scene->name, col->numPolygons,
            colHeader->numPolygons);
}
