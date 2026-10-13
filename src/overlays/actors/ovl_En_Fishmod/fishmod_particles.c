/*
 * File: fishmod_particles.c
 * Overlay: ovl_En_Fishmod
 * Description: water particles (FISHMOD_OPT 5): now and then a few bubbles rising from the weed near the camera
 * (vanilla's bubble, the one Ruto's letter and Link give off, popping at the surface with vanilla's ripple, so they
 * show from above the water too), and, while the camera is under the surface, faint specks drifting round it.
 *
 * Cheaper than spawning vanilla's bubble effect: no effect slots, no water lookups each frame (a bubble learns the
 * surface once, from the baked water map, when it's made), and one setup for all of them; each is then a matrix and
 * vanilla's display list (which turns it to face the camera itself).
 */

#include "z_en_fishmod.h"
#include "fishmod.h"

#include "gfx.h"
#include "gfx_setupdl.h"
#include "play_state.h"
#include "segmented_address.h"
#include "z_lib.h"
#include "array_count.h"
#include "effect.h"
#include "rand.h"
#include "libc/math.h"
#include "ultra64.h"
#include "assets/objects/gameplay_keep/gameplay_keep_0x55DB0.h"
#include "assets/objects/gameplay_keep/eff_bubble.h"

#if FISHMOD_OPT >= 5

#define BUBBLE_MAX 24
#define SPECK_MAX 40
#define BUBBLE_SPAWN_EVERY 25  // frames between bubble trails (an occasional one, not a stream like Ruto's bottle)
#define BUBBLE_RANGE 450.0f    // from the camera: born this close, gone past 700
#define SPECK_RANGE 260.0f     // specks live in a ball this big round the camera
#define SPECK_NEAR 40.0f       // and not right in front of the lens (they'd fill the screen)
#define RIPPLE_RANGE 600.0f    // only bubbles this close to the camera leave a ripple

typedef struct FishmodBubble {
    Vec3f pos;
    f32 surface; // where it pops
    f32 rise;
    u8 scale; // 1/100
    u8 tex;   // which of vanilla's two
    u8 alive;
    u8 wobble;
} FishmodBubble;

typedef struct FishmodSpeck {
    Vec3f pos;
    Vec3f vel;
    u8 alive;
    u8 alpha;
} FishmodSpeck;

static FishmodBubble sBubbles[BUBBLE_MAX];
static FishmodSpeck sSpecks[SPECK_MAX];
static s32 sActive;
static s32 sUnder; // the camera is under water: the specks show
static s32 sTimer;
// two frames' worth of matrices (the RSP reads last frame's while this one is built)
static Mtx sPartMtx[2][BUBBLE_MAX + SPECK_MAX];
static s32 sPartFrame;
static s32 sPartUsed;

static void Particles_SpawnTrail(PlayState* play, Vec3f* eye) {
    Vec3f from;
    f32 surface;
    f32 floor;
    s32 n;
    s32 i;

    if (!EnFishmod_ScatterRandomDrawnPlant(eye, BUBBLE_RANGE, &from)) {
        return;
    }
    if (!EnFishmod_WaterFloor(play, from.x, from.z, &surface, &floor, false)) {
        return;
    }
    // a short trail: one or two bubbles, a little apart
    n = 1 + (s32)Rand_ZeroFloat(1.99f);
    for (i = 0; (i < BUBBLE_MAX) && (n > 0); i++) {
        FishmodBubble* b = &sBubbles[i];

        if (b->alive) {
            continue;
        }
        b->alive = true;
        b->pos.x = from.x + Rand_CenteredFloat(10.0f);
        b->pos.y = from.y + 8.0f + Rand_ZeroFloat(30.0f) - n * 6.0f;
        b->pos.z = from.z + Rand_CenteredFloat(10.0f);
        b->surface = surface;
        b->rise = 1.0f + Rand_ZeroFloat(0.8f);
        b->scale = 6 + (s32)Rand_ZeroFloat(9.0f);
        b->tex = Rand_ZeroOne() < 0.5f;
        b->wobble = (s32)Rand_ZeroFloat(255.0f);
        n--;
    }
}

static void Particles_PlaceSpeck(PlayState* play, FishmodSpeck* s, Vec3f* eye) {
    f32 surface;
    f32 floor;
    s32 tries;

    s->alive = false;
    for (tries = 0; tries < 3; tries++) {
        Vec3f p;
        f32 r = SPECK_NEAR + Rand_ZeroFloat(SPECK_RANGE - SPECK_NEAR);
        s16 yaw = (s16)Rand_ZeroFloat(65535.0f);
        s16 pitch = (s16)Rand_CenteredFloat(0x5000);

        p.x = eye->x + Math_CosS(pitch) * Math_SinS(yaw) * r;
        p.y = eye->y + Math_SinS(pitch) * r;
        p.z = eye->z + Math_CosS(pitch) * Math_CosS(yaw) * r;
        if (EnFishmod_WaterFloor(play, p.x, p.z, &surface, &floor, false) && (p.y < surface - 4.0f) &&
            (p.y > floor + 2.0f)) {
            s->pos = p;
            s->vel.x = Rand_CenteredFloat(0.3f);
            s->vel.y = -0.05f - Rand_ZeroFloat(0.1f);
            s->vel.z = Rand_CenteredFloat(0.3f);
            s->alpha = 50 + (s32)Rand_ZeroFloat(70.0f);
            s->alive = true;
            return;
        }
    }
}

void EnFishmod_ParticlesUpdate(PlayState* play) {
    Vec3f eye = play->view.eye;
    s32 i;

    if (!(eye.x == eye.x) || !(eye.z == eye.z) || (fabsf(eye.x) > 30000.0f) || (fabsf(eye.z) > 30000.0f)) {
        return;
    }
    sActive = true;
    sUnder = (gFishmodHaze >= 0.5f);
    sTimer++;

    // bubbles: rise with a little wobble, pop at the surface
    if ((sTimer % BUBBLE_SPAWN_EVERY) == 0) {
        Particles_SpawnTrail(play, &eye);
    }
    for (i = 0; i < BUBBLE_MAX; i++) {
        FishmodBubble* b = &sBubbles[i];

        if (!b->alive) {
            continue;
        }
        b->wobble += 9;
        b->pos.y += b->rise;
        b->pos.x += Math_SinS(b->wobble << 8) * 0.25f;
        b->rise = CLAMP_MAX(b->rise + 0.02f, 2.6f); // a little faster as it goes up
        if (b->pos.y >= b->surface - 1.0f) {
            b->alive = false;
            if ((SQ(b->pos.x - eye.x) + SQ(b->pos.z - eye.z)) < SQ(RIPPLE_RANGE)) {
                Vec3f ripple = b->pos;

                ripple.y = b->surface;
                EffectSsGRipple_Spawn(play, &ripple, 0, 60, 0);
            }
        } else if ((SQ(b->pos.x - eye.x) + SQ(b->pos.z - eye.z)) > SQ(700.0f)) {
            b->alive = false;
        }
    }

    // specks (under water only): drift slowly; those that leave the ball round the camera come back elsewhere in it
    if (!sUnder) {
        for (i = 0; i < SPECK_MAX; i++) {
            sSpecks[i].alive = false;
        }
        return;
    }
    for (i = 0; i < SPECK_MAX; i++) {
        FishmodSpeck* s = &sSpecks[i];

        if (s->alive) {
            s->pos.x += s->vel.x;
            s->pos.y += s->vel.y;
            s->pos.z += s->vel.z;
            if ((SQ(s->pos.x - eye.x) + SQ(s->pos.y - eye.y) + SQ(s->pos.z - eye.z)) > SQ(SPECK_RANGE + 20.0f)) {
                s->alive = false;
            }
        }
        // a few each frame, so arriving under water doesn't place them all at once
        if (!s->alive && (((i + sTimer) & 7) == 0)) {
            Particles_PlaceSpeck(play, s, &eye);
        }
    }
}

/** A matrix that places and sizes (the display list's own billboard turns it to the camera). */
static Mtx* Particles_Matrix(Vec3f* pos, f32 scale) {
    Mtx* mtx;
    s32 s = (s32)(scale * 0x10000);
    s32 i;

    mtx = &sPartMtx[sPartFrame][sPartUsed++];
    for (i = 0; i < 4; i++) {
        mtx->m[0][i] = 0;
        mtx->m[1][i] = 0;
        mtx->m[2][i] = 0;
        mtx->m[3][i] = 0;
    }
    // integer parts in the first half, fractions in the second (s15.16)
    mtx->intPart[0][0] = (u32)s >> 16;
    mtx->fracPart[0][0] = s & 0xFFFF;
    mtx->intPart[1][1] = (u32)s >> 16;
    mtx->fracPart[1][1] = s & 0xFFFF;
    mtx->intPart[2][2] = (u32)s >> 16;
    mtx->fracPart[2][2] = s & 0xFFFF;
    mtx->intPart[3][0] = (s16)pos->x;
    mtx->intPart[3][1] = (s16)pos->y;
    mtx->intPart[3][2] = (s16)pos->z;
    mtx->intPart[3][3] = 1;
    return mtx;
}

void EnFishmod_ParticlesDraw(PlayState* play) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    void* tex[2];
    s32 lastTex = -1;
    s32 i;

    if (!sActive) {
        return;
    }
    tex[0] = SEGMENTED_TO_VIRTUAL(gEffBubble1Tex);
    tex[1] = SEGMENTED_TO_VIRTUAL(gEffBubble2Tex);
    sPartFrame ^= 1;
    sPartUsed = 0;

    OPEN_DISPS(gfxCtx, "../fishmod_particles.c", __LINE__);
    Gfx_SetupDL_25Opa(gfxCtx);
    // bubbles: as vanilla draws them
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 150, 150, 150, 0);
    for (i = 0; i < BUBBLE_MAX; i++) {
        FishmodBubble* b = &sBubbles[i];

        if (!b->alive) {
            continue;
        }
        if (b->tex != lastTex) {
            lastTex = b->tex;
            gSPSegment(POLY_OPA_DISP++, 0x08, tex[b->tex]);
        }
        gSPMatrix(POLY_OPA_DISP++, Particles_Matrix(&b->pos, b->scale * 0.01f),
                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, SEGMENTED_TO_VIRTUAL(gEffBubbleDL));
    }
    // specks: the same little sprite, tiny, dim and murky-coloured
    gSPSegment(POLY_OPA_DISP++, 0x08, tex[0]);
    gDPSetEnvColor(POLY_OPA_DISP++, 70, 90, 80, 0);
    for (i = 0; i < SPECK_MAX; i++) {
        FishmodSpeck* s = &sSpecks[i];

        if (!s->alive) {
            continue;
        }
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 200, 210, 190, s->alpha);
        gSPMatrix(POLY_OPA_DISP++, Particles_Matrix(&s->pos, 0.035f), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, SEGMENTED_TO_VIRTUAL(gEffBubbleDL));
    }
    gSPTexture(POLY_OPA_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    CLOSE_DISPS(gfxCtx, "../fishmod_particles.c", __LINE__);
}

#endif
