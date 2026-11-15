/*
 * bgcheck_check: runs two builds of the collision code (each a shared library: z_bgcheck.c and sys_math3d.c with the
 * game files they use, see run.sh) on every scene of a version, with moving platforms, and checks that every answer is
 * the same, bit for bit: each query's return value, its out positions, the poly it found (by index), the bgId.
 *
 *   driver LIB_A LIB_B [--frames N] [--queries N] [--scene NAME] [--seed N]
 *   driver LIB [--frames N] ...            (one build alone: for timing it, e.g. under cachegrind)
 *
 * The queries: the five entry points every public collision call goes through (the floor, wall, ceiling, line and
 * sphere tests: BgCheck_RaycastDownImpl, BgCheck_CheckWallImpl, BgCheck_CheckCeilingImpl, BgCheck_CheckLineImpl,
 * BgCheck_SphVsFirstPolyImpl), with every flag combination, from points on and near the scene's triangles (on their
 * corners and edges too), anywhere in the scene's box, and around the platforms; with repeats and small moves
 * (what a standing or walking actor asks). Deterministic for a seed.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// (the game's own prototypes of these differ from the C library's in their size type)
#define bzero game_bzero
#define bcmp game_bcmp
#define bcopy game_bcopy
#include "ultra64.h"
#include "actor.h"
#include "bgcheck.h"
#include "play_state.h"
#include "bgcheck_check.h"
#undef bzero
#undef bcmp
#undef bcopy
#ifndef SHT_MAX
#define SHT_MAX 32767.0f // (as the game's libc/math.h)
#endif
#include <math.h>

/* (the platforms' paths only: any smooth function does) */
static f32 Path_Sin(s32 angle) {
    return sinf(angle * (6.2831853f / 65536.0f));
}

typedef f32 (*RaycastDownFn)(PlayState*, CollisionContext*, u16, CollisionPoly**, s32*, Vec3f*, Actor*, u32, f32);
typedef s32 (*CheckWallFn)(CollisionContext*, u16, Vec3f*, Vec3f*, Vec3f*, f32, CollisionPoly**, s32*, Actor*, f32, u8);
typedef s32 (*CheckCeilingFn)(CollisionContext*, u16, f32*, Vec3f*, f32, CollisionPoly**, s32*, Actor*);
typedef s32 (*CheckLineFn)(CollisionContext*, u16, u16, Vec3f*, Vec3f*, Vec3f*, CollisionPoly**, s32*, Actor*, f32,
                           u32);
typedef s32 (*SphVsFirstPolyFn)(CollisionContext*, u16, CollisionPoly**, s32*, Vec3f*, f32, Actor*, u16);
typedef void (*AllocateFn)(CollisionContext*, PlayState*, CollisionHeader*);
typedef s32 (*SetBgActorFn)(PlayState*, DynaCollisionContext*, Actor*, CollisionHeader*);
typedef void (*DynaFn)(PlayState*, DynaCollisionContext*);

#define MAX_PLATFORMS 8

typedef struct Lib {
    const char* path;
    void* handle;
    RaycastDownFn raycastDown;
    CheckWallFn checkWall;
    CheckCeilingFn checkCeiling;
    CheckLineFn checkLine;
    SphVsFirstPolyFn sphVsFirstPoly;
    AllocateFn allocate;
    SetBgActorFn setBgActor;
    DynaFn updateContext;
    DynaFn updateTransforms;
    PlayState* play;
    DynaPolyActor* platforms[MAX_PLATFORMS];
} Lib;

static void Platform_Update(Actor* actor, PlayState* play) {
}

static void* Lib_Sym(Lib* lib, const char* name) {
    void* p = dlsym(lib->handle, name);

    if (p == NULL) {
        fprintf(stderr, "%s: no %s\n", lib->path, name);
        exit(2);
    }
    return p;
}

static void Lib_Open(Lib* lib, const char* path) {
    lib->path = path;
    lib->handle = dlopen(path, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
    if (lib->handle == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        exit(2);
    }
    lib->raycastDown = Lib_Sym(lib, "BgCheck_RaycastDownImpl");
    lib->checkWall = Lib_Sym(lib, "BgCheck_CheckWallImpl");
    lib->checkCeiling = Lib_Sym(lib, "BgCheck_CheckCeilingImpl");
    lib->checkLine = Lib_Sym(lib, "BgCheck_CheckLineImpl");
    lib->sphVsFirstPoly = Lib_Sym(lib, "BgCheck_SphVsFirstPolyImpl");
    lib->allocate = Lib_Sym(lib, "BgCheck_Allocate");
    lib->setBgActor = Lib_Sym(lib, "DynaPoly_SetBgActor");
    lib->updateContext = Lib_Sym(lib, "DynaPoly_UpdateContext");
    lib->updateTransforms = Lib_Sym(lib, "DynaPoly_UpdateBgActorTransforms");
}

/* a small deterministic generator (xorshift64*) */
static u64 sRng;

static u32 Rng_U32(void) {
    sRng ^= sRng >> 12;
    sRng ^= sRng << 25;
    sRng ^= sRng >> 27;
    return (u32)((sRng * 0x2545F4914F6CDD1DULL) >> 32);
}

static f32 Rng_F(f32 lo, f32 hi) {
    return lo + (hi - lo) * ((Rng_U32() >> 8) * (1.0f / 16777216.0f));
}

static s32 Rng_Below(s32 n) {
    return Rng_U32() % n;
}

static u32 Bits(f32 f) {
    u32 w;

    memcpy(&w, &f, 4);
    return w;
}

static CollisionHeader* sCol;
static s32 sMismatches;
static s32 sChecked;
static const char* sSceneName;
static s32 sMemReport;
static s32 sFrame;
static s32 sQuery;

/* a poly as a number that means the same in both builds: the scene's index, or 100000 + its index in the build's own
   moving-platform list */
static s32 Lib_PolyId(Lib* lib, CollisionPoly* poly) {
    DynaCollisionContext* dyna = &lib->play->colCtx.dyna;

    if (poly == NULL) {
        return -1;
    }
    if ((poly >= sCol->polyList) && (poly < sCol->polyList + sCol->numPolygons)) {
        return poly - sCol->polyList;
    }
    if ((poly >= dyna->polyList) && (poly < dyna->polyList + dyna->polyListMax)) {
        return 100000 + (poly - dyna->polyList);
    }
    return -2;
}

static Actor* Lib_Actor(Lib* lib, s32 which) {
    return (which < 0) ? NULL : &lib->platforms[which]->actor;
}

static void Report(const char* what, const char* detail) {
    if (sMismatches < 20) {
        fprintf(stderr, "MISMATCH %s frame %d query %d: %s %s\n", sSceneName, sFrame, sQuery, what, detail);
    }
    sMismatches++;
}

static void Cmp_U(const char* what, const char* field, u32 x, u32 y) {
    if (x != y) {
        char d[96];

        snprintf(d, sizeof(d), "%s: %08X vs %08X (xor %08X)", field, x, y, x ^ y);
        Report(what, d);
    }
}

static void Cmp_V(const char* what, const char* field, Vec3f* x, Vec3f* y) {
    Cmp_U(what, field, Bits(x->x), Bits(y->x));
    Cmp_U(what, field, Bits(x->y), Bits(y->y));
    Cmp_U(what, field, Bits(x->z), Bits(y->z));
}

/* a point to ask from: on or near one of the scene's triangles, at or near a corner, anywhere in the box, or around a
   platform */
static void Gen_Point(Lib* lib, s32 numPlatforms, Vec3f* out) {
    s32 kind = Rng_Below(100);

    if ((kind < 55) && (sCol->numPolygons > 0)) {
        CollisionPoly* poly = &sCol->polyList[Rng_Below(sCol->numPolygons)];
        Vec3s* a = &sCol->vtxList[COLPOLY_VTX_INDEX(poly->flags_vIA)];
        Vec3s* b = &sCol->vtxList[COLPOLY_VTX_INDEX(poly->flags_vIB)];
        Vec3s* c = &sCol->vtxList[poly->vIC];
        f32 u = Rng_F(0.0f, 1.0f);
        f32 v = Rng_F(0.0f, 1.0f);
        f32 off;

        if (u + v > 1.0f) {
            u = 1.0f - u;
            v = 1.0f - v;
        }
        out->x = a->x + u * (b->x - a->x) + v * (c->x - a->x);
        out->y = a->y + u * (b->y - a->y) + v * (c->y - a->y);
        out->z = a->z + u * (b->z - a->z) + v * (c->z - a->z);
        off = (Rng_Below(2) == 0) ? Rng_F(-15.0f, 15.0f) : Rng_F(-120.0f, 120.0f);
        out->x += off * COLPOLY_GET_NORMAL(poly->normal.x) + Rng_F(-2.0f, 2.0f);
        out->y += off * COLPOLY_GET_NORMAL(poly->normal.y) + Rng_F(-2.0f, 2.0f);
        out->z += off * COLPOLY_GET_NORMAL(poly->normal.z) + Rng_F(-2.0f, 2.0f);
    } else if ((kind < 70) && (sCol->numVertices > 0)) {
        // a corner exactly, or a hair (or a unit) off it
        static const f32 offs[] = { 0.0f, 0.0f, 0.001f, -0.001f, 1.0f, -1.0f, 0.5f, 10.0f };
        Vec3s* a = &sCol->vtxList[Rng_Below(sCol->numVertices)];

        out->x = a->x + offs[Rng_Below(8)];
        out->y = a->y + offs[Rng_Below(8)];
        out->z = a->z + offs[Rng_Below(8)];
    } else if ((kind < 85) && (numPlatforms > 0)) {
        Actor* p = &lib->platforms[Rng_Below(numPlatforms)]->actor;

        out->x = p->world.pos.x + Rng_F(-300.0f, 300.0f);
        out->y = p->world.pos.y + Rng_F(-100.0f, 200.0f);
        out->z = p->world.pos.z + Rng_F(-300.0f, 300.0f);
    } else {
        out->x = Rng_F(sCol->minBounds.x - 200.0f, sCol->maxBounds.x + 200.0f);
        out->y = Rng_F(sCol->minBounds.y - 200.0f, sCol->maxBounds.y + 200.0f);
        out->z = Rng_F(sCol->minBounds.z - 200.0f, sCol->maxBounds.z + 200.0f);
    }
}

static u16 Gen_XpFlags(void) {
    return (Rng_Below(3) == 0) ? 0 : (u16)Rng_Below(8);
}

typedef struct Query {
    s32 kind;
    u16 xpFlags;
    u16 xpFlags2;
    u32 flags;
    f32 a;
    f32 b;
    Vec3f p;
    Vec3f q;
    s32 actor; // a platform's index, or -1
} Query;

static void Gen_Query(Lib* lib, s32 numPlatforms, Query* qy, Query* last) {
    s32 r = Rng_Below(100);

    if ((r < 12) && (last->kind >= 0)) {
        return; // the same question again (an actor standing still)
    }
    if ((r < 22) && (last->kind >= 0)) {
        // the same place asked differently (another actor there, another check): what the answer caches key on
        qy->xpFlags = Gen_XpFlags();
        qy->xpFlags2 = (Rng_Below(4) == 0) ? Gen_XpFlags() : 0;
        qy->actor = ((numPlatforms > 0) && (Rng_Below(5) == 0)) ? Rng_Below(numPlatforms) : -1;
        switch (Rng_Below(3)) {
            case 0:
                qy->flags = (qy->kind == 0) ? (u32)Rng_Below(32) : (qy->kind == 1) ? (u32)Rng_Below(2)
                            : (qy->kind == 3) ? (u32)Rng_Below(32) : (u32)Rng_Below(8);
                break;
            case 1:
                qy->a = (qy->kind == 0) ? ((Rng_Below(2) == 0) ? 1.0f : Rng_F(0.0f, 50.0f)) : Rng_F(1.0f, 90.0f);
                break;
            default:
                qy->b = Rng_F(0.0f, 80.0f);
                break;
        }
        return;
    }
    if ((r < 38) && (last->kind >= 0)) {
        // a small step from the last (an actor walking)
        qy->p.x += Rng_F(-6.0f, 6.0f);
        qy->p.y += Rng_F(-3.0f, 3.0f);
        qy->p.z += Rng_F(-6.0f, 6.0f);
        qy->q.x += Rng_F(-6.0f, 6.0f);
        qy->q.z += Rng_F(-6.0f, 6.0f);
        return;
    }
    qy->kind = Rng_Below(5);
    qy->xpFlags = Gen_XpFlags();
    qy->xpFlags2 = (Rng_Below(4) == 0) ? Gen_XpFlags() : 0;
    qy->actor = ((numPlatforms > 0) && (Rng_Below(5) == 0)) ? Rng_Below(numPlatforms) : -1;
    Gen_Point(lib, numPlatforms, &qy->p);
    switch (qy->kind) {
        case 0: // floor
            qy->flags = (Rng_Below(2) == 0) ? 0x1C : (u32)Rng_Below(32);
            qy->a = (Rng_Below(2) == 0) ? 1.0f : Rng_F(0.0f, 50.0f);
            break;
        case 1: { // wall: from q (the previous place) to p
            static const f32 radii[] = { 10.0f, 15.0f, 20.0f, 25.0f, 26.0f, 30.0f, 40.0f, 60.0f };
            f32 step = (Rng_Below(4) == 0) ? Rng_F(0.0f, 200.0f) : Rng_F(0.0f, 20.0f);

            qy->q.x = qy->p.x + Rng_F(-step, step);
            qy->q.y = qy->p.y + Rng_F(-step, step) * 0.5f;
            qy->q.z = qy->p.z + Rng_F(-step, step);
            if (Rng_Below(6) == 0) {
                qy->q = qy->p;
            }
            qy->a = (Rng_Below(2) == 0) ? radii[Rng_Below(8)] : Rng_F(1.0f, 90.0f);
            qy->b = Rng_F(0.0f, 80.0f);
            qy->flags = Rng_Below(2);
            break;
        }
        case 2: // ceiling
            qy->a = Rng_F(0.0f, 100.0f);
            break;
        case 3: { // line
            s32 l = Rng_Below(4);

            if (l == 0) {
                Gen_Point(lib, numPlatforms, &qy->q);
            } else {
                f32 len = (l == 1) ? 50.0f : (l == 2) ? 400.0f : 3000.0f;

                qy->q.x = qy->p.x + Rng_F(-len, len);
                qy->q.y = qy->p.y + Rng_F(-len, len);
                qy->q.z = qy->p.z + Rng_F(-len, len);
                if (Rng_Below(5) == 0) {
                    qy->q.x = qy->p.x; // vertical
                    qy->q.z = qy->p.z;
                }
            }
            qy->a = (Rng_Below(2) == 0) ? 1.0f : Rng_F(0.0f, 20.0f);
            qy->flags = Rng_Below(32);
            break;
        }
        default: // sphere
            qy->a = Rng_F(1.0f, 100.0f);
            qy->flags = Rng_Below(8);
            break;
    }
}

typedef struct Answer {
    u32 ret;
    s32 poly;
    s32 bgId;
    Vec3f v0;
    Vec3f v1;
    Vec3f v2;
    f32 f;
} Answer;

static void Ask(Lib* lib, Query* qy, Answer* ans) {
    CollisionContext* colCtx = &lib->play->colCtx;
    CollisionPoly* poly = NULL;
    s32 bgId = -7;
    Actor* actor = Lib_Actor(lib, qy->actor);

    memset(ans, 0, sizeof(*ans));
    switch (qy->kind) {
        case 0: {
            Vec3f pos = qy->p;

            ans->ret = Bits(lib->raycastDown(lib->play, colCtx, qy->xpFlags, &poly, &bgId, &pos, actor, qy->flags,
                                             qy->a));
            ans->v0 = pos;
            break;
        }
        case 1: {
            Vec3f next = qy->p;
            Vec3f prev = qy->q;
            Vec3f result;

            ans->ret = lib->checkWall(colCtx, qy->xpFlags, &result, &next, &prev, qy->a, &poly, &bgId, actor, qy->b,
                                      qy->flags);
            ans->v0 = result;
            ans->v1 = next;
            ans->v2 = prev;
            break;
        }
        case 2: {
            Vec3f pos = qy->p;
            f32 outY = -1234.5f;

            ans->ret = lib->checkCeiling(colCtx, qy->xpFlags, &outY, &pos, qy->a, &poly, &bgId, actor);
            ans->f = outY;
            ans->v0 = pos;
            break;
        }
        case 3: {
            Vec3f a = qy->p;
            Vec3f b = qy->q;
            Vec3f result = { -1.0f, -2.0f, -3.0f };

            ans->ret = lib->checkLine(colCtx, qy->xpFlags, qy->xpFlags2, &a, &b, &result, &poly, &bgId, actor, qy->a,
                                      qy->flags);
            ans->v0 = result;
            ans->v1 = a;
            ans->v2 = b;
            break;
        }
        default: {
            Vec3f center = qy->p;

            ans->ret = lib->sphVsFirstPoly(colCtx, qy->xpFlags, &poly, &bgId, &center, qy->a, actor, qy->flags);
            ans->v0 = center;
            break;
        }
    }
    ans->poly = Lib_PolyId(lib, poly);
    ans->bgId = bgId;
}

static const char* sKindNames[] = { "floor", "wall", "ceiling", "line", "sphere" };
static u32 sKindCount[5];

static void Compare(Query* qy, Answer* a, Answer* b) {
    const char* w = sKindNames[qy->kind];

    sChecked++;
    Cmp_U(w, "return", a->ret, b->ret);
    Cmp_U(w, "poly", a->poly, b->poly);
    Cmp_U(w, "bgId", a->bgId, b->bgId);
    Cmp_V(w, "v0", &a->v0, &b->v0);
    Cmp_V(w, "v1", &a->v1, &b->v1);
    Cmp_V(w, "v2", &a->v2, &b->v2);
    Cmp_U(w, "f", Bits(a->f), Bits(b->f));
}

/* the platforms: some still, some moving, some turning; one sometimes removed and another added */
typedef struct PlatformPlan {
    CollisionHeader* col;
    Vec3f base;
    f32 moveX;
    f32 moveY;
    s16 turnY;
    s16 turnX;
    s32 removeAt;
    f32 scale;
} PlatformPlan;

static void Platform_Place(Actor* actor, PlatformPlan* plan, s32 frame) {
    actor->world.pos.x = plan->base.x + plan->moveX * Path_Sin(frame * 300);
    actor->world.pos.y = plan->base.y + plan->moveY * Path_Sin(frame * 500);
    actor->world.pos.z = plan->base.z;
    actor->shape.rot.y = plan->turnY * frame;
    actor->shape.rot.x = plan->turnX * frame;
}

static void Platform_Make(Lib* lib, s32 i, PlatformPlan* plan) {
    DynaPolyActor* d = calloc(1, sizeof(DynaPolyActor) + 64);

    d->actor.update = Platform_Update;
    d->actor.scale.x = d->actor.scale.y = d->actor.scale.z = plan->scale;
    Platform_Place(&d->actor, plan, 0);
    lib->platforms[i] = d;
}

/* a tree's own changes to a scene's collision before it's tested (scene_extra.c, see run.sh): none here */
__attribute__((weak)) void BgCheckCheck_PrepareScene(BgCheckCheckScene* scene) {
}

static void Run_Scene(Lib* libs, s32 numLibs, BgCheckCheckScene* scene, s32 frames, s32 queries) {
    PlatformPlan plans[MAX_PLATFORMS];
    s32 numPlatforms = 0;
    s32 i;
    s32 l;
    Query qy;

    BgCheckCheck_PrepareScene(scene);
    sCol = scene->col;
    sSceneName = scene->name;
    for (l = 0; l < numLibs; l++) {
        libs[l].play = calloc(1, sizeof(PlayState));
        libs[l].play->sceneId = scene->sceneId;
        libs[l].allocate(&libs[l].play->colCtx, libs[l].play, sCol);
    }
    if (sMemReport) {
        SSNodeList* nodes = &libs[0].play->colCtx.polyNodes;
        s32 need = sCol->numVertices * 12 + sCol->numPolygons * 16 + 16;
        s32 spare = (nodes->max - nodes->count) * (s32)sizeof(SSNode);

        printf("MEM %-22s vtx %5d polys %5d need %6d spare %6d %s\n", scene->name, sCol->numVertices,
               sCol->numPolygons, need, spare, (spare >= need) ? "fits" : "SHORT");
    }
    if ((gBgCheckCheckPlatformCount > 0) && (sCol->numPolygons > 0)) {
        numPlatforms = Rng_Below(MAX_PLATFORMS + 1);
    }
    for (i = 0; i < numPlatforms; i++) {
        CollisionPoly* poly = &sCol->polyList[Rng_Below(sCol->numPolygons)];
        Vec3s* a = &sCol->vtxList[COLPOLY_VTX_INDEX(poly->flags_vIA)];
        PlatformPlan* p = &plans[i];

        p->col = gBgCheckCheckPlatforms[Rng_Below(gBgCheckCheckPlatformCount)];
        p->base.x = a->x;
        p->base.y = a->y + Rng_F(-50.0f, 100.0f);
        p->base.z = a->z;
        p->moveX = (Rng_Below(2) == 0) ? 0.0f : Rng_F(0.0f, 100.0f);
        p->moveY = (Rng_Below(2) == 0) ? 0.0f : Rng_F(0.0f, 60.0f);
        p->turnY = (Rng_Below(3) == 0) ? (s16)Rng_Below(800) : 0;
        p->turnX = (Rng_Below(6) == 0) ? (s16)Rng_Below(300) : 0;
        p->removeAt = (Rng_Below(4) == 0) ? Rng_Below(frames) : -1;
        p->scale = (Rng_Below(3) == 0) ? Rng_F(0.05f, 0.2f) : 0.1f;
        for (l = 0; l < numLibs; l++) {
            Platform_Make(&libs[l], i, p);
            libs[l].platforms[i]->bgId =
                libs[l].setBgActor(libs[l].play, &libs[l].play->colCtx.dyna, &libs[l].platforms[i]->actor, p->col);
        }
    }
    qy.kind = -1;
    for (sFrame = 0; sFrame < frames; sFrame++) {
        Query last;

        for (l = 0; l < numLibs; l++) {
            libs[l].updateContext(libs[l].play, &libs[l].play->colCtx.dyna);
        }
        for (sQuery = 0; sQuery < queries; sQuery++) {
            Answer ans[2];

            last = qy;
            Gen_Query(&libs[0], numPlatforms, &qy, &last);
            sKindCount[qy.kind]++;
            for (l = 0; l < numLibs; l++) {
                Ask(&libs[l], &qy, &ans[l]);
            }
            if (numLibs == 2) {
                Compare(&qy, &ans[0], &ans[1]);
            }
        }
        for (l = 0; l < numLibs; l++) {
            libs[l].updateTransforms(libs[l].play, &libs[l].play->colCtx.dyna);
            for (i = 0; i < numPlatforms; i++) {
                Actor* actor = &libs[l].platforms[i]->actor;

                if (plans[i].removeAt == sFrame) {
                    actor->update = NULL; // (DynaPoly_UpdateContext takes its slot away)
                } else if ((plans[i].removeAt >= 0) && (sFrame == plans[i].removeAt + 5)) {
                    // a new one in the freed slot (or the next free one)
                    actor->update = Platform_Update;
                    libs[l].platforms[i]->bgId =
                        libs[l].setBgActor(libs[l].play, &libs[l].play->colCtx.dyna, actor, plans[i].col);
                }
                Platform_Place(actor, &plans[i], sFrame + 1);
            }
        }
    }
}

int main(int argc, char** argv) {
    Lib libs[2];
    s32 numLibs = 0;
    s32 frames = 60;
    s32 queries = 300;
    const char* only = NULL;
    u64 seed = 1;
    s32 ramMB = 8;
    s32 i;

    memset(libs, 0, sizeof(libs));
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--frames")) {
            frames = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--queries")) {
            queries = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--scene")) {
            only = argv[++i];
        } else if (!strcmp(argv[i], "--ram")) {
            ramMB = atoi(argv[++i]); // (the console's memory: 4, or 8 with an Expansion Pak)
        } else if (!strcmp(argv[i], "--mem")) {
            sMemReport = 1; // (each scene's tables against its node table's unused tail)
        } else if (!strcmp(argv[i], "--seed")) {
            seed = strtoull(argv[++i], NULL, 0);
        } else if (numLibs < 2) {
            Lib_Open(&libs[numLibs++], argv[i]);
        }
    }
    for (i = 0; i < numLibs; i++) {
        u32* memSize = dlsym(libs[i].handle, "osMemSize");

        if (memSize != NULL) {
            *memSize = ramMB << 20;
        }
    }
    for (i = 0; i < gBgCheckCheckSceneCount; i++) {
        if ((only != NULL) && strcmp(only, gBgCheckCheckScenes[i].name)) {
            continue;
        }
        sRng = (seed * 0x9E3779B97F4A7C15ULL) ^ (u64)(i + 1) * 0xD1B54A32D192ED03ULL;
        Run_Scene(libs, numLibs, &gBgCheckCheckScenes[i], frames, queries);
    }
    printf("%d scenes, %d queries compared (floor %u, wall %u, ceiling %u, line %u, sphere %u): %d mismatches\n",
           (only != NULL) ? 1 : gBgCheckCheckSceneCount, sChecked, sKindCount[0], sKindCount[1], sKindCount[2],
           sKindCount[3], sKindCount[4], sMismatches);
    return sMismatches != 0;
}
