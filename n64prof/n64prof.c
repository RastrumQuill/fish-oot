/* n64prof: see n64prof.h and README.md. */
#include "n64prof.h"

#define BUCKET_SHIFT 4
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
    sTotal = sDropped = sUnmapped = 0;
    sTable = table; /* last: the interrupt checks it */
}

unsigned int n64prof_next_interval(void) {
    sRand ^= sRand << 13;
    sRand ^= sRand >> 17;
    sRand ^= sRand << 5;
    return (sInterval * 3 / 4) + (sRand % (sInterval / 2 + 1));
}

void n64prof_sample(unsigned int pc) {
    const N64ProfRange* r;
    N64ProfEntry* e;
    unsigned int key;
    unsigned int h;
    int set;
    int n;
    int i;

    if (sTable == 0) {
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
    key = pc & ~((1u << BUCKET_SHIFT) - 1);
    h = ((key >> BUCKET_SHIFT) * 2654435761u) >> sHashShift;
    for (i = 0; i < PROBES; i++) {
        e = &sTable[(h + i) & sMask];
        if (e->addr == key) {
            e->count++;
            return;
        }
        if (e->addr == 0) {
            e->addr = key;
            e->count = 1;
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
    room = max;
    for (b = 31; (b >= floorBit) && (room > 0); b--) {
        for (i = 0; (i <= sMask) && (room > 0); i++) {
            unsigned int c = sTable[i].count;

            if ((c != 0) && (TopBit(c) == b)) {
                dst[saved] = sTable[i];
                saved++;
                room--;
                counted += c;
            }
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
    hdr->bucketShift = BUCKET_SHIFT;
    hdr->dumps = ++sDumps;
    for (i = 0; i < 6; i++) {
        hdr->user[i] = (user != 0) ? user[i] : 0;
    }
    bytes = sizeof(N64ProfHeader) + saved * sizeof(N64ProfEntry);
    return (bytes + 15) & ~15u;
}
