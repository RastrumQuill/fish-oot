/*
 * File: fishmod_particles.c
 * Overlay: ovl_En_Fishmod
 * Description: water particles: now and then a few bubbles rising from the weed near the camera
 * (vanilla's bubble, the one Ruto's letter and Link give off, popping at the surface with vanilla's ripple, so they
 * show from above the water too), and, while the camera is under the surface, faint specks drifting round it (the
 * soft dot of vanilla's Kokiri Forest dust motes, much smaller).
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
#include "assets/objects/gameplay_keep/gameplay_keep_0x4D160.h"
#include "assets/objects/gameplay_keep/sun_textures.h"


#define BUBBLE_MAX 24
#define SPECK_MAX 60
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
    s16 phase;
    u8 alive;
    u8 alpha; // how bright it gets
    u8 fade;  // 0-255: fading in when it appears, out before it goes
    u8 size;  // 1/1000 of vanilla's 31-unit dust mote
    u8 leaving;
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
    n = 1 + (s32)Fishmod_RandZeroFloat(gFishmodRngNow, 1.99f);
    for (i = 0; (i < BUBBLE_MAX) && (n > 0); i++) {
        FishmodBubble* b = &sBubbles[i];

        if (b->alive) {
            continue;
        }
        b->alive = true;
        b->pos.x = from.x + Fishmod_RandCenteredFloat(gFishmodRngNow, 10.0f);
        b->pos.y = from.y + 8.0f + Fishmod_RandZeroFloat(gFishmodRngNow, 30.0f) - n * 6.0f;
        b->pos.z = from.z + Fishmod_RandCenteredFloat(gFishmodRngNow, 10.0f);
        b->surface = surface;
        b->rise = 1.0f + Fishmod_RandZeroFloat(gFishmodRngNow, 0.8f);
        b->scale = 6 + (s32)Fishmod_RandZeroFloat(gFishmodRngNow, 9.0f);
        b->tex = Fishmod_RandZeroOne(gFishmodRngNow) < 0.5f;
        b->wobble = (s32)Fishmod_RandZeroFloat(gFishmodRngNow, 255.0f);
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
        f32 r = SPECK_NEAR + Fishmod_RandZeroFloat(gFishmodRngNow, SPECK_RANGE - SPECK_NEAR);
        s16 yaw = (s16)Fishmod_RandZeroFloat(gFishmodRngNow, 65535.0f);
        s16 pitch = (s16)Fishmod_RandCenteredFloat(gFishmodRngNow, 0x5000);

        p.x = eye->x + Math_CosS(pitch) * Math_SinS(yaw) * r;
        p.y = eye->y + Math_SinS(pitch) * r;
        p.z = eye->z + Math_CosS(pitch) * Math_CosS(yaw) * r;
        if (EnFishmod_WaterFloor(play, p.x, p.z, &surface, &floor, false) && (p.y < surface - 4.0f) &&
            (p.y > floor + 2.0f)) {
            s->pos = p;
            s->vel.x = Fishmod_RandCenteredFloat(gFishmodRngNow, 0.16f);
            s->vel.y = -0.02f - Fishmod_RandZeroFloat(gFishmodRngNow, 0.05f);
            s->vel.z = Fishmod_RandCenteredFloat(gFishmodRngNow, 0.16f);
            s->phase = (s16)Fishmod_RandZeroFloat(gFishmodRngNow, 65535.0f);
            s->alpha = 90 + (s32)Fishmod_RandZeroFloat(gFishmodRngNow, 90.0f);
            s->size = 50 + (s32)Fishmod_RandZeroFloat(gFishmodRngNow, 25.0f); // a little dot, about a unit across with a soft edge
            s->fade = 0;
            s->leaving = false;
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
            // a slow wander: its drift, plus a gentle sway that turns over every few seconds
            s->phase += 0x180;
            s->pos.x += s->vel.x + Math_SinS(s->phase) * 0.05f;
            s->pos.y += s->vel.y + Math_SinS((s16)(s->phase * 3 / 4 + 0x2000)) * 0.03f;
            s->pos.z += s->vel.z + Math_CosS((s16)(s->phase * 5 / 4)) * 0.05f;
            if ((SQ(s->pos.x - eye.x) + SQ(s->pos.y - eye.y) + SQ(s->pos.z - eye.z)) > SQ(SPECK_RANGE)) {
                s->leaving = true;
            }
            // fade in, and out before going (no popping)
            if (s->leaving) {
                if (s->fade <= 12) {
                    s->alive = false;
                } else {
                    s->fade -= 12;
                }
            } else if (s->fade < 255) {
                s->fade = (s->fade > 255 - 6) ? 255 : s->fade + 6;
            }
        }
        // a few each frame, so arriving under water doesn't place them all at once
        if (!s->alive && (((i + sTimer) & 7) == 0)) {
            Particles_PlaceSpeck(play, s, &eye);
        }
    }
}

/** An s15.16 entry of a fixed-point matrix. */
static void Particles_Set(Mtx* mtx, s32 r, s32 c, f32 v) {
    s32 f = (s32)(v * 65536.0f);

    mtx->intPart[r][c] = (u32)f >> 16;
    mtx->fracPart[r][c] = f & 0xFFFF;
}

/**
 * A particle's matrix: sized and placed, and turned to face the camera with `billboard` (or NULL when the display
 * list does that itself). Positions keep their fractions, so slow movement is smooth.
 */
static Mtx* Particles_Matrix(Vec3f* pos, f32 scale, MtxF* billboard) {
    Mtx* mtx = &sPartMtx[sPartFrame][sPartUsed++];
    s32 r;
    s32 c;

    for (r = 0; r < 3; r++) {
        for (c = 0; c < 3; c++) {
            Particles_Set(mtx, r, c, (billboard != NULL) ? billboard->mf[r][c] * scale : ((r == c) ? scale : 0.0f));
        }
        Particles_Set(mtx, r, 3, 0.0f);
    }
    Particles_Set(mtx, 3, 0, pos->x);
    Particles_Set(mtx, 3, 1, pos->y);
    Particles_Set(mtx, 3, 2, pos->z);
    Particles_Set(mtx, 3, 3, 1.0f);
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
        gSPMatrix(POLY_OPA_DISP++, Particles_Matrix(&b->pos, b->scale * 0.01f, NULL),
                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, SEGMENTED_TO_VIRTUAL(gEffBubbleDL));
    }
    gSPTexture(POLY_OPA_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);

    // specks: vanilla's dust mote (a soft dot), tiny, faint and the colour of the water, see-through
    if (sUnder) {
        POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_20);
        gSPSegment(POLY_XLU_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(gSun1Tex));
        gSPDisplayList(POLY_XLU_DISP++, gKokiriDustMoteMaterialDL);
        for (i = 0; i < SPECK_MAX; i++) {
            FishmodSpeck* s = &sSpecks[i];
            u8 a;

            if (!s->alive || (s->fade == 0)) {
                continue;
            }
            a = s->alpha * s->fade / 255;
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 220, 230, 210, a);
            gDPSetEnvColor(POLY_XLU_DISP++, 90, 120, 100, a);
            gSPMatrix(POLY_XLU_DISP++, Particles_Matrix(&s->pos, s->size * 0.001f, &play->billboardMtxF),
                      G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gKokiriDustMoteModelDL);
        }
    }
    CLOSE_DISPS(gfxCtx, "../fishmod_particles.c", __LINE__);
}

