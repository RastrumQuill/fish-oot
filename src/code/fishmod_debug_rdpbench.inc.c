// Part of fishmod_debug.c (#included there, sharing its statics): the RDP calibration bench (FISHMOD_RDP_BENCH).
#ifdef FISHMOD_RDP_BENCH
/*
 * The RDP calibration bench (fishmod/scripts/rdpbench.inc.c): controlled drawing, one test at a time, so the console's
 * RDP time for each kind of work can be measured and an RDP cost model fitted (and ares's RDP timing made to follow it).
 *
 * In play, each frame after the frame setup (colour and depth buffers, scissor: Gfx_SetupFrame, the same every frame),
 * the game draws nothing of its own (Play_Draw skips to its end) and this draws the test instead: fill rectangles, texture
 * rectangles in each texture format, filter, cycle type and render mode, triangle grids of four sizes in six modes,
 * texture loads, and mode changes. Each test is held RB_FRAMES frames (the first RB_SETTLE not counted: the RDP's time
 * is measured a frame or two behind), RB_REPS times round. Every frame's record goes to the frame log (RDP time, its
 * four counters, the RSP's time, F3DEX3's counters), tagged test << 24 | repetition << 16 | counted; each test's
 * description goes over USB as a text line at the start ("RDPTEST ..."). Read with fishmod/tools/rdpbench.py.
 */
#define RB_FRAMES 30
#define RB_SETTLE 10
#define RB_REPS 3
#define RB_WARMUP 120 // play frames before the first test (the scene loaded, the profiler started)

typedef enum RbKind {
    RBK_EMPTY,   // nothing: the frame setup alone (the baseline)
    RBK_FILL,    // fill-mode rectangles
    RBK_TEXRECT, // texture rectangles
    RBK_TRIS,    // a grid of quads (two triangles each), through the RSP
    RBK_LOADS,   // texture rectangles, the texture loaded again before each
    RBK_SYNCS,   // small texture rectangles, a pipe sync before each
    RBK_SWITCH   // small texture rectangles, the render mode switched (with a sync) before each
} RbKind;

typedef enum RbMode {
    RBM_SHADE,     // 1-cycle, shade colour, no texture, no Z
    RBM_SHADE_Z,   // the same, Z compared and written
    RBM_TEX16P,    // 1-cycle, RGBA16 32x32, point sampled
    RBM_TEX16,     // bilinear
    RBM_TEX16_Z,   // bilinear, Z
    RBM_CI8,       // CI8 32x32 with a 256-colour palette, bilinear
    RBM_I4,        // I4 64x64, bilinear
    RBM_TEX32,     // RGBA32 32x32, bilinear
    RBM_2CYC,      // 2-cycle: texture x colour, then pass; Z
    RBM_2CYC_FOG,  // 2-cycle: texture x shade, fog in the blender, anti-aliased, Z (the game's usual world surface)
    RBM_XLU,       // translucent: the colour buffer read and blended
    RBM_XLU_Z,     // translucent with Z compared
    RBM_AA_Z,      // anti-aliased opaque with Z (coverage written)
    RBM_MAX
} RbMode;

static const char* sRbModeNames[RBM_MAX] = {
    "shade", "shade+z", "rgba16 point", "rgba16 bilinear", "rgba16+z", "ci8", "i4", "rgba32",
    "2cycle+z", "2cycle fog aa+z", "xlu", "xlu+z", "aa+z",
};

typedef struct RdpTest {
    u8 kind;
    u8 mode;
    u16 count; // rectangles or quads
    u16 size;  // their width and height, pixels
} RdpTest;

static const RdpTest sRdpTests[] = {
    { RBK_EMPTY, 0, 0, 0 },
    // fill rate: full-screen fill rectangles
    { RBK_FILL, 0, 1, 320 },
    { RBK_FILL, 0, 4, 320 },
    // texture rectangles, full screen four deep, by format, filter, cycle and blending
    { RBK_TEXRECT, RBM_TEX16P, 4, 320 },
    { RBK_TEXRECT, RBM_TEX16, 4, 320 },
    { RBK_TEXRECT, RBM_TEX16_Z, 4, 320 },
    { RBK_TEXRECT, RBM_CI8, 4, 320 },
    { RBK_TEXRECT, RBM_I4, 4, 320 },
    { RBK_TEXRECT, RBM_TEX32, 4, 320 },
    { RBK_TEXRECT, RBM_2CYC, 4, 320 },
    { RBK_TEXRECT, RBM_XLU, 4, 320 },
    { RBK_TEXRECT, RBM_XLU_Z, 4, 320 },
    // the same pixels in more, smaller rectangles: the cost of each rectangle
    { RBK_TEXRECT, RBM_TEX16, 19, 64 },
    { RBK_TEXRECT, RBM_TEX16, 75, 32 },
    { RBK_TEXRECT, RBM_TEX16, 300, 16 },
    { RBK_TEXRECT, RBM_TEX16, 1200, 8 },
    // triangles: four sizes (the setup's cost against the fill's) in six modes
    { RBK_TRIS, RBM_SHADE, 500, 2 },
    { RBK_TRIS, RBM_SHADE, 300, 8 },
    { RBK_TRIS, RBM_SHADE, 100, 32 },
    { RBK_TRIS, RBM_SHADE, 8, 128 },
    { RBK_TRIS, RBM_SHADE_Z, 500, 2 },
    { RBK_TRIS, RBM_SHADE_Z, 300, 8 },
    { RBK_TRIS, RBM_SHADE_Z, 100, 32 },
    { RBK_TRIS, RBM_SHADE_Z, 8, 128 },
    { RBK_TRIS, RBM_TEX16_Z, 500, 2 },
    { RBK_TRIS, RBM_TEX16_Z, 300, 8 },
    { RBK_TRIS, RBM_TEX16_Z, 100, 32 },
    { RBK_TRIS, RBM_TEX16_Z, 8, 128 },
    { RBK_TRIS, RBM_2CYC_FOG, 500, 2 },
    { RBK_TRIS, RBM_2CYC_FOG, 300, 8 },
    { RBK_TRIS, RBM_2CYC_FOG, 100, 32 },
    { RBK_TRIS, RBM_2CYC_FOG, 8, 128 },
    { RBK_TRIS, RBM_AA_Z, 500, 2 },
    { RBK_TRIS, RBM_AA_Z, 300, 8 },
    { RBK_TRIS, RBM_AA_Z, 100, 32 },
    { RBK_TRIS, RBM_AA_Z, 8, 128 },
    { RBK_TRIS, RBM_XLU_Z, 500, 2 },
    { RBK_TRIS, RBM_XLU_Z, 300, 8 },
    { RBK_TRIS, RBM_XLU_Z, 100, 32 },
    { RBK_TRIS, RBM_XLU_Z, 8, 128 },
    // texture loads: 32x32 rectangles, the texture loaded before each (2 KB, then 4 KB, then a CI8 with its palette)
    { RBK_LOADS, RBM_TEX16, 50, 32 },
    { RBK_LOADS, RBM_TEX16, 200, 32 },
    { RBK_LOADS, RBM_TEX32, 200, 32 },
    { RBK_LOADS, RBM_CI8, 200, 32 },
    // syncs and render-mode switches between small rectangles (against 300 x 16 above)
    { RBK_SYNCS, RBM_TEX16, 300, 16 },
    { RBK_SWITCH, RBM_TEX16, 300, 16 },
};

#define RB_TESTS ARRAY_COUNT(sRdpTests)
#define RB_MAX_QUADS 500

static u32 sRdpTag; // the frame log's tag for this frame (test << 24 | rep << 16 | counted)
static s32 sRbFrame = -RB_WARMUP;
static s32 sRbSaid;
static s32 sRbBuiltTest = -1;
static u16* sRbTex16;  // RGBA16 32x32
static u32* sRbTex32;  // RGBA32 32x32
static u8* sRbTexCi8;  // CI8 32x32
static u16* sRbTlut;   // its 256 colours
static u8* sRbTexI4;   // I4 64x64
static Vtx* sRbVtx;    // the triangle grid (4 vertices a quad)
static Mtx sRbProj;
static Mtx sRbIdent;
static Vp sRbVp = { { { SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0 },
                      { SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0 } } };

static s32 RdpBench_Init(void) {
    s32 i;
    u32 r = 0x2545F491;

    if (sRbVtx != NULL) {
        return true;
    }
    sRbTex16 = SYSTEM_ARENA_MALLOC(32 * 32 * 2, "../fishmod_debug.c", __LINE__);
    sRbTex32 = SYSTEM_ARENA_MALLOC(32 * 32 * 4, "../fishmod_debug.c", __LINE__);
    sRbTexCi8 = SYSTEM_ARENA_MALLOC(32 * 32, "../fishmod_debug.c", __LINE__);
    sRbTlut = SYSTEM_ARENA_MALLOC(256 * 2, "../fishmod_debug.c", __LINE__);
    sRbTexI4 = SYSTEM_ARENA_MALLOC(64 * 64 / 2, "../fishmod_debug.c", __LINE__);
    sRbVtx = SYSTEM_ARENA_MALLOC(RB_MAX_QUADS * 4 * sizeof(Vtx), "../fishmod_debug.c", __LINE__);
    if ((sRbTex16 == NULL) || (sRbTex32 == NULL) || (sRbTexCi8 == NULL) || (sRbTlut == NULL) || (sRbTexI4 == NULL) ||
        (sRbVtx == NULL)) {
        sRbVtx = NULL;
        return false;
    }
    // noisy textures (no run of equal texels for the RDP to make light of)
    for (i = 0; i < 32 * 32; i++) {
        r = r * 1103515245 + 12345;
        sRbTex16[i] = (r >> 16) | 1;
        sRbTex32[i] = r | 0xFF;
        sRbTexCi8[i] = r >> 24;
    }
    for (i = 0; i < 256; i++) {
        r = r * 1103515245 + 12345;
        sRbTlut[i] = (r >> 16) | 1;
    }
    for (i = 0; i < 64 * 64 / 2; i++) {
        r = r * 1103515245 + 12345;
        sRbTexI4[i] = r >> 24;
    }
    osWritebackDCache(sRbTex16, 32 * 32 * 2);
    osWritebackDCache(sRbTex32, 32 * 32 * 4);
    osWritebackDCache(sRbTexCi8, 32 * 32);
    osWritebackDCache(sRbTlut, 256 * 2);
    osWritebackDCache(sRbTexI4, 64 * 64 / 2);
    // screen space: x right, y down, a vertex's z larger nearer
    guOrtho(&sRbProj, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, -4000.0f, 4000.0f, 1.0f);
    guMtxIdent(&sRbIdent);
    return true;
}

/** The test's quads, laid out in rows across the screen and round again (each one nearer than the last). */
static void RdpBench_BuildGrid(const RdpTest* t) {
    s32 perRow = (SCREEN_WIDTH - 8) / (t->size + 2);
    s32 rows = (SCREEN_HEIGHT - 8) / (t->size + 2);
    s32 q;

    if (perRow < 1) {
        perRow = 1;
    }
    if (rows < 1) {
        rows = 1;
    }
    for (q = 0; q < t->count; q++) {
        Vtx* v = &sRbVtx[q * 4];
        s32 cell = q % (perRow * rows);
        s32 x = 4 + (cell % perRow) * (t->size + 2);
        s32 y = 4 + (cell / perRow) * (t->size + 2);
        s32 z = q * 4 - 2000;
        s32 k;

        if (t->size >= 128) { // (the big ones overlap: fewer fit)
            x = 4 + (q % 2) * 160 + (q / 2) % 3 * 8;
            y = 4 + ((q / 2) % 2) * 100 + (q / 6) * 4;
        }
        for (k = 0; k < 4; k++) {
            s32 dx = (k == 1 || k == 2) ? t->size : 0;
            s32 dy = (k >= 2) ? t->size : 0;

            v[k].v.ob[0] = x + dx;
            v[k].v.ob[1] = y + dy;
            v[k].v.ob[2] = z;
            v[k].v.flag = 0;
            v[k].v.tc[0] = (dx ? 32 : 0) << 5;
            v[k].v.tc[1] = (dy ? 32 : 0) << 5;
            v[k].v.cn[0] = 0x40 + k * 0x30;
            v[k].v.cn[1] = 0xC0 - k * 0x20;
            v[k].v.cn[2] = 0x80 + (q & 0x3F);
            v[k].v.cn[3] = 0x80;
        }
    }
    osWritebackDCache(sRbVtx, t->count * 4 * sizeof(Vtx));
}

static Gfx* RdpBench_LoadTexture(Gfx* gfx, s32 mode) {
    switch (mode) {
        case RBM_CI8:
            gDPSetTextureLUT(gfx++, G_TT_RGBA16);
            gDPLoadTLUT_pal256(gfx++, sRbTlut);
            gDPLoadTextureBlock(gfx++, sRbTexCi8, G_IM_FMT_CI, G_IM_SIZ_8b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, 5, 5,
                                G_TX_NOLOD, G_TX_NOLOD);
            break;
        case RBM_I4:
            gDPSetTextureLUT(gfx++, G_TT_NONE);
            gDPLoadTextureBlock_4b(gfx++, sRbTexI4, G_IM_FMT_I, 64, 64, 0, G_TX_WRAP, G_TX_WRAP, 6, 6, G_TX_NOLOD,
                                   G_TX_NOLOD);
            break;
        case RBM_TEX32:
            gDPSetTextureLUT(gfx++, G_TT_NONE);
            gDPLoadTextureBlock(gfx++, sRbTex32, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, 5, 5,
                                G_TX_NOLOD, G_TX_NOLOD);
            break;
        default:
            gDPSetTextureLUT(gfx++, G_TT_NONE);
            gDPLoadTextureBlock(gfx++, sRbTex16, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, 5, 5,
                                G_TX_NOLOD, G_TX_NOLOD);
            break;
    }
    return gfx;
}

/** The render state for a mode (rect: for texture rectangles: no shade, the colour from the primitive colour). */
static Gfx* RdpBench_SetMode(Gfx* gfx, s32 mode, s32 rect) {
    s32 textured = (mode != RBM_SHADE) && (mode != RBM_SHADE_Z);

    gDPPipeSync(gfx++);
    gDPSetPrimColor(gfx++, 0, 0, 0xC0, 0xA0, 0x80, 0x80);
    gDPSetEnvColor(gfx++, 0x80, 0x80, 0x80, 0x80);
    gDPSetFogColor(gfx++, 0x40, 0x60, 0x80, 0xFF);
    gDPSetTexturePersp(gfx++, rect ? G_TP_NONE : G_TP_PERSP);
    gDPSetTextureFilter(gfx++, (mode == RBM_TEX16P) ? G_TF_POINT : G_TF_BILERP);
    gDPSetTextureDetail(gfx++, G_TD_CLAMP);
    gDPSetTextureLOD(gfx++, G_TL_TILE);
    gDPSetTextureConvert(gfx++, G_TC_FILT);
    gDPSetAlphaCompare(gfx++, G_AC_NONE);
    gDPSetDepthSource(gfx++, rect ? G_ZS_PRIM : G_ZS_PIXEL);
    gDPSetPrimDepth(gfx++, 0, 0);
    gDPSetColorDither(gfx++, G_CD_DISABLE);
    gDPSetAlphaDither(gfx++, G_AD_DISABLE);
    if ((mode == RBM_2CYC) || (mode == RBM_2CYC_FOG)) {
        gDPSetCycleType(gfx++, G_CYC_2CYCLE);
        if (rect) {
            gDPSetCombineMode(gfx++, G_CC_MODULATERGBA_PRIM, G_CC_PASS2);
        } else {
            gDPSetCombineMode(gfx++, G_CC_MODULATERGBA, G_CC_PASS2);
        }
        if (mode == RBM_2CYC_FOG) {
            gDPSetRenderMode(gfx++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);
        } else {
            gDPSetRenderMode(gfx++, G_RM_PASS, G_RM_ZB_OPA_SURF2);
        }
    } else {
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        if (!textured) {
            if (rect) {
                gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
            } else {
                gDPSetCombineMode(gfx++, G_CC_SHADE, G_CC_SHADE);
            }
        } else if (rect) {
            gDPSetCombineMode(gfx++, G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM);
        } else {
            gDPSetCombineMode(gfx++, G_CC_MODULATERGBA, G_CC_MODULATERGBA);
        }
        switch (mode) {
            case RBM_SHADE_Z:
            case RBM_TEX16_Z:
                gDPSetRenderMode(gfx++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);
                break;
            case RBM_XLU:
                gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
                break;
            case RBM_XLU_Z:
                gDPSetRenderMode(gfx++, G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2);
                break;
            case RBM_AA_Z:
                gDPSetRenderMode(gfx++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
                break;
            default:
                gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
                break;
        }
    }
    if (textured) {
        gfx = RdpBench_LoadTexture(gfx, mode);
    }
    return gfx;
}

static Gfx* RdpBench_Rect(Gfx* gfx, s32 x, s32 y, s32 size, s32 mode) {
    s32 w = (size >= 320) ? SCREEN_WIDTH : size;
    s32 h = (size >= 320) ? SCREEN_HEIGHT : size;

    gSPTextureRectangle(gfx++, x << 2, y << 2, (x + w) << 2, (y + h) << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    return gfx;
}

/** Draws this frame's test (after the frame setup) and says so: Play_Draw then skips the game's own drawing. */
s32 Fishmod_RdpBenchDraw(PlayState* play) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    s32 test;
    s32 rep;
    s32 inTest;
    const RdpTest* t;
    s32 i;
    char line[96];

    sRdpTag = 0;
    if (sRbFrame < 0) {
        sRbFrame++;
        return false;
    }
    if (!RdpBench_Init()) {
        return false;
    }
    if (!sRbSaid) {
        // each test's description, for fishmod/tools/rdpbench.py (from the debug log)
        sRbSaid = true;
        for (i = 0; i < (s32)RB_TESTS; i++) {
            t = &sRdpTests[i];
            sprintf(line, "RDPTEST %d %d %d %d %d %s\n", i, t->kind, t->mode, t->count, t->size,
                    (t->kind == RBK_FILL || t->kind == RBK_EMPTY) ? "-" : sRbModeNames[t->mode]);
            Sc64_UsbText(line);
        }
    }
    test = (sRbFrame / RB_FRAMES) % RB_TESTS;
    rep = sRbFrame / (RB_FRAMES * RB_TESTS);
    inTest = sRbFrame % RB_FRAMES;
    if (rep >= RB_REPS) {
        gFishmodBenchDone = true; // (the profiler sends its last counts, then FISHMOD-DONE)
        return false;
    }
    sRbFrame++;
    sRdpTag = ((u32)test << 24) | ((u32)rep << 16) | ((inTest >= RB_SETTLE) ? 1 : 0);
    t = &sRdpTests[test];
    if ((t->kind == RBK_TRIS) && (sRbBuiltTest != test)) {
        RdpBench_BuildGrid(t);
        sRbBuiltTest = test;
    }

    OPEN_DISPS(gfxCtx, "../fishmod_debug_rdpbench.inc.c", __LINE__);
    {
        Gfx* gfx = POLY_OPA_DISP;

        gDPPipeSync(gfx++);
        gDPSetScissor(gfx++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        switch (t->kind) {
            case RBK_EMPTY:
                break;

            case RBK_FILL:
                gDPSetCycleType(gfx++, G_CYC_FILL);
                gDPSetRenderMode(gfx++, G_RM_NOOP, G_RM_NOOP2);
                for (i = 0; i < t->count; i++) {
                    gDPSetFillColor(gfx++, (GPACK_RGBA5551(i * 40, 0x80, 0xC0, 1) << 16) |
                                               GPACK_RGBA5551(i * 40, 0x80, 0xC0, 1));
                    gDPFillRectangle(gfx++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
                    gDPPipeSync(gfx++);
                }
                break;

            case RBK_TEXRECT:
            case RBK_LOADS:
            case RBK_SYNCS:
            case RBK_SWITCH: {
                s32 perRow = (t->size >= 320) ? 1 : (SCREEN_WIDTH / t->size);
                s32 rows = (t->size >= 320) ? 1 : (SCREEN_HEIGHT / t->size);

                gfx = RdpBench_SetMode(gfx, t->mode, true);
                for (i = 0; i < t->count; i++) {
                    s32 cell = i % (perRow * rows);

                    if ((t->mode == RBM_TEX16_Z) || (t->mode == RBM_XLU_Z)) {
                        gDPPipeSync(gfx++);
                        gDPSetPrimDepth(gfx++, 0x7FFF - i * 16, 0); // (each one nearer)
                    }
                    if (t->kind == RBK_LOADS) {
                        gDPPipeSync(gfx++);
                        gfx = RdpBench_LoadTexture(gfx, t->mode);
                    } else if (t->kind == RBK_SYNCS) {
                        gDPPipeSync(gfx++);
                    } else if (t->kind == RBK_SWITCH) {
                        gDPPipeSync(gfx++);
                        if (i & 1) {
                            gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
                        } else {
                            gDPSetRenderMode(gfx++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
                        }
                    }
                    gfx = RdpBench_Rect(gfx, (cell % perRow) * t->size, (cell / perRow) * t->size, t->size, t->mode);
                }
                break;
            }

            case RBK_TRIS: {
                u32 geo = G_SHADE | G_SHADING_SMOOTH;

                if ((t->mode == RBM_SHADE_Z) || (t->mode == RBM_TEX16_Z) || (t->mode == RBM_2CYC_FOG) ||
                    (t->mode == RBM_AA_Z) || (t->mode == RBM_XLU_Z)) {
                    geo |= G_ZBUFFER;
                }
                if (t->mode == RBM_2CYC_FOG) {
                    geo |= G_FOG;
                }
                gSPViewport(gfx++, &sRbVp);
                gSPMatrix(gfx++, &sRbProj, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
                gSPMatrix(gfx++, &sRbIdent, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
                gSPLoadGeometryMode(gfx++, geo);
                gSPFogPosition(gfx++, 0, 1000);
                gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE,
                           ((t->mode == RBM_SHADE) || (t->mode == RBM_SHADE_Z)) ? G_OFF : G_ON);
                gfx = RdpBench_SetMode(gfx, t->mode, false);
                for (i = 0; i < t->count; i += 8) {
                    s32 n = MIN(8, t->count - i);
                    s32 k;

                    gSPVertex(gfx++, &sRbVtx[i * 4], n * 4, 0);
                    for (k = 0; k < n; k++) {
                        gSP2Triangles(gfx++, k * 4, k * 4 + 1, k * 4 + 2, 0, k * 4, k * 4 + 2, k * 4 + 3, 0);
                    }
                }
                break;
            }
        }
        gDPPipeSync(gfx++);
        POLY_OPA_DISP = gfx;
    }
    CLOSE_DISPS(gfxCtx, "../fishmod_debug_rdpbench.inc.c", __LINE__);
    return true;
}
#else
s32 Fishmod_RdpBenchDraw(struct PlayState* play) {
    return false;
}
#endif
