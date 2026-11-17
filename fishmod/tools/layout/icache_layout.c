// Chooses the order the audio driver's functions are linked in from records of what the audio thread executed
// (ares, ARES_ITRACE): the console's instruction cache is 16 KB, direct-mapped, 32-byte lines, so which functions
// share lines decides how often code is fetched from memory again. This plays the records through that cache for a
// candidate order, counts the fills, and searches orders (moves and swaps, annealed) for the fewest.
//
//   icache_layout ORDER_IN ITERATIONS SEED BUILD... > ORDER_OUT
//
// BUILD: FUNCS[,TRACE...]: one build of the game. FUNCS (layout_funcs.py): one line a function: hex address, size,
// 1 if it can be moved (the driver's), name. TRACE: that build's records: (u32 first pc, u32 count) runs of
// consecutive instructions; (0xFFFFFFFF, 0) starts a new stretch and (0xFFFFFFFE, n) says other threads ran n runs:
// the cache is emptied at the first, and at the second when n >= GAP (other threads' code has replaced the lines).
// Several builds (with and without --expak, with and without the debug harness) are searched together: the fills
// are summed over those that have records. A function every build has is given its largest size in all of them
// (written after its name: tools/mkspecrules.c pads it to that), so every build has it at the same address and
// the order suits them all, the builds listed without records too; functions only some builds have come after.
// ORDER_IN: the order to start from (names; may be empty: then the first build's addresses).
// Written out: the order found (a name a line, and the size to pad it to where the builds differ); on stderr the
// fills before and after.
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GAP 32
#define LINES 512
#define MAXB 8

typedef struct { uint32_t addr, size; int movable, gid; char name[96]; } Func;
typedef struct { int32_t fid; uint32_t off; uint32_t n; } Ev; // fid -1: code that stays put, off = pc; -2: emptied
typedef struct {
    Func *funcs; int nfuncs;
    Ev *evs; size_t nevs, capevs;
    uint32_t base, *newaddr;
    int *local; // global id -> this build's function, or -1
} Build;
typedef struct { char name[96]; int variable, differs; uint64_t weight; uint32_t size0, maxsize; int seen; } Name;

static Build builds[MAXB]; static int nbuilds;
static Name *names; static int nnames, capnames;

static int name_id(const char *s) {
    for (int i = 0; i < nnames; i++) if (strcmp(names[i].name, s) == 0) return i;
    if (nnames == capnames) { capnames = capnames ? capnames * 2 : 1024; names = realloc(names, capnames * sizeof(Name)); }
    memset(&names[nnames], 0, sizeof(Name));
    strcpy(names[nnames].name, s);
    return nnames++;
}

static int find(const Build *b, uint32_t pc) {
    int lo = 0, hi = b->nfuncs - 1, r = -1;
    while (lo <= hi) { int m = (lo + hi) / 2; if (b->funcs[m].addr <= pc) { r = m; lo = m + 1; } else hi = m - 1; }
    return r;
}

static void place(Build *b, const int *order) {
    uint32_t a = b->base;
    for (int i = 0; i < nnames; i++) { int f = b->local[order[i]]; if (f >= 0) { b->newaddr[f] = a; a += names[order[i]].variable ? b->funcs[f].size : names[order[i]].maxsize; } }
}

static uint64_t simulate(const int *order) {
    static uint32_t tag[LINES];
    uint64_t fills = 0;
    for (int k = 0; k < nbuilds; k++) {
        Build *b = &builds[k];
        if (!b->nevs) continue;
        place(b, order);
        memset(tag, 0, sizeof(tag));
        for (size_t i = 0; i < b->nevs; i++) {
            const Ev *e = &b->evs[i];
            if (e->fid == -2) { memset(tag, 0, sizeof(tag)); continue; }
            uint32_t pc = e->fid >= 0 ? b->newaddr[e->fid] + e->off : e->off;
            uint32_t l1 = (pc + e->n * 4 - 4) >> 5;
            for (uint32_t l = pc >> 5; l <= l1; l++) {
                if (tag[l & (LINES - 1)] != l) { tag[l & (LINES - 1)] = l; fills++; }
            }
        }
    }
    return fills;
}

// The fills a cache with no conflicts would still need: each line once per stretch between two emptyings.
static uint64_t floor_fills(const int *order) {
    static uint32_t seen[1 << 18], stamp = 0;
    uint64_t fills = 0;
    for (int k = 0; k < nbuilds; k++) {
        Build *b = &builds[k];
        if (!b->nevs) continue;
        place(b, order);
        stamp++;
        for (size_t i = 0; i < b->nevs; i++) {
            const Ev *e = &b->evs[i];
            if (e->fid == -2) { stamp++; continue; }
            uint32_t pc = e->fid >= 0 ? b->newaddr[e->fid] + e->off : e->off;
            for (uint32_t l = pc >> 5; l <= (pc + e->n * 4 - 4) >> 5; l++) {
                if (seen[l & 0x3FFFF] != stamp) { seen[l & 0x3FFFF] = stamp; fills++; }
            }
        }
    }
    return fills;
}

static uint64_t rng = 88172645463325252ull;
static uint32_t rnd(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return (uint32_t)(rng >> 16); }

static void load_build(Build *b, char *spec) {
    char line[512], *tok = strtok(spec, ",");
    FILE *f = fopen(tok, "r");
    int cap = 0;
    if (!f) { perror(tok); exit(1); }
    while (fgets(line, sizeof(line), f)) {
        if (b->nfuncs == cap) { cap = cap ? cap * 2 : 4096; b->funcs = realloc(b->funcs, cap * sizeof(Func)); }
        Func *p = &b->funcs[b->nfuncs];
        if (sscanf(line, "%x %u %d %95s", &p->addr, &p->size, &p->movable, p->name) == 4) b->nfuncs++;
    }
    fclose(f);
    b->newaddr = calloc(b->nfuncs, sizeof(uint32_t));
    b->base = 0xFFFFFFFF;
    for (int i = 0; i < b->nfuncs; i++) {
        Func *p = &b->funcs[i];
        p->gid = -1;
        if (!p->movable) continue;
        if (p->addr < b->base) b->base = p->addr;
        p->gid = name_id(p->name);
    }
    while ((tok = strtok(NULL, ",")) != NULL) {
        FILE *tf = fopen(tok, "rb");
        uint32_t r[2];
        if (!tf) { perror(tok); exit(1); }
        while (fread(r, 4, 2, tf) == 2) {
            Ev e;
            if (r[0] == 0xFFFFFFFFu || (r[0] == 0xFFFFFFFEu && r[1] >= GAP)) { e.fid = -2; e.off = e.n = 0; }
            else if (r[0] == 0xFFFFFFFEu) continue;
            else {
                int k = find(b, r[0]);
                if (k >= 0 && b->funcs[k].movable && r[0] < b->funcs[k].addr + b->funcs[k].size) {
                    e.fid = k; e.off = r[0] - b->funcs[k].addr; names[b->funcs[k].gid].weight += r[1];
                } else { e.fid = -1; e.off = r[0]; }
                e.n = r[1];
            }
            if (e.fid == -2 && b->nevs && b->evs[b->nevs - 1].fid == -2) continue;
            if (b->nevs == b->capevs) { b->capevs = b->capevs ? b->capevs * 2 : 1 << 20; b->evs = realloc(b->evs, b->capevs * sizeof(Ev)); }
            b->evs[b->nevs++] = e;
        }
        fclose(tf);
    }
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: icache_layout ORDER_IN ITERATIONS SEED FUNCS[,TRACE...]...\n"); return 1; }
    long iters = atol(argv[2]);
    char line[512];
    rng ^= strtoull(argv[3], NULL, 0) * 0x9E3779B97F4A7C15ull;
    for (int a = 4; a < argc && nbuilds < MAXB; a++) load_build(&builds[nbuilds++], argv[a]);
    size_t nevs = 0;
    for (int k = 0; k < nbuilds; k++) {
        Build *b = &builds[k];
        b->local = malloc(nnames * sizeof(int));
        for (int i = 0; i < nnames; i++) b->local[i] = -1;
        for (int i = 0; i < b->nfuncs; i++) if (b->funcs[i].gid >= 0) b->local[b->funcs[i].gid] = i;
        nevs += b->nevs;
        for (int i = 0; i < nnames; i++) {
            int f = b->local[i];
            if (f < 0) names[i].variable = 1;
            else if (!names[i].seen) { names[i].seen = 1; names[i].size0 = b->funcs[f].size; }
            else if (names[i].size0 != b->funcs[f].size) names[i].differs = 1;
            if (f >= 0 && b->funcs[f].size > names[i].maxsize) names[i].maxsize = b->funcs[f].size;
        }
    }

    // the starting order: ORDER_IN's names, then the rest as the first build has them (then the other builds' own)
    int *order = malloc(nnames * sizeof(int)), n = 0;
    char *used = calloc(nnames, 1);
    FILE *f = fopen(argv[1], "r");
    while (f && fgets(line, sizeof(line), f)) {
        char name[96];
        if (line[0] == '#' || sscanf(line, " %95[A-Za-z0-9_.]", name) != 1) continue;
        for (int i = 0; i < nnames; i++) if (!used[i] && strcmp(names[i].name, name) == 0) { order[n++] = i; used[i] = 1; }
    }
    if (f) fclose(f);
    for (int i = 0; i < nnames; i++) if (!used[i]) order[n++] = i; // (ids were given in the first build's address order)
    uint64_t first = simulate(order);

    // Four groups, in this order: functions every build has that the records run (searched), functions only some
    // builds have that the records run (searched), then those that never ran (in every build, then the others).
    int *tmp = malloc(nnames * sizeof(int)), lim[5] = {0}, m = 0;
    for (int g = 0; g < 4; g++) {
        for (int i = 0; i < nnames; i++) {
            int id = order[i], grp = (names[id].weight ? 0 : 2) + (names[id].variable ? 1 : 0);
            if (grp == g) tmp[m++] = id;
        }
        lim[g + 1] = m;
    }
    memcpy(order, tmp, nnames * sizeof(int));
    uint64_t cur = simulate(order), best = cur;
    int *bestOrder = malloc(nnames * sizeof(int));
    memcpy(bestOrder, order, nnames * sizeof(int));
    fprintf(stderr, "%zu events in %d builds; %d functions: %d in every build and run, %d in some and run; fills: as given %llu, grouped %llu\n",
            nevs, nbuilds, nnames, lim[1], lim[2] - lim[1], (unsigned long long)first, (unsigned long long)cur);

    int nhot = lim[2];
    double temp = cur * 0.001;
    for (long it = 0; it < iters; it++) {
        int g = (lim[2] > lim[1] + 1 && (lim[1] < 2 || rnd() % nhot >= (uint32_t)lim[1])) ? 1 : 0;
        int lo = lim[g], cnt = lim[g + 1] - lim[g];
        if (cnt < 2) continue;
        int a = lo + rnd() % cnt, b = lo + rnd() % cnt;
        if (a == b) continue;
        memcpy(tmp, order, nhot * sizeof(int));
        if (rnd() & 1) { int t = order[a]; order[a] = order[b]; order[b] = t; }
        else {
            int t = order[a];
            if (a < b) memmove(&order[a], &order[a + 1], (b - a) * sizeof(int)); else memmove(&order[b + 1], &order[b], (a - b) * sizeof(int));
            order[b] = t;
        }
        uint64_t c = simulate(order);
        double d = (double)c - (double)cur;
        if (d <= 0 || (temp > 1e-9 && (rnd() & 0xFFFFFF) / 16777216.0 < exp(-d / temp))) {
            cur = c;
            if (c < best) { best = c; memcpy(bestOrder, order, nnames * sizeof(int)); }
        } else memcpy(order, tmp, nhot * sizeof(int));
        temp *= 1.0 - 6.0 / iters;
        if ((it & 1023) == 0) fprintf(stderr, "%ld: now %llu best %llu\n", it, (unsigned long long)cur, (unsigned long long)best);
    }
    fprintf(stderr, "fills: %llu as given, %llu found; %llu with no line shared\n", (unsigned long long)first, (unsigned long long)best,
            (unsigned long long)floor_fills(bestOrder));
    for (int i = 0; i < nnames; i++) {
        const Name *nm = &names[bestOrder[i]];
        if (!nm->variable && nm->differs) printf("%s 0x%X\n", nm->name, nm->maxsize); else printf("%s\n", nm->name);
    }
    return 0;
}
