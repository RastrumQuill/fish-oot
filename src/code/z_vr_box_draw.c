#include "fishmod.h"
#include "skybox.h"

#include "gfx.h"
#include "attributes.h"
#include "gfx_setupdl.h"
#include "sys_matrix.h"

#include "ultra64.h"

Mtx* sSkyboxDrawMatrix;

Mtx* Skybox_UpdateMatrix(SkyboxContext* skyboxCtx, f32 x, f32 y, f32 z) {
    Matrix_Translate(x, y, z, MTXMODE_NEW);
    Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);
    Matrix_RotateX(skyboxCtx->rot.x, MTXMODE_APPLY);
    Matrix_RotateY(skyboxCtx->rot.y, MTXMODE_APPLY);
    Matrix_RotateZ(skyboxCtx->rot.z, MTXMODE_APPLY);
    return MATRIX_TO_MTX(sSkyboxDrawMatrix, "../z_vr_box_draw.c", 42);
}

extern Gfx (*gFishmodSkyOneTex)[150]; // z_vr_box.c

void Skybox_Draw(SkyboxContext* skyboxCtx, GraphicsContext* gfxCtx, s16 skyboxId, s16 blend, f32 x, f32 y, f32 z) {
    OPEN_DISPS(gfxCtx, "../z_vr_box_draw.c", 52);

    Gfx_SetupDL_40Opa(gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x7, skyboxCtx->staticSegments[0]);
    gSPSegment(POLY_OPA_DISP++, 0x8, skyboxCtx->staticSegments[1]);
    gSPSegment(POLY_OPA_DISP++, 0x9, skyboxCtx->palettes);

    gDPSetPrimColor(POLY_OPA_DISP++, 0x00, 0x00, 0, 0, 0, blend);
    gSPTexture(POLY_OPA_DISP++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);

    // Prepare matrix
    sSkyboxDrawMatrix = GRAPH_ALLOC(gfxCtx, sizeof(Mtx));
    Matrix_Translate(x, y, z, MTXMODE_NEW);
    Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);
    Matrix_RotateX(skyboxCtx->rot.x, MTXMODE_APPLY);
    Matrix_RotateY(skyboxCtx->rot.y, MTXMODE_APPLY);
    Matrix_RotateZ(skyboxCtx->rot.z, MTXMODE_APPLY);
    MATRIX_TO_MTX(sSkyboxDrawMatrix, "../z_vr_box_draw.c", 76);
    gSPMatrix(POLY_OPA_DISP++, sSkyboxDrawMatrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    // fish-oot (frames11): the sky writes full coverage outright (CVG_DST_FULL), not added to what's there (the
    // setup's CVG_DST_CLAMP): with the colour fill under it (full coverage everywhere) the two are the same, and
    // without it (Lake Hylia: Play_Draw leaves the fill out) the coverage left from an older frame doesn't come
    // through into the edges drawn over the sky
    gDPSetRenderMode(POLY_OPA_DISP++, (G_RM_OPA_SURF & ~CVG_DST_CLAMP) | CVG_DST_FULL,
                     (G_RM_OPA_SURF2 & ~CVG_DST_CLAMP) | CVG_DST_FULL);

    // Enable magic square RGB dithering and bilinear filtering
    gDPSetColorDither(POLY_OPA_DISP++, G_CD_MAGICSQ);
    gDPSetTextureFilter(POLY_OPA_DISP++, G_TF_BILERP);

    // All skyboxes use CI8 textures with an RGBA16 palette
    gDPLoadTLUT_pal256(POLY_OPA_DISP++, skyboxCtx->palettes[0]);
    gDPSetTextureLUT(POLY_OPA_DISP++, G_TT_RGBA16);

    // Enable texture filtering RDP pipeline stages for bilinear filtering
    gDPSetTextureConvert(POLY_OPA_DISP++, G_TC_FILT);

    if (skyboxCtx->drawType != SKYBOX_DRAW_128) {
        // 256x256 textures, per-face palettes
        // 2, 3 or 4 faces

        gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[0]); // -z face upper
        gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[1]); // -z face lower

        gDPPipeSync(POLY_OPA_DISP++);
        gDPLoadTLUT_pal256(POLY_OPA_DISP++, skyboxCtx->palettes[1]);
        gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[2]); // +x face upper
        gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[3]); // +x face lower

        if (skyboxId != SKYBOX_BAZAAR) {
            if (skyboxId < SKYBOX_KOKIRI_SHOP || skyboxId > SKYBOX_BOMBCHU_SHOP) {
                // Skip remaining faces for most shop skyboxes

                gDPPipeSync(POLY_OPA_DISP++);
                gDPLoadTLUT_pal256(POLY_OPA_DISP++, skyboxCtx->palettes[2]);
                gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[4]); // +z face upper
                gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[5]); // +z face lower

                // Note this pipesync is slightly misplaced and would be better off inside the condition
                gDPPipeSync(POLY_OPA_DISP++);

                if (skyboxCtx->drawType != SKYBOX_DRAW_256_3FACE) {
                    gDPLoadTLUT_pal256(POLY_OPA_DISP++, skyboxCtx->palettes[3]);
                    gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[6]); // -x face upper
                    gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[7]); // -x face lower
                }
            }
        }
    } else if ((blend == 0) && (gFishmodSkyOneTex != NULL) && !FISHMOD_BENCH_SKIP(FISHMOD_BENCH_OLDSKY_1CYC)) {
        // fish-oot: not blending two skies: 1-cycle, one texture (the same picture: see gFishmodSkyOneTex)
        gDPPipeSync(POLY_OPA_DISP++);
        gDPSetCycleType(POLY_OPA_DISP++, G_CYC_1CYCLE);
        gDPSetCombineMode(POLY_OPA_DISP++, G_CC_DECALRGBA, G_CC_DECALRGBA);
        gDPSetRenderMode(POLY_OPA_DISP++, (G_RM_OPA_SURF & ~CVG_DST_CLAMP) | CVG_DST_FULL,
                         (G_RM_OPA_SURF2 & ~CVG_DST_CLAMP) | CVG_DST_FULL);
        gSPDisplayList(POLY_OPA_DISP++, gFishmodSkyOneTex[0]); // -z face
        gSPDisplayList(POLY_OPA_DISP++, gFishmodSkyOneTex[2]); // +z face
        gSPDisplayList(POLY_OPA_DISP++, gFishmodSkyOneTex[4]); // -x face
        gSPDisplayList(POLY_OPA_DISP++, gFishmodSkyOneTex[6]); // +x face
        gSPDisplayList(POLY_OPA_DISP++, gFishmodSkyOneTex[8]); // +y face
        if (skyboxId == SKYBOX_CUTSCENE_MAP) {
            gSPDisplayList(POLY_OPA_DISP++, gFishmodSkyOneTex[10]); // -y face
        }
    } else {
        // 128x128 and 128x64 textures
        // 5 or 6 faces

        // Draw each face
        gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[0]); // -z face
        gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[2]); // +z face
        gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[4]); // -x face
        gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[6]); // +x face
        gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[8]); // +y face
        if (skyboxId == SKYBOX_CUTSCENE_MAP) {
            // Only draw the bottom face in the cutscene map
            gSPDisplayList(POLY_OPA_DISP++, skyboxCtx->dListBuf[10]); // -y face
        }
    }

    gDPPipeSync(POLY_OPA_DISP++);

    CLOSE_DISPS(gfxCtx, "../z_vr_box_draw.c", 125);
}

void Skybox_Update(UNUSED SkyboxContext* skyboxCtx) {
}
