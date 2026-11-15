/*
 * File: z_eff_ss_g_ripple.c
 * Overlay: ovl_Effect_Ss_G_Ripple
 * Description: Water Ripple
 */

#include "z_eff_ss_g_ripple.h"

#include "gfx.h"
#include "gfx_setupdl.h"
#include "segmented_address.h"
#include "stack_pad.h"
#include "z_lib.h"
#include "effect.h"
#include "play_state.h"
#include "skin_matrix.h"

#include "assets/objects/gameplay_keep/eff_water_ripple.h"

#define rWaterBoxNum regs[0]
#define rRadius regs[1]
#define rRadiusMax regs[2]
#define rPrimColorR regs[3]
#define rPrimColorG regs[4]
#define rPrimColorB regs[5]
#define rPrimColorA regs[6]
#define rEnvColorR regs[7]
#define rEnvColorG regs[8]
#define rEnvColorB regs[9]
#define rEnvColorA regs[10]
#define rLifespan regs[11]

u32 EffectSsGRipple_Init(PlayState* play, u32 index, EffectSs* this, void* initParamsx);
void EffectSsGRipple_Draw(PlayState* play, u32 index, EffectSs* this);
void EffectSsGRipple_Update(PlayState* play, u32 index, EffectSs* this);

EffectSsProfile Effect_Ss_G_Ripple_Profile = {
    EFFECT_SS_G_RIPPLE,
    EffectSsGRipple_Init,
};

u32 EffectSsGRipple_Init(PlayState* play, u32 index, EffectSs* this, void* initParamsx) {
    STACK_PAD(s32);
    Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
    WaterBox* waterBox;
    EffectSsGRippleInitParams* initParams = (EffectSsGRippleInitParams*)initParamsx;

    waterBox = NULL;
    this->velocity = this->accel = zeroVec;
    this->pos = initParams->pos;
    this->gfx = SEGMENTED_TO_VIRTUAL(gEffWaterRippleDL);
    this->life = initParams->life + 20;
    this->flags = 0;
    this->draw = EffectSsGRipple_Draw;
    this->update = EffectSsGRipple_Update;
    this->rRadius = initParams->radius;
    this->rRadiusMax = initParams->radiusMax;
    this->rLifespan = initParams->life;
    this->rPrimColorR = 255;
    this->rPrimColorG = 255;
    this->rPrimColorB = 255;
    this->rPrimColorA = 255;
    this->rEnvColorR = 255;
    this->rEnvColorG = 255;
    this->rEnvColorB = 255;
    this->rEnvColorA = 255;
    this->rWaterBoxNum = BgCheck_FindWaterBox(play, &play->colCtx, &initParams->pos, 3.0f, &waterBox);

    return 1;
}

/*
 * fish-oot: a ripple drawn right after another (nothing between them in the list) doesn't load the texture again:
 * gEffWaterRippleDL without its load, TMEM and the tiles still holding the last one's (the setup list and colours
 * between don't touch them). The same picture: a schooling fish's ripples come many to a frame, each loaded 4 KB.
 */
static Gfx sRippleNoLoadDL[] = {
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsSPClearGeometryMode(G_CULL_BACK),
    gsSPVertex(gEffWaterRippleVtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};
static Gfx* sRippleListEnd; // where the last ripple's commands ended, and in which frame
static u32 sRippleFrame;

void EffectSsGRipple_DrawRipple(PlayState* play2, EffectSs* this, void* segment) {
    PlayState* play = play2;
    f32 radius;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    MtxF mfResult;
    Mtx* mtx;
    f32 yPos;
    s32 loaded;

    OPEN_DISPS(gfxCtx, "../z_eff_ss_g_ripple.c", 199);

    loaded = (POLY_XLU_DISP == sRippleListEnd) && (sRippleFrame == play->state.frames) &&
             (this->gfx == SEGMENTED_TO_VIRTUAL(gEffWaterRippleDL));

    radius = this->rRadius * 0.0025f;

    if ((this->rWaterBoxNum != -1) && (this->rWaterBoxNum < play->colCtx.colHeader->numWaterBoxes)) {
        yPos = (this->rWaterBoxNum + play->colCtx.colHeader->waterBoxes)->ySurface;
    } else {
        yPos = this->pos.y;
    }

    // fishmod: the translation times the scale, written as the product comes out (the scale on the diagonal, the
    // position in the last column) instead of two matrices and a 64-multiply product: the same matrix once converted
    // to fixed point (the product's zeros can only differ from these in sign)
    mfResult.xx = radius;
    mfResult.yx = 0.0f;
    mfResult.zx = 0.0f;
    mfResult.wx = 0.0f;
    mfResult.xy = 0.0f;
    mfResult.yy = radius;
    mfResult.zy = 0.0f;
    mfResult.wy = 0.0f;
    mfResult.xz = 0.0f;
    mfResult.yz = 0.0f;
    mfResult.zz = radius;
    mfResult.wz = 0.0f;
    mfResult.xw = this->pos.x;
    mfResult.yw = yPos;
    mfResult.zw = this->pos.z;
    mfResult.ww = 1.0f;

    mtx = SkinMatrix_MtxFToNewMtx(gfxCtx, &mfResult);

    if (mtx != NULL) {
        gSPMatrix(POLY_XLU_DISP++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        Gfx_SetupDL_60NoCDXlu(gfxCtx);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, this->rPrimColorR, this->rPrimColorG, this->rPrimColorB,
                        this->rPrimColorA);
        gDPSetEnvColor(POLY_XLU_DISP++, this->rEnvColorR, this->rEnvColorG, this->rEnvColorB, this->rEnvColorA);
        gDPSetAlphaDither(POLY_XLU_DISP++, G_AD_NOISE);
        gDPSetColorDither(POLY_XLU_DISP++, G_CD_NOISE);
        gSPDisplayList(POLY_XLU_DISP++, loaded ? sRippleNoLoadDL : this->gfx);
        if (this->gfx == SEGMENTED_TO_VIRTUAL(gEffWaterRippleDL)) {
            sRippleListEnd = POLY_XLU_DISP;
            sRippleFrame = play->state.frames;
        }
    }

    CLOSE_DISPS(gfxCtx, "../z_eff_ss_g_ripple.c", 247);
}

void EffectSsGRipple_Draw(PlayState* play, u32 index, EffectSs* this) {
    if (this->rLifespan == 0) {
        EffectSsGRipple_DrawRipple(play, this, gEffWaterRippleTex);
    }
}

void EffectSsGRipple_Update(PlayState* play, u32 index, EffectSs* this) {
    f32 radius;
    f32 primAlpha;
    f32 envAlpha;

    if (DECR(this->rLifespan) == 0) {
        radius = this->rRadius;
        Math_SmoothStepToF(&radius, this->rRadiusMax, 0.2f, 30.0f, 1.0f);
        this->rRadius = radius;

        primAlpha = this->rPrimColorA;
        envAlpha = this->rEnvColorA;

        Math_SmoothStepToF(&primAlpha, 0.0f, 0.2f, 15.0f, 7.0f);
        Math_SmoothStepToF(&envAlpha, 0.0f, 0.2f, 15.0f, 7.0f);

        this->rPrimColorA = primAlpha;
        this->rEnvColorA = envAlpha;
    }
}
