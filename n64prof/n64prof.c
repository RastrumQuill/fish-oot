/* n64prof: see n64prof.h and README.md. */
#include "n64prof.h"

#ifndef N64PROF_BUCKET_SHIFT
#define N64PROF_BUCKET_SHIFT 4 /* log2 of the bucket size: 16 bytes (4 instructions); 5 halves the table's need */
#endif
#define PROBES 16

static N64ProfEntry* sTable;
static unsigned int sMask;
static unsigned int sHashShift;
static unsigned int sInterval;
static unsigned int sStaticEnd;
static unsigned int sRand = 0x2545F491;
static volatile unsigned int sTotal;
static volatile unsigned int sDropped;
static volatile unsigned int sUnmapped;
static volatile unsigned int sUsed; /* entries filled in the current table */
static unsigned int sShift = N64PROF_BUCKET_SHIFT;
static unsigned int sDumps;
static N64ProfRange sRanges[2][N64PROF_MAX_RANGES];
static int sRangeCount[2];
static volatile int sRangeSet;

void n64prof_init(N64ProfEntry* table, unsigned int tableSize, unsigned int interval, unsigned int staticEnd) {
    unsigned int i;

    for (i = 0; i < tableSize; i++) {
        table[i].addr = 0;
        table[i].count = 0;
    }
    sHashShift = 32;
    for (i = tableSize; i > 1; i >>= 1) {
        sHashShift--;
    }
    sMask = tableSize - 1;
    sInterval = interval;
    sStaticEnd = staticEnd;
    sRangeCount[0] = sRangeCount[1] = 0;
    sTotal = sDropped = sUnmapped = sUsed = 0;
    sTable = table; /* last: the interrupt checks it */
}

void n64prof_set_bucket_shift(unsigned int shift) {
    sShift = shift;
}

void n64prof_clear(N64ProfEntry* table) {
    unsigned int i;

    for (i = 0; i <= sMask; i++) {
        table[i].addr = 0;
        table[i].count = 0;
    }
}

N64ProfEntry* n64prof_swap(N64ProfEntry* fresh) {
    N64ProfEntry* old = sTable;

    sTable = fresh; /* one store: the interrupt counts into the old table or the new one */
    sUsed = 0;      /* (a sample between the two stores is lost from the count, not from the table) */
    return old;
}

unsigned int n64prof_used(void) {
    return sUsed;
}

unsigned int n64prof_next_interval(void) {
    sRand ^= sRand << 13;
    sRand ^= sRand >> 17;
    sRand ^= sRand << 5;
    return (sInterval * 3 / 4) + (sRand % (sInterval / 2 + 1));
}

void n64prof_sample(unsigned int pc) {
    const N64ProfRange* r;
    N64ProfEntry* table = sTable;
    N64ProfEntry* e;
    unsigned int key;
    unsigned int h;
    int set;
    int n;
    int i;

    if (table == 0) {
        return;
    }
    sTotal++;
    set = sRangeSet;
    r = sRanges[set];
    n = sRangeCount[set];
    for (i = 0; i < n; i++, r++) {
        if ((pc - r->ramStart) < (r->ramEnd - r->ramStart)) {
            pc = pc - r->ramStart + r->vramStart;
            break;
        }
    }
    if ((i == n) && (pc >= sStaticEnd)) {
        sUnmapped++;
    }
    key = pc & ~((1u << sShift) - 1);
    h = ((key >> sShift) * 2654435761u) >> sHashShift;
    for (i = 0; i < PROBES; i++) {
        e = &table[(h + i) & sMask];
        if (e->addr == key) {
            e->count++;
            return;
        }
        if (e->addr == 0) {
            e->addr = key;
            e->count = 1;
            sUsed++;
            return;
        }
    }
    sDropped++;
}

N64ProfRange* n64prof_ranges_begin(void) {
    return sRanges[sRangeSet ^ 1];
}

void n64prof_ranges_end(int count) {
    int set = sRangeSet ^ 1;

    sRangeCount[set] = (count < N64PROF_MAX_RANGES) ? count : N64PROF_MAX_RANGES;
    sRangeSet = set; /* one store: the interrupt sees the old set or the new one */
}

static int TopBit(unsigned int c) {
    int b = 31;

    while (!(c & (1u << b))) {
        b--;
    }
    return b;
}

unsigned int n64prof_serialize(void* out, unsigned int outSize, const unsigned int* user) {
    N64ProfHeader* hdr = (N64ProfHeader*)out;
    N64ProfEntry* dst = (N64ProfEntry*)((char*)out + sizeof(N64ProfHeader));
    unsigned int max = (outSize - sizeof(N64ProfHeader)) / sizeof(N64ProfEntry);
    unsigned int hist[32];
    unsigned int room;
    unsigned int saved = 0;
    unsigned int counted = 0;
    unsigned int bytes;
    int floorBit;
    int b;
    unsigned int i;

    if ((sTable == 0) || (outSize < sizeof(N64ProfHeader))) {
        return 0;
    }
    for (b = 0; b < 32; b++) {
        hist[b] = 0;
    }
    for (i = 0; i <= sMask; i++) {
        if (sTable[i].count != 0) {
            hist[TopBit(sTable[i].count)]++;
        }
    }
    /* the hottest powers of two that fit whole; the next one down fills what's left */
    room = max;
    for (floorBit = 31; floorBit > 0; floorBit--) {
        if (hist[floorBit] > room) {
            break;
        }
        room -= hist[floorBit];
    }
    /* one pass: every bucket above floorBit's power of two, and floorBit's own while they fit */
    for (i = 0; (i <= sMask) && (saved < max); i++) {
        unsigned int c = sTable[i].count;
        int top;

        if (c == 0) {
            continue;
        }
        top = TopBit(c);
        if ((top > floorBit) || ((top == floorBit) && (room > 0))) {
            if (top == floorBit) {
                room--;
            }
            dst[saved] = sTable[i];
            saved++;
            counted += c;
        }
    }
    hdr->magic = N64PROF_MAGIC;
    hdr->version = N64PROF_VERSION;
    hdr->total = sTotal;
    hdr->dropped = sDropped;
    hdr->unsaved = sTotal - sDropped - counted;
    hdr->entries = saved;
    hdr->interval = sInterval;
    hdr->unmapped = sUnmapped;
    hdr->bucketShift = sShift;
    hdr->dumps = ++sDumps;
    for (i = 0; i < 6; i++) {
        hdr->user[i] = (user != 0) ? user[i] : 0;
    }
    bytes = sizeof(N64ProfHeader) + saved * sizeof(N64ProfEntry);
    return (bytes + 15) & ~15u;
}

int n64prof_pack_done(unsigned int pos) {
    return pos > sMask;
}

unsigned int n64prof_pack(const N64ProfEntry* table, unsigned int* pos, void* out, unsigned int outSize,
                          const unsigned int* user) {
    N64ProfHeader* hdr = (N64ProfHeader*)out;
    N64ProfEntry* dst = (N64ProfEntry*)((char*)out + sizeof(N64ProfHeader));
    unsigned int max;
    unsigned int saved = 0;
    unsigned int i = *pos;

    if (outSize < sizeof(N64ProfHeader) + sizeof(N64ProfEntry)) {
        return 0;
    }
    max = (outSize - sizeof(N64ProfHeader)) / sizeof(N64ProfEntry);
    for (; (i <= sMask) && (saved < max); i++) {
        if (table[i].count != 0) {
            dst[saved] = table[i];
            saved++;
        }
    }
    *pos = i;
    hdr->magic = N64PROF_MAGIC;
    hdr->version = N64PROF_VERSION_DELTA;
    hdr->total = sTotal;
    hdr->dropped = sDropped;
    hdr->unsaved = 0;
    hdr->entries = saved;
    hdr->interval = sInterval;
    hdr->unmapped = sUnmapped;
    hdr->bucketShift = sShift;
    hdr->dumps = ++sDumps;
    for (i = 0; i < 6; i++) {
        hdr->user[i] = (user != 0) ? user[i] : 0;
    }
    return sizeof(N64ProfHeader) + saved * sizeof(N64ProfEntry);
}
