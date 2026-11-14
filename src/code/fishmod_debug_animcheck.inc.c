// Part of fishmod_debug.c (#included there, sharing its statics): the animation check (FISHMOD_ANIMCHECK;
// fishmod/docs/anim-verify.md, fishmod/scripts/animcheck.inc.c).
/*
 * Link held still (his own update skipped: Player_Update asks Fishmod_AnimCheckPlayerUpdate first) and posed by the
 * harness, one case a frame: an animation and a frame of it, loaded through the game's own path (the anim task queue,
 * as LinkAnimation_* does), and the variants (blends between two animations, every model group, faces, tunics and
 * boots, the far level of detail). His drawing is bracketed in the actor draw loop (z_actor.c) and, when it's done,
 * walked here as the RSP would walk it, every call followed: each triangle hashed with its three vertices (their
 * bytes, the matrix and the RSP state they were loaded under) and the RDP state it's drawn with (the texture memory as
 * what was loaded into it, not where from). Commands that change nothing change no hash, so a cleaned list hashes as
 * the original; anything drawn differently does not. Addresses are never hashed, only the bytes they lead to.
 *
 * Per case, a record of 8 words in gFishmodAnimLog (fishmod/tools/collog.sh --anim saves it; animcheck_compare.py
 * compares two): the case, the pose's hash (the joint table and the limb matrices: level 1), the opaque and the
 * translucent draw streams' hashes (level 2), the triangles in each, the commands walked, flags (bit 0: a walk went
 * astray), the frame.
 */

#ifdef FISHMOD_ANIMCHECK
#include "src/fishmod_data/fishmod_animcheck_cases.h"
#ifdef FISHMOD_ANIMCHECK_SELFTEST
extern Vtx gLinkChildHeadNearVtx[];
extern Gfx gLinkChildHeadNearDL[];
#endif

// level 3 (FISHMOD_ANIMCHECK_PIX): each case drawn from 8 camera angles in turn, one a frame, and each picture's
// frame buffer hashed in the ROM once the RDP has finished it (two frames later): 8 more words a record
#ifdef FISHMOD_ANIMCHECK_PIX
#define AC_ANGLES 8
#define AC_REC_WORDS 16
#else
#define AC_ANGLES 1
#define AC_REC_WORDS 8
#endif
#define AC_MAX_CASES 24000
// a share of the cases (several emulators running one each): the cases numbered FROM to TO-1, in order
#ifndef FISHMOD_ANIMCHECK_FROM
#define FISHMOD_ANIMCHECK_FROM 0
#endif
#ifndef FISHMOD_ANIMCHECK_TO
#define FISHMOD_ANIMCHECK_TO 0x7FFFFFFF
#endif

u32* gFishmodAnimLog;
u32 gFishmodAnimLogCount;
u32 gFishmodAnimDone;

static u32 Ac_Mix(u32 h, u32 w) {
    h ^= w;
    h *= 0x01000193;
    return h ^ (h >> 15);
}

static u32 Ac_Bytes(u32 h, const u8* p, s32 n) {
    s32 i;

    for (i = 0; i < n; i++) {
        h = Ac_Mix(h, p[i]);
    }
    return h;
}

// ---- the walker ----

typedef struct AcWalk {
    u32 seg[16];
    u32 mv[12];
    s32 mvTop;
    u32 proj;
    u32 geom;
    u32 omH;
    u32 omL;
    u32 tex[2];       // G_TEXTURE
    u32 rdp[18];      // combine 2, prim 2, env, fog, blend, fill, primdepth, scissor 2, convert 2, keyr, keygb 2, misc, zimg
    u32 tile[8][4];   // settile w0 w1, settilesize w0 w1
    u32 timg[2];      // the image's format word, its address (physical)
    u16 tmemKey[32];
    u32 tmemSig[32];
    s32 tmemCount;
    u8 light[256];
    u32 numLights;
    u32 mwKey[24];
    u32 mwVal[24];
    s32 mwCount;
    u32 mem[16];
    u32 slot[64];
    u32 rspHash;
    u32 rdpHash;
    u8 rspDirty;
    u8 rdpDirty;
    u32 drawHash;
    u32 draws;
    u32 cmds;
    u32 flags;
    u32 mtxSeg;       // where segment 0x0D was first pointed (the limb matrices: level 1)
} AcWalk;

static AcWalk sAcWalk;

static u32 Ac_Resolve(AcWalk* w, u32 a) {
    return (w->seg[(a >> 24) & 0xF] + (a & 0xFFFFFF)) & 0x1FFFFFFF;
}

static const u8* Ac_Ptr(AcWalk* w, u32 phys, u32 len) {
    if ((phys + len) > 0x800000) {
        w->flags |= 1;
        return NULL;
    }
    return (const u8*)(0x80000000 | phys);
}

static u32 Ac_RspHash(AcWalk* w) {
    if (w->rspDirty) {
        u32 h = Ac_Mix(Ac_Mix(Ac_Mix(0x811C9DC5, w->mv[w->mvTop]), w->proj), w->geom);
        s32 i;
        s32 n = 0x10 + w->numLights * 16 + 8;

        h = Ac_Mix(Ac_Mix(h, w->tex[0]), w->tex[1]);
        h = Ac_Mix(h, w->numLights);
        h = Ac_Bytes(h, w->light, (n > 256) ? 256 : n);
        for (i = 0; i < w->mwCount; i++) {
            h = Ac_Mix(Ac_Mix(h, w->mwKey[i]), w->mwVal[i]);
        }
        for (i = 0; i < 16; i++) {
            h = Ac_Mix(h, w->mem[i]);
        }
        w->rspHash = h;
        w->rspDirty = false;
    }
    return w->rspHash;
}

static u32 Ac_TmemSig(AcWalk* w, u32 key) {
    s32 i;

    for (i = 0; i < w->tmemCount; i++) {
        if (w->tmemKey[i] == key) {
            return w->tmemSig[i];
        }
    }
    return 0;
}

static u32 Ac_RdpHash(AcWalk* w) {
    if (w->rdpDirty) {
        u32 h = 0x811C9DC5;
        s32 i;

        h = Ac_Mix(Ac_Mix(h, w->omH), w->omL);
        for (i = 0; i < ARRAY_COUNT(w->rdp); i++) {
            h = Ac_Mix(h, w->rdp[i]);
        }
        for (i = 0; i < 8; i++) {
            h = Ac_Mix(Ac_Mix(Ac_Mix(Ac_Mix(h, w->tile[i][0]), w->tile[i][1]), w->tile[i][2]), w->tile[i][3]);
            h = Ac_Mix(h, Ac_TmemSig(w, w->tile[i][0] & 0x1FF));
        }
        // the palettes (the upper half of texture memory)
        for (i = 0; i < w->tmemCount; i++) {
            if (w->tmemKey[i] >= 0x100) {
                h = Ac_Mix(Ac_Mix(h, w->tmemKey[i]), w->tmemSig[i]);
            }
        }
        h = Ac_Mix(Ac_Mix(h, w->tex[0]), w->tex[1]);
        w->rdpHash = h;
        w->rdpDirty = false;
    }
    return w->rdpHash;
}

static void Ac_Draw(AcWalk* w, u32 a, u32 b, u32 c) {
    u32 h = Ac_Mix(Ac_Mix(Ac_RdpHash(w), w->geom), w->slot[a & 63]);

    h = Ac_Mix(Ac_Mix(h, w->slot[b & 63]), w->slot[c & 63]);
    w->drawHash = Ac_Mix(w->drawHash, h);
    w->draws++;
}

static void Ac_Load(AcWalk* w, u32 op, u32 w0, u32 w1) {
    s32 t = (w1 >> 24) & 7;
    u32 siz = (w->timg[0] >> 19) & 3;
    u32 width = (w->timg[0] & 0xFFF) + 1;
    u32 h = Ac_Mix(Ac_Mix(Ac_Mix(0x811C9DC5, w0), w1 & ~(7 << 24)), w->timg[0]);
    const u8* p;
    s32 i;

    h = Ac_Mix(Ac_Mix(h, w->tile[t][0]), w->tile[t][1]);
    if (op == G_LOADBLOCK) {
        u32 uls = (w0 >> 12) & 0xFFF;
        u32 ult = w0 & 0xFFF;
        u32 bytes = ((((w1 >> 12) & 0xFFF) + 1) << siz) >> 1;
        u32 start = w->timg[1] + ((((ult * width) + uls) << siz) >> 1);

        bytes = (bytes > 4096) ? 4096 : bytes;
        if ((p = Ac_Ptr(w, start, bytes)) != NULL) {
            h = Ac_Bytes(h, p, bytes);
        }
    } else if (op == G_LOADTILE) {
        u32 uls = ((w0 >> 12) & 0xFFF) >> 2;
        u32 ult = (w0 & 0xFFF) >> 2;
        u32 lrs = ((w1 >> 12) & 0xFFF) >> 2;
        u32 lrt = (w1 & 0xFFF) >> 2;
        u32 row = ((lrs - uls + 1) << siz) >> 1;
        u32 stride = (width << siz) >> 1;
        u32 r;

        for (r = ult; (r <= lrt) && (r < ult + 256); r++) {
            if ((p = Ac_Ptr(w, w->timg[1] + r * stride + ((uls << siz) >> 1), row)) != NULL) {
                h = Ac_Bytes(h, p, (row > 1024) ? 1024 : row);
            }
        }
    } else { // G_LOADTLUT
        u32 count = (((w1 >> 12) & 0xFFF) >> 2) + 1;
        u32 start = w->timg[1] + (((w0 >> 12) & 0xFFF) >> 2) * 2;

        if ((p = Ac_Ptr(w, start, count * 2)) != NULL) {
            h = Ac_Bytes(h, p, (count > 256) ? 512 : count * 2);
        }
    }
    {
        u32 key = w->tile[t][0] & 0x1FF;

        for (i = 0; i < w->tmemCount; i++) {
            if (w->tmemKey[i] == key) {
                break;
            }
        }
        if (i == w->tmemCount) {
            if (w->tmemCount < ARRAY_COUNT(w->tmemKey)) {
                w->tmemCount++;
            } else {
                i = 0; // (full: the oldest goes)
            }
        }
        w->tmemKey[i] = key;
        w->tmemSig[i] = h;
    }
    w->rdpDirty = true;
}

static void Ac_MoveWord(AcWalk* w, u32 index, u32 offset, u32 value) {
    s32 i;
    u32 key = (index << 16) | offset;

    if (index == G_MW_NUMLIGHT) {
        w->numLights = value / 0x10;
    }
    for (i = 0; i < w->mwCount; i++) {
        if (w->mwKey[i] == key) {
            break;
        }
    }
    if (i == w->mwCount) {
        if (w->mwCount >= ARRAY_COUNT(w->mwKey)) {
            w->flags |= 1;
            return;
        }
        w->mwCount++;
    }
    w->mwKey[i] = key;
    w->mwVal[i] = value;
    w->rspDirty = true;
}

static void Ac_Walk(AcWalk* w, Gfx* start, Gfx* end) {
    u32 stack[18];
    s32 depth = 0;
    u32 pc = (u32)start & 0x1FFFFFFF;
    u32 top = (u32)end & 0x1FFFFFFF;
    s32 i;

    for (i = 0; i < 16; i++) {
        w->seg[i] = gSegments[i] & 0x1FFFFFFF;
    }
    w->mvTop = 0;
    w->rspDirty = w->rdpDirty = true;
    while (w->cmds < 200000) {
        const u32* c;
        u32 w0;
        u32 w1;
        u32 op;

        if ((depth == 0) && (pc >= top)) {
            return;
        }
        if ((c = (const u32*)Ac_Ptr(w, pc, 8)) == NULL) {
            return;
        }
        w0 = c[0];
        w1 = c[1];
        op = w0 >> 24;
        pc += 8;
        w->cmds++;
        switch (op) {
            case G_DL:
                if (!((w0 >> 16) & 1)) {
                    if (depth >= ARRAY_COUNT(stack)) {
                        w->flags |= 1;
                        return;
                    }
                    stack[depth++] = pc;
                }
                pc = Ac_Resolve(w, w1);
                break;
            case G_ENDDL:
                if (depth == 0) {
                    return;
                }
                pc = stack[--depth];
                break;
            case G_RELSEGMENT:
            case G_MOVEWORD:
                if (((w0 >> 16) & 0xFF) == G_MW_SEGMENT) {
                    s32 s = ((w0 & 0xFFFF) / 4) & 0xF;

                    w->seg[s] = (op == G_RELSEGMENT) ? Ac_Resolve(w, w1) : (w1 & 0x1FFFFFFF);
                    if ((s == 0xD) && (w->mtxSeg == 0)) {
                        w->mtxSeg = w->seg[s];
                    }
                } else {
                    Ac_MoveWord(w, (w0 >> 16) & 0xFF, w0 & 0xFFFF, w1);
                }
                break;
            case G_MTX: {
                u32 flags = (w0 & 0xFF) ^ (G_MTX_PUSH | G_MTX_LOAD);
                const u8* m = Ac_Ptr(w, Ac_Resolve(w, w1), 64);
                u32 h = (m != NULL) ? Ac_Bytes(0x811C9DC5, m, 64) : 0;

                if (flags & G_MTX_VIEWPROJECTION) {
                    w->proj = (flags & G_MTX_LOAD) ? h : Ac_Mix(w->proj, h);
                } else {
                    if ((flags & G_MTX_PUSH) && (w->mvTop < ARRAY_COUNT(w->mv) - 1)) {
                        w->mv[w->mvTop + 1] = w->mv[w->mvTop];
                        w->mvTop++;
                    }
                    w->mv[w->mvTop] = (flags & G_MTX_LOAD) ? h : Ac_Mix(w->mv[w->mvTop], h);
                }
                w->rspDirty = true;
                break;
            }
            case G_POPMTX: {
                u32 n = w1 / 64;

                w->mvTop = (n > (u32)w->mvTop) ? 0 : (w->mvTop - n);
                w->rspDirty = true;
                break;
            }
            case G_MOVEMEM: {
                u32 len = ((w0 >> 19) & 0x1F) * 8 + 8;
                u32 ofs = ((w0 >> 8) & 0xFF) * 8;
                u32 idx = w0 & 0xFF;
                const u8* p = Ac_Ptr(w, Ac_Resolve(w, w1), len);

                if (p == NULL) {
                    break;
                }
                if (idx == G_MV_LIGHT) {
                    u32 k;

                    // the lights' bytes, without what isn't cleared each frame (pads, and the ambient light's type)
                    for (k = 0; (k < len) && (ofs + k < 256); k++) {
                        u32 at = ofs + k;
                        u32 pos = (at - 0x10) & 15;
                        u8 v = p[k];

                        if ((at >= 0x10) && ((pos == 7) || (pos >= 11) || ((pos == 3) && (k >= (len & ~15)))))
                        {
                            v = 0;
                        }
                        w->light[at] = v;
                    }
                } else {
                    w->mem[idx & 15] = Ac_Mix(Ac_Bytes(0x811C9DC5, p, len), ofs);
                }
                w->rspDirty = true;
                break;
            }
            case G_GEOMETRYMODE:
                w->geom = (w->geom & (w0 | 0xFF000000)) | w1;
                w->rspDirty = true;
                break;
            case G_TEXTURE:
                w->tex[0] = w0 & 0xFFFFFF;
                w->tex[1] = w1;
                w->rspDirty = w->rdpDirty = true;
                break;
            case G_VTX: {
                u32 n = (w0 >> 12) & 0xFF;
                u32 v0 = ((w0 >> 1) & 0x7F) - n;
                const u8* p = Ac_Ptr(w, Ac_Resolve(w, w1), n * 16);
                u32 rh = Ac_RspHash(w);
                u32 k;

                if (p == NULL) {
                    break;
                }
                for (k = 0; k < n; k++) {
                    w->slot[(v0 + k) & 63] = Ac_Bytes(rh, p + k * 16, 16);
                }
                break;
            }
            case G_MODIFYVTX:
                w->slot[((w0 & 0xFFFF) / 2) & 63] = Ac_Mix(Ac_Mix(w->slot[((w0 & 0xFFFF) / 2) & 63], w0), w1);
                break;
            case G_TRI1:
                Ac_Draw(w, ((w0 >> 16) & 0xFF) / 2, ((w0 >> 8) & 0xFF) / 2, (w0 & 0xFF) / 2);
                break;
            case G_TRI2:
            case G_QUAD:
                Ac_Draw(w, ((w0 >> 16) & 0xFF) / 2, ((w0 >> 8) & 0xFF) / 2, (w0 & 0xFF) / 2);
                Ac_Draw(w, ((w1 >> 16) & 0xFF) / 2, ((w1 >> 8) & 0xFF) / 2, (w1 & 0xFF) / 2);
                break;
            case G_TRISNAKE: {
                u32 h = Ac_Mix(Ac_Mix(Ac_RdpHash(w), w0), w1);

                for (i = 0; i < 64; i++) {
                    h = Ac_Mix(h, w->slot[i]);
                }
                w->drawHash = Ac_Mix(w->drawHash, h);
                w->draws++;
                break;
            }
            case G_TEXRECT:
            case G_TEXRECTFLIP:
            case G_FILLRECT:
            case G_RDPHALF_1:
            case G_RDPHALF_2:
            case G_BRANCH_WZ:
            case G_MEMSET:
            case G_LOAD_UCODE:
                // (drawn by its words, or a branch not followed: its words go in the stream)
                w->drawHash = Ac_Mix(Ac_Mix(Ac_Mix(w->drawHash, Ac_RdpHash(w)), w0), w1);
                break;
            case G_LIGHTTORDP:
                w->rdp[16] = Ac_Mix(Ac_Mix(Ac_Mix(0, w0), w1), Ac_RspHash(w));
                w->rdpDirty = true;
                break;
            case G_SETOTHERMODE_L:
            case G_SETOTHERMODE_H: {
                u32 len = (w0 & 0xFF) + 1;
                u32 sft = 32 - ((w0 >> 8) & 0xFF) - len;
                u32 mask = ((len >= 32) ? 0xFFFFFFFF : ((1u << len) - 1)) << sft;

                if (op == G_SETOTHERMODE_L) {
                    w->omL = (w->omL & ~mask) | (w1 & mask);
                } else {
                    w->omH = (w->omH & ~mask) | (w1 & mask);
                }
                w->rdpDirty = true;
                break;
            }
            case G_RDPSETOTHERMODE:
                w->omH = w0 & 0xFFFFFF;
                w->omL = w1;
                w->rdpDirty = true;
                break;
            case G_SETCOMBINE:
                w->rdp[0] = w0 & 0xFFFFFF;
                w->rdp[1] = w1;
                w->rdpDirty = true;
                break;
            case G_SETPRIMCOLOR:
                w->rdp[2] = w0 & 0xFFFFFF;
                w->rdp[3] = w1;
                w->rdpDirty = true;
                break;
            case G_SETENVCOLOR:
                w->rdp[4] = w1;
                w->rdpDirty = true;
                break;
            case G_SETFOGCOLOR:
                w->rdp[5] = w1;
                w->rdpDirty = true;
                break;
            case G_SETBLENDCOLOR:
                w->rdp[6] = w1;
                w->rdpDirty = true;
                break;
            case G_SETFILLCOLOR:
                w->rdp[7] = w1;
                w->rdpDirty = true;
                break;
            case G_SETPRIMDEPTH:
                w->rdp[8] = w1;
                w->rdpDirty = true;
                break;
            case G_SETSCISSOR:
                w->rdp[9] = w0 & 0xFFFFFF;
                w->rdp[10] = w1;
                w->rdpDirty = true;
                break;
            case G_SETCONVERT:
                w->rdp[11] = w0 & 0xFFFFFF;
                w->rdp[12] = w1;
                w->rdpDirty = true;
                break;
            case G_SETKEYR:
                w->rdp[13] = w1;
                w->rdpDirty = true;
                break;
            case G_SETKEYGB:
                w->rdp[14] = w0 & 0xFFFFFF;
                w->rdp[15] = w1;
                w->rdpDirty = true;
                break;
            case G_SETZIMG:
            case G_SETCIMG:
                w->rdp[17] = Ac_Mix(w->rdp[17], w0); // (the format, not the buffer's address)
                w->rdpDirty = true;
                break;
            case G_SETTILE:
                w->tile[(w1 >> 24) & 7][0] = w0 & 0xFFFFFF;
                w->tile[(w1 >> 24) & 7][1] = w1;
                w->rdpDirty = true;
                break;
            case G_SETTILESIZE:
                w->tile[(w1 >> 24) & 7][2] = w0 & 0xFFFFFF;
                w->tile[(w1 >> 24) & 7][3] = w1;
                w->rdpDirty = true;
                break;
            case G_SETTIMG:
                w->timg[0] = w0 & 0xFFFFFF;
                w->timg[1] = Ac_Resolve(w, w1);
                break;
            case G_LOADBLOCK:
            case G_LOADTILE:
            case G_LOADTLUT:
                Ac_Load(w, op, w0, w1);
                break;
            default:
                // syncs, no-ops, the RSP's culling (taken as drawn), flushes and DMAs: nothing drawn changes
                break;
        }
    }
    w->flags |= 1; // (ran on too long)
}

// ---- the cases ----

typedef enum AcPhase {
    AC_PHASE_FRAMES,  // every frame of every animation
    AC_PHASE_BLENDS,  // two animations' first frames blended (every 8th pair, three weights)
    AC_PHASE_GROUPS,  // every model group (what's in his hands) on every 16th animation's first frame
    AC_PHASE_FACES,   // every face on the first 8 animations
    AC_PHASE_TUNICS,  // every tunic and boots on every 64th animation
    AC_PHASE_FAR,     // the far level of detail: every 4th animation's first frame, seen from far off
    AC_PHASE_LOOK,    // the head and upper body turned by his code (looking at something, aiming): 90 angle sets on
                      // every 32nd animation's first frame
    AC_PHASE_DONE
} AcPhase;

static s32 sAcPhase;
static s32 sAcA;      // the animation (or the variant's index)
static s32 sAcF;      // the frame (or the variant's sub-index)
static s32 sAcSettle; // frames to wait (not logged) after a change the camera or the models need to take in
static s32 sAcCase;   // the case being drawn this frame (logged at its draw), -1: none
static s16 sAcCamId = SUB_CAM_ID_DONE;
static u8 sAcModelGroup;
static s8 sAcTunic;
static s8 sAcBoots;
static s16 sAcFace;
static Vec3s* sAcJoints;
static Gfx* sAcOpaEnd;
static Gfx* sAcXluEnd;
static s32 sAcAngle; // (level 3) the camera angle this frame
static s32 sAcSeq;   // the cases begun so far (for FISHMOD_ANIMCHECK_FROM/TO)
static s32 sAcRec;   // the record this case's pictures go in
static s32 sAcMax;   // the records the log holds

typedef struct AcPixJob {
    u16* fb;
    s32 rec;
    s32 angle;
    s32 frame;
} AcPixJob;
static AcPixJob sAcJobs[4];
static s32 sAcJobCount;

/** The pictures finished by now (drawn two frames ago or more): their frame buffers hashed into their records. */
static void Ac_PixelJobs(void) {
    s32 i = 0;

    while (i < sAcJobCount) {
        AcPixJob* j = &sAcJobs[i];

        if (sPlayFrames - j->frame >= 2) {
            u32 h = 0x811C9DC5;
            const u32* p = (const u32*)j->fb;
            s32 k;

            osInvalDCache(j->fb, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
            for (k = 0; k < SCREEN_WIDTH * SCREEN_HEIGHT / 2; k++) {
                h = Ac_Mix(h, p[k]);
            }
            gFishmodAnimLog[j->rec * AC_REC_WORDS + 8 + j->angle] = h;
            *j = sAcJobs[--sAcJobCount];
        } else {
            i++;
        }
    }
}

static s32 Ac_FrameCount(s32 a) {
    LinkAnimationHeader* h = SEGMENTED_TO_VIRTUAL(sAnimCheckLinkAnims[a]);

    return h->common.frameCount;
}

/** Moves to the next case; false when there are none left in this phase. */
static s32 Ac_Next(void) {
    switch (sAcPhase) {
        case AC_PHASE_FRAMES:
            if (++sAcF < Ac_FrameCount(sAcA)) {
                return true;
            }
            sAcF = 0;
            return ++sAcA < FISHMOD_ANIMCHECK_LINK_ANIMS;
        case AC_PHASE_BLENDS:
            if (++sAcF < 3) {
                return true;
            }
            sAcF = 0;
            sAcA += 8;
            return sAcA + 1 < FISHMOD_ANIMCHECK_LINK_ANIMS;
        case AC_PHASE_GROUPS:
            sAcF += 16;
            if (sAcF < FISHMOD_ANIMCHECK_LINK_ANIMS) {
                return true;
            }
            sAcF = 0;
            return ++sAcA < PLAYER_MODELGROUP_MAX;
        case AC_PHASE_FACES:
            if (++sAcF < 8) {
                return true;
            }
            sAcF = 0;
            return ++sAcA < 16;
        case AC_PHASE_TUNICS:
            sAcF += 64;
            if (sAcF < FISHMOD_ANIMCHECK_LINK_ANIMS) {
                return true;
            }
            sAcF = 0;
            return ++sAcA < 9;
        case AC_PHASE_FAR:
            sAcF += 4;
            return sAcF < FISHMOD_ANIMCHECK_LINK_ANIMS;
        case AC_PHASE_LOOK:
            sAcF += 32;
            if (sAcF < FISHMOD_ANIMCHECK_LINK_ANIMS) {
                return true;
            }
            sAcF = 0;
            return ++sAcA < 90;
        default:
            return false;
    }
}

static void Ac_Camera(PlayState* play, Player* player, f32 dist, s32 angle) {
    // (the angles: front, back, his left, his right, three-quarter front-left from above, three-quarter back-right
    // from above, from above, from below)
    static const s16 sYaw[8] = { 0, 0x8000, 0x4000, -0x4000, 0x2000, -0x6000, 0, 0x8000 };
    static const f32 sPitch[8] = { 0.15f, 0.15f, 0.15f, 0.15f, 0.5f, 0.5f, 1.45f, -1.2f };
    Vec3f at = player->actor.world.pos;
    Vec3f eye;
    s16 yaw = player->actor.shape.rot.y + sYaw[angle];
    f32 flat = dist * cosf(sPitch[angle]);

    at.y += LINK_IS_ADULT ? 40.0f : 25.0f;
    eye.x = at.x + Math_SinS(yaw) * flat;
    eye.y = at.y + dist * sinf(sPitch[angle]);
    eye.z = at.z + Math_CosS(yaw) * flat;
    Play_SetCameraAtEye(play, sAcCamId, &at, &eye);
    Play_SetCameraFov(play, sAcCamId, 60.0f);
}

/** The next case (the next phase from its start, with his default models back, when this one's done). */
static void Ac_Advance(Player* this) {
    sAcSeq++;
    if (!Ac_Next()) {
        sAcPhase++;
        sAcA = sAcF = 0;
        Player_SetModelGroup(this, sAcModelGroup);
        this->currentTunic = sAcTunic;
        this->currentBoots = sAcBoots;
        this->actor.shape.face = sAcFace;
        this->headLimbRot.x = this->headLimbRot.y = this->headLimbRot.z = 0;
        this->upperLimbRot.x = this->upperLimbRot.y = this->upperLimbRot.z = 0;
        sAcSettle = 10; // (the camera moves for the far phase)
    }
}

s32 Fishmod_AnimCheckPlayerUpdate(Player* this, PlayState* play) {
    s32 limbs = this->skelAnime.limbCount;
    s32 a;

    if (!gFishmodAnimCheck.active) {
        return false;
    }
    if (sAcCamId == SUB_CAM_ID_DONE) {
        // the first frame held: his own models and equipment as they are now are the default ones
        sAcCamId = Play_CreateSubCamera(play);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, sAcCamId, CAM_STAT_ACTIVE);
        sAcModelGroup = this->modelGroup;
        sAcTunic = this->currentTunic;
        sAcBoots = this->currentBoots;
        sAcFace = this->actor.shape.face;
        sAcJoints = this->skelAnime.jointTable;
        if (gFishmodAnimLog == NULL) {
            // as many records as its share of the cases (the system heap can't hold a whole pixel run's: 1.4 MB),
            // fewer if even that doesn't fit
            s32 n = FISHMOD_ANIMCHECK_TO - FISHMOD_ANIMCHECK_FROM;

            n = (n > AC_MAX_CASES) ? AC_MAX_CASES : n;
            while ((n > 0) && ((gFishmodAnimLog = SYSTEM_ARENA_MALLOC(n * AC_REC_WORDS * sizeof(u32),
                                                                     "../fishmod_debug.c", __LINE__)) == NULL)) {
                n /= 2;
            }
            sAcMax = n;
        }
        sAcPhase = AC_PHASE_FRAMES;
        sAcA = sAcF = 0;
        sAcSettle = 10;
#ifdef FISHMOD_ANIMCHECK_SELFTEST
        {
            // the harness's own test (child Link): 1, a change that shows (one texture coordinate of his head moved by
            // one step): every case drawing it must differ; 2, a change that doesn't (the head's first pipe sync made
            // a no-op): every case must match
            Vtx* v = SEGMENTED_TO_VIRTUAL(gLinkChildHeadNearVtx);
            Gfx* g = SEGMENTED_TO_VIRTUAL(gLinkChildHeadNearDL);

            if (FISHMOD_ANIMCHECK_SELFTEST == 1) {
                v[0].v.tc[0] += 1;
                osWritebackDCache(&v[0], sizeof(Vtx));
            } else {
                while ((g->words.w0 >> 24) != G_RDPPIPESYNC) {
                    g++;
                }
                g->words.w0 = G_NOOP << 24;
                osWritebackDCache(g, sizeof(Gfx));
            }
        }
#endif
        sAcAngle = 0;
        sAcSeq = 0;
        // (a share of the cases: the ones before it passed over at once)
        while ((sAcSeq < FISHMOD_ANIMCHECK_FROM) && (sAcPhase < AC_PHASE_DONE)) {
            Ac_Advance(this);
        }
    } else if (sAcSettle > 0) {
        sAcSettle--;
    } else if (sAcAngle + 1 < AC_ANGLES) {
        sAcAngle++; // (the same pose from the next angle)
    } else if (sAcPhase < AC_PHASE_DONE) {
        sAcAngle = 0;
        Ac_Advance(this);
        if (sAcSeq >= FISHMOD_ANIMCHECK_TO) {
            sAcPhase = AC_PHASE_DONE;
        }
    }
#ifdef FISHMOD_ANIMCHECK_PIX
    Ac_PixelJobs();
#endif
    gTimeSpeed = 0;
    Ac_Camera(play, this, (sAcPhase == AC_PHASE_FAR) ? 450.0f : 60.0f, sAcAngle);
    sAcCase = -1;
    if (sAcPhase >= AC_PHASE_DONE) {
        gFishmodAnimDone = (sAcJobCount == 0);
        return true;
    }
    if (sAcAngle != 0) {
        // (the pose stays from the angle-0 frame: nothing loaded)
        sAcCase = 0x0FFFFFFF;
        return true;
    }
    switch (sAcPhase) {
        case AC_PHASE_FRAMES:
            AnimTaskQueue_AddLoadPlayerFrame(play, sAnimCheckLinkAnims[sAcA], sAcF, limbs, sAcJoints);
            break;
        case AC_PHASE_BLENDS:
            AnimTaskQueue_AddLoadPlayerFrame(play, sAnimCheckLinkAnims[sAcA], 0, limbs, sAcJoints);
            AnimTaskQueue_AddLoadPlayerFrame(play, sAnimCheckLinkAnims[sAcA + 1], 0, limbs, this->skelAnime.morphTable);
            AnimTaskQueue_AddInterp(play, limbs, sAcJoints, this->skelAnime.morphTable, 0.25f * (sAcF + 1));
            break;
        case AC_PHASE_GROUPS:
            Player_SetModelGroup(this, sAcA);
            AnimTaskQueue_AddLoadPlayerFrame(play, sAnimCheckLinkAnims[sAcF], 0, limbs, sAcJoints);
            break;
        case AC_PHASE_FACES:
            this->actor.shape.face = sAcA;
            AnimTaskQueue_AddLoadPlayerFrame(play, sAnimCheckLinkAnims[sAcF], 0, limbs, sAcJoints);
            break;
        case AC_PHASE_TUNICS:
            this->currentTunic = sAcA % 3;
            this->currentBoots = LINK_IS_ADULT ? (sAcA / 3) : 0;
            AnimTaskQueue_AddLoadPlayerFrame(play, sAnimCheckLinkAnims[sAcF], 0, limbs, sAcJoints);
            break;
        case AC_PHASE_FAR:
            AnimTaskQueue_AddLoadPlayerFrame(play, sAnimCheckLinkAnims[sAcF], 0, limbs, sAcJoints);
            break;
        case AC_PHASE_LOOK: {
            // (variant a: head yaw 5 steps x head pitch 3 x upper body yaw 3 x upper body pitch 2)
            static const s16 sHeadYaw[5] = { -0x3000, -0x1500, 0, 0x1500, 0x3000 };
            static const s16 sPitch[3] = { -0x1000, 0, 0x1000 };
            static const s16 sUpperYaw[3] = { -0x2000, 0, 0x2000 };

            this->headLimbRot.y = sHeadYaw[sAcA % 5];
            this->headLimbRot.x = sPitch[(sAcA / 5) % 3];
            this->upperLimbRot.y = sUpperYaw[(sAcA / 15) % 3];
            this->upperLimbRot.x = ((sAcA / 45) & 1) ? 0x800 : 0;
            AnimTaskQueue_AddLoadPlayerFrame(play, sAnimCheckLinkAnims[sAcF], 0, limbs, sAcJoints);
            break;
        }
    }
    a = sAcA;
    if (sAcSettle == 0) {
        sAcCase = (sAcPhase << 28) | ((a & 0xFFF) << 16) | (sAcF & 0xFFFF);
    }
    return true;
}

void Fishmod_AnimCheckDrawBegin(PlayState* play) {
    gFishmodAnimCheck.opaStart = play->state.gfxCtx->polyOpa.p;
    gFishmodAnimCheck.xluStart = play->state.gfxCtx->polyXlu.p;
}

void Fishmod_AnimCheckDrawEnd(PlayState* play, Actor* actor) {
    Player* player = (Player*)actor;
    AcWalk* w = &sAcWalk;
    u32* rec;
    u32 pose;
    u32 opa;
    u32 xlu;
    u32 draws;
    u32 cmds;
    u32 flags;

    sAcOpaEnd = gFishmodAnimCheck.opaEnd = play->state.gfxCtx->polyOpa.p;
    sAcXluEnd = gFishmodAnimCheck.xluEnd = play->state.gfxCtx->polyXlu.p;
    if ((sAcCase < 0) || (gFishmodAnimLog == NULL) || (gFishmodAnimLogCount >= sAcMax)) {
        return;
    }
#ifdef FISHMOD_ANIMCHECK_PIX
    if (sAcJobCount < ARRAY_COUNT(sAcJobs)) {
        AcPixJob* j = &sAcJobs[sAcJobCount++];

        j->fb = play->state.gfxCtx->curFrameBuffer;
        j->rec = (sAcAngle == 0) ? gFishmodAnimLogCount : sAcRec;
        if ((sAcAngle != 0) && (sAcRec < 0)) {
            sAcJobCount--; // (its first angle wasn't logged)
        }
        j->angle = sAcAngle;
        j->frame = sPlayFrames;
    }
#endif
    if (sAcAngle != 0) {
        return; // (levels 1 and 2 once a case, from the first angle)
    }
    sAcRec = gFishmodAnimLogCount;
    // the draw streams (each list from a clean state: the same in both builds before Link's own commands)
    bzero(w, sizeof(*w));
    Ac_Walk(w, gFishmodAnimCheck.opaStart, sAcOpaEnd);
    opa = w->drawHash;
    draws = w->draws << 16;
    cmds = w->cmds;
    flags = w->flags;
    {
        u32 mtxSeg = w->mtxSeg;

        bzero(w, sizeof(*w));
        w->mtxSeg = mtxSeg;
    }
    Ac_Walk(w, gFishmodAnimCheck.xluStart, sAcXluEnd);
    xlu = w->drawHash;
    draws |= w->draws & 0xFFFF;
    cmds += w->cmds;
    flags |= w->flags;
    // the pose: the joint table (the face's byte with it) and the limb matrices
    pose = Ac_Bytes(0x811C9DC5, (const u8*)player->skelAnime.jointTable,
                    (player->skelAnime.limbCount + 1) * sizeof(Vec3s));
    if ((w->mtxSeg != 0) && (w->mtxSeg + player->skelAnime.dListCount * sizeof(Mtx) < 0x800000)) {
        pose = Ac_Bytes(pose, (const u8*)(0x80000000 | w->mtxSeg), player->skelAnime.dListCount * sizeof(Mtx));
    } else {
        flags |= 2; // (no limb matrices found)
    }
    rec = &gFishmodAnimLog[gFishmodAnimLogCount * AC_REC_WORDS];
    rec[0] = sAcCase;
    rec[1] = pose;
    rec[2] = opa;
    rec[3] = xlu;
    rec[4] = draws;
    rec[5] = cmds;
    rec[6] = flags;
    rec[7] = sPlayFrames;
#ifdef FISHMOD_ANIMCHECK_PIX
    bzero(&rec[8], 8 * sizeof(u32));
#endif
    gFishmodAnimLogCount++;
}

#else

s32 Fishmod_AnimCheckPlayerUpdate(Player* player, PlayState* play) {
    return false;
}

void Fishmod_AnimCheckDrawBegin(PlayState* play) {
}

void Fishmod_AnimCheckDrawEnd(PlayState* play, Actor* actor) {
}

#endif
