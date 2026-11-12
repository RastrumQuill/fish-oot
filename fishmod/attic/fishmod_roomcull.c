/*
 * NOT BUILT (kept for reference). Tried on Lake Hylia, November 2026: exact (F3DEX3's "tris drawn" count matched a
 * frames6 run on all 2,772 frames compared in ares), but a net loss there: the lake's room is only about 1,740
 * vertices in all, loaded 32 at a time, most of the triangles the RSP drops are back-facing rather than off screen,
 * and a box per 32-vertex run costs 8 vertices: 71 vertices a frame more, 67 triangles fewer. Might pay on a scene with
 * big room pieces whose runs sit off screen; it would go in src/code with the hook in Room_DrawCullable (z_room.c).
 */
/*
 * fish-oot: culling inside a room's big pieces (Room_DrawCullable). A cullable room's pieces each carry a bounding
 * sphere, and the game skips a piece only when its sphere is wholly too near or too far, so a piece spanning the whole
 * map (Lake Hylia's five largest are 3,400 to 5,500 units across) always goes to the RSP: every vertex transformed,
 * most of its triangles then dropped as off screen.
 *
 * Here each big piece's display list is rebuilt once, the first time it's drawn: every run of vertex loads and the
 * triangles drawn from them (between two state changes) becomes a display list of its own, called from the rebuilt
 * list, that first loads the eight corners of the run's box into vertex slots no room uses (48-55) and asks the RSP to
 * end it if they're all off screen (G_CULLDL). The RSP's own test, the same one it drops each triangle by: a run whose
 * box is off screen draws nothing either way, so the frame is the same, minus the work. Everything else (state
 * changes, calls to other lists) is copied as it was, in order.
 *
 * A run is left as it was (never culled) when a later triangle uses a vertex it loaded (skipping it would leave that
 * slot stale), when it loads into slots 48-55, or when it's small (the box costs eight vertices). A list's own cull
 * (the exporter put one on each piece's whole box) is kept as it was. A list holding anything this doesn't follow
 * (vertex edits, LOD branches, triangle snakes) is left whole.
 */
#include "ultra64.h"
#include "room.h"
#include "segmented_address.h"
#include "zelda_arena.h"
#include "libc64/malloc.h"
#include "array_count.h"
#include "alignment.h"
#include "fishmod.h"

#define ROOMCULL_MIN_RADIUS 1000 // pieces at least this large (their bounding sphere's radius) are rebuilt
#define ROOMCULL_MIN_VERTS 16    // runs loading fewer vertices than this aren't worth a box
#define ROOMCULL_SLOT 48         // the box's corners: slots 48-55 (F3DEX3 has 56; the game's rooms use the first 32)
#define ROOMCULL_MAX_CMDS 4096   // commands in a list rebuilt at most (longer: left whole)
#define ROOMCULL_MAX_RUNS 512
#define ROOMCULL_ENTRIES 64      // ROOM_SHAPE_CULLABLE_MAX_ENTRIES

typedef struct RoomCullCache {
    void* segment;                       // the room data it was made for (NULL: nothing kept)
    void* block[ROOMCULL_ENTRIES][2];    // each entry's rebuilt opa and xlu lists' memory (NULL: not rebuilt yet)
    Gfx* dl[ROOMCULL_ENTRIES][2];        // the list to draw (the original, if it couldn't be rebuilt)
    u8 made[ROOMCULL_ENTRIES][2];
} RoomCullCache;

static RoomCullCache sRoomCull[2]; // the current room and the previous one (RoomContext's two)
static Room* sRoomCullRoom[2];

#if FISHMOD_DEBUG
u32 gFishmodRoomCullRuns;   // runs given a box, summed over the lists rebuilt
u32 gFishmodRoomCullLists;  // lists rebuilt
#endif

static void* RoomCull_Resolve(u32 addr) {
    if ((addr >> 24) >= 0x80) {
        return (void*)addr;
    }
    return SEGMENTED_TO_VIRTUAL(addr);
}

/** The rebuilt list for one display list, in one allocation (*blockOut), or NULL to draw the original. */
static Gfx* RoomCull_Rebuild(Gfx* dl, void** blockOut) {
    static s16 sRunOf[ROOMCULL_MAX_CMDS];      // each command's run (-1: none)
    static u8 sRunOk[ROOMCULL_MAX_RUNS];       // the run can be culled
    static u16 sRunVerts[ROOMCULL_MAX_RUNS];   // vertices it loads
    static s16 sRunBox[ROOMCULL_MAX_RUNS][6];  // its box: min x y z, max x y z
    s16 slotRun[64];
    Gfx* src = RoomCull_Resolve((u32)dl);
    s32 n = 0;
    s32 runs = 0;
    s32 run = -1;
    s32 culled = 0;
    s32 i;
    s32 k;
    u8* mem;
    Gfx* out;
    Gfx* sub;
    Vtx* box;
    s32 size;

    for (k = 0; k < ARRAY_COUNT(slotRun); k++) {
        slotRun[k] = -1;
    }
    // the commands, runs and what each run's vertices are used by
    while (true) {
        u32 w0;
        u32 op;

        if (n >= ROOMCULL_MAX_CMDS) {
            return NULL;
        }
        w0 = src[n].words.w0;
        op = w0 >> 24;
        sRunOf[n] = -1;
        if (op == G_VTX) {
            u32 cnt = (w0 >> 12) & 0xFF;
            u32 end = (w0 >> 1) & 0x7F;
            u32 v0 = end - cnt;
            Vtx* v = RoomCull_Resolve(src[n].words.w1);

            if ((cnt == 0) || (end > 56)) {
                return NULL;
            }
            // (each load its own run: a load replaces its slots, so the triangles after it draw from it alone, unless
            // they reach back to an earlier load's slots, which then can't be culled)
            run = -1;
            if (run < 0) {
                if (runs >= ROOMCULL_MAX_RUNS) {
                    return NULL;
                }
                run = runs++;
                sRunOk[run] = true;
                sRunVerts[run] = 0;
                sRunBox[run][0] = sRunBox[run][1] = sRunBox[run][2] = 0x7FFF;
                sRunBox[run][3] = sRunBox[run][4] = sRunBox[run][5] = -0x8000;
            }
            if (end > ROOMCULL_SLOT) {
                sRunOk[run] = false; // (into the box's slots)
            }
            if ((src[n].words.w1 >> 24) != 0x03) {
                sRunOk[run] = false; // (not the room's own data, which never changes: its box could go stale)
            }
            for (k = v0; k < (s32)end; k++) {
                slotRun[k] = run;
            }
            for (k = 0; k < (s32)cnt; k++) {
                s32 c;

                for (c = 0; c < 3; c++) {
                    s16 x = v[k].v.ob[c];

                    if (x < sRunBox[run][c]) {
                        sRunBox[run][c] = x;
                    }
                    if (x > sRunBox[run][3 + c]) {
                        sRunBox[run][3 + c] = x;
                    }
                }
            }
            sRunVerts[run] += cnt;
            sRunOf[n] = run;
        } else if ((op == G_TRI1) || (op == G_TRI2) || (op == G_QUAD)) {
            u32 idx[6];
            s32 m = 3;

            idx[0] = ((w0 >> 16) & 0xFF) >> 1;
            idx[1] = ((w0 >> 8) & 0xFF) >> 1;
            idx[2] = (w0 & 0xFF) >> 1;
            if (op != G_TRI1) {
                u32 w1 = src[n].words.w1;

                idx[3] = ((w1 >> 16) & 0xFF) >> 1;
                idx[4] = ((w1 >> 8) & 0xFF) >> 1;
                idx[5] = (w1 & 0xFF) >> 1;
                m = 6;
            }
            for (k = 0; k < m; k++) {
                s32 owner = (idx[k] < ARRAY_COUNT(slotRun)) ? slotRun[idx[k]] : -1;

                // a vertex loaded by another run (or before any): that run must always run
                if ((owner >= 0) && (owner != run)) {
                    sRunOk[owner] = false;
                }
            }
            sRunOf[n] = run; // (outside a run: -1)
        } else if (op == G_CULLDL) {
            // (the list's own cull, on the whole piece's box: kept as it was; the vertices it tests must stay loaded)
            u32 first = (w0 & 0xFFFF) >> 1;
            u32 last = (src[n].words.w1 & 0xFFFF) >> 1;

            for (k = first; (k <= (s32)last) && (k < ARRAY_COUNT(slotRun)); k++) {
                if (slotRun[k] >= 0) {
                    sRunOk[slotRun[k]] = false;
                }
            }
            run = -1;
        } else if ((op == G_MODIFYVTX) || (op == G_BRANCH_WZ) || (op == G_TRISNAKE)) {
            return NULL; // (not followed here: the list is drawn as it was)
        } else {
            run = -1; // (any other command ends the run)
            if ((op == G_ENDDL) || ((op == G_DL) && (((w0 >> 16) & 0xFF) == G_DL_NOPUSH))) {
                n++;
                break; // (the end, or a branch: nothing after it runs)
            }
        }
        n++;
    }
    for (i = 0; i < runs; i++) {
        if (sRunVerts[i] < ROOMCULL_MIN_VERTS) {
            sRunOk[i] = false;
        }
        culled += sRunOk[i];
    }
    if (culled == 0) {
        return NULL;
    }
    // one block: the boxes' corners, the new list (at most n commands: a culled run becomes one call), then the
    // culled runs' lists (their commands, and three more each: the box's load, the cull, the end)
    size = culled * 8 * sizeof(Vtx) + (2 * n + 3 * culled) * sizeof(Gfx);
    mem = ZeldaArena_Malloc(size + 16);
    if (mem == NULL) {
        return NULL;
    }
    *blockOut = mem;
    box = (Vtx*)ALIGN16((uintptr_t)mem);
    out = (Gfx*)(box + culled * 8);
    sub = out + n;
    {
        Gfx* o = out;
        s32 b = 0;

        for (i = 0; i < n;) {
            s32 r = sRunOf[i];

            if ((r >= 0) && sRunOk[r] && ((i == 0) || (sRunOf[i - 1] != r))) {
                // the run's own list: its box's corners into slots 48-55, the RSP's cull, the run, the end
                Vtx* c = &box[b * 8];
                s32 j;

                for (j = 0; j < 8; j++) {
                    c[j].v.ob[0] = sRunBox[r][(j & 1) ? 3 : 0];
                    c[j].v.ob[1] = sRunBox[r][(j & 2) ? 4 : 1];
                    c[j].v.ob[2] = sRunBox[r][(j & 4) ? 5 : 2];
                    c[j].v.flag = 0;
                    c[j].v.tc[0] = c[j].v.tc[1] = 0;
                    c[j].v.cn[0] = c[j].v.cn[1] = c[j].v.cn[2] = 0;
                    c[j].v.cn[3] = 0xFF;
                }
                gSPDisplayList(o++, sub);
                gSPVertex(sub++, c, 8, ROOMCULL_SLOT);
                gSPCullDisplayList(sub++, ROOMCULL_SLOT, ROOMCULL_SLOT + 7);
                while ((i < n) && (sRunOf[i] == r)) {
                    *sub++ = src[i++];
                }
                gSPEndDisplayList(sub++);
                b++;
            } else {
                *o++ = src[i++];
            }
        }
    }
    osWritebackDCache(mem, size + 16);
#if FISHMOD_DEBUG
    gFishmodRoomCullRuns += culled;
    gFishmodRoomCullLists++;
#endif
    return out;
}

/** What to draw for a room's piece: its rebuilt list (made the first time), or the original. */
Gfx* Fishmod_RoomCullList(Room* room, s32 entry, s32 xlu, Gfx* dl, s16 radius) {
    RoomCullCache* cache;
    s32 c;

    if ((dl == NULL) || (radius < ROOMCULL_MIN_RADIUS) || (entry >= ROOMCULL_ENTRIES) || (osMemSize < 0x800000)) {
        return dl; // (small pieces as they were; without the Expansion Pak, as they were)
    }
    for (c = 0; c < 2; c++) {
        if (sRoomCullRoom[c] == room) {
            break;
        }
    }
    if (c == 2) {
        c = (sRoomCullRoom[0] == NULL) ? 0 : 1;
        sRoomCullRoom[c] = room;
        sRoomCull[c].segment = NULL;
    }
    cache = &sRoomCull[c];
    if (cache->segment != room->segment) {
        // another room's data in this one: what was kept for it is no use
        s32 e;

        for (e = 0; e < ROOMCULL_ENTRIES; e++) {
            for (c = 0; c < 2; c++) {
                if ((cache->segment != NULL) && (cache->block[e][c] != NULL)) {
                    ZeldaArena_Free(cache->block[e][c]);
                }
                cache->block[e][c] = NULL;
                cache->made[e][c] = false;
            }
        }
        cache->segment = room->segment;
    }
    if (!cache->made[entry][xlu]) {
        Gfx* rebuilt = RoomCull_Rebuild(dl, &cache->block[entry][xlu]);

        cache->dl[entry][xlu] = (rebuilt != NULL) ? rebuilt : dl;
        cache->made[entry][xlu] = true;
    }
    return cache->dl[entry][xlu];
}

/** A new scene: what was kept was in the old one's heap (cleared with it). */
void Fishmod_RoomCullReset(void) {
    bzero(sRoomCull, sizeof(sRoomCull));
    sRoomCullRoom[0] = sRoomCullRoom[1] = NULL;
}
