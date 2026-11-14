#ifndef N64PROF_H
#define N64PROF_H

/*
 * n64prof: a PC-sampling profiler for N64 programs (any game or homebrew), for compiler sample profiles.
 *
 * The program calls n64prof_sample() from a timer interrupt with the PC the CPU was interrupted at; samples are
 * counted per bucket of code (16 bytes by default) in a hash table the program gives it. Overlays: the program
 * says where each loaded overlay is (n64prof_ranges_*), and samples inside one are counted at their link-time
 * address, so every bucket matches the ELF. n64prof_serialize() writes the hottest buckets to a buffer in a fixed
 * big-endian format (README.md) for the program to store (save memory, a flashcart's USB, ...).
 *
 * No libultra or libdragon: plain C89, no floating point, nothing allocated. See README.md for wiring it up.
 */

#define N64PROF_MAGIC 0x4E363450 /* 'N64P' */
#define N64PROF_VERSION 1       /* a snapshot: every count since the start (n64prof_serialize) */
#define N64PROF_VERSION_DELTA 2 /* a chunk of counts since the last swap (n64prof_pack): a reader adds them all up */
#define N64PROF_MAX_RANGES 128

typedef struct N64ProfEntry {
    unsigned int addr;  /* the bucket's first address (link-time for overlay code) */
    unsigned int count; /* samples in it */
} N64ProfEntry;

typedef struct N64ProfHeader {
    /* 0x00 */ unsigned int magic;    /* N64PROF_MAGIC */
    /* 0x04 */ unsigned int version;  /* N64PROF_VERSION */
    /* 0x08 */ unsigned int total;    /* samples taken */
    /* 0x0C */ unsigned int dropped;  /* not counted: the table was full */
    /* 0x10 */ unsigned int unsaved;  /* counted, in buckets too cold to fit in this dump */
    /* 0x14 */ unsigned int entries;  /* N64ProfEntry records that follow (in no order) */
    /* 0x18 */ unsigned int interval; /* CPU count cycles (half the CPU clock) between samples, on average */
    /* 0x1C */ unsigned int unmapped; /* outside every range given and above n64prof_init's staticEnd */
    /* 0x20 */ unsigned int bucketShift; /* log2 of the bucket size in bytes */
    /* 0x24 */ unsigned int dumps;    /* dumps written so far (this one included) */
    /* 0x28 */ unsigned int user[6];  /* the program's own (e.g. a build id, frames played) */
} N64ProfHeader; /* size = 0x40 */

/* Starts counting into table (tableSize entries, a power of two). Code below staticEnd runs where it was linked;
   above it, only the ranges given are mapped (the rest is counted as unmapped). */
void n64prof_init(N64ProfEntry* table, unsigned int tableSize, unsigned int interval, unsigned int staticEnd);

/* One sample (interrupt context). */
void n64prof_sample(unsigned int pc);

/* A tag for the samples from now on (0 to the bucket size - 1, kept in the low bits of each bucket's address): what
   the program was doing, its own say (e.g. 1 while some part of it runs). 0: untagged. Samples counted as they come
   only: with a frame ring set (below), its own tags take those bits. */
extern unsigned int n64prof_tag;

/* The count cycles until the next sample: the interval, jittered a quarter either way so that work repeating with
   the frame isn't always hit or always missed. Call it from the interrupt and set the timer to it. */
unsigned int n64prof_next_interval(void);

/* The overlays loaded now: fill the array n64prof_ranges_begin() returns (at most N64PROF_MAX_RANGES), then
   n64prof_ranges_end(count). Double-buffered: safe while the interrupt samples. */
typedef struct N64ProfRange {
    unsigned int ramStart; /* where it's loaded */
    unsigned int ramEnd;
    unsigned int vramStart; /* where it was linked */
} N64ProfRange;
N64ProfRange* n64prof_ranges_begin(void);
void n64prof_ranges_end(int count);

/* The bucket size, log2 bytes (default N64PROF_BUCKET_SHIFT): 2 is one bucket per instruction. Before n64prof_init. */
void n64prof_set_bucket_shift(unsigned int shift);
/* Streaming (a link to a PC, e.g. a flashcart's USB): count into one table while the last is sent. n64prof_swap starts
   counting into fresh (the same size as n64prof_init's, all zero: n64prof_clear) and returns the table it replaces;
   n64prof_used says how many entries the current table has filled (time to swap before it gets full). */
N64ProfEntry* n64prof_swap(N64ProfEntry* fresh);
unsigned int n64prof_used(void);
void n64prof_clear(N64ProfEntry* table);
/* Packs a table n64prof_swap returned into chunks: the header (version N64PROF_VERSION_DELTA, a running chunk number
   in dumps, the totals so far) and its buckets from *pos on, as many as fit in outSize. Returns the bytes written
   and moves *pos on; the table is done when *pos passes the last entry (n64prof_pack_done). */
unsigned int n64prof_pack(const N64ProfEntry* table, unsigned int* pos, void* out, unsigned int outSize,
                          const unsigned int* user);
int n64prof_pack_done(unsigned int pos);
/* Per-frame tags (for "where does a slow frame's time go"): with a frame ring set, samples wait in it (at most size;
   two rings of size are used, alternately) until n64prof_frame_end(tag) counts the frame's samples with tag in the
   bucket address's low bits (bucket shift 2 or more: the low two bits are free; tag 1 or 2). Thread context. */
void n64prof_set_frame_ring(unsigned int* rings, unsigned int size);
void n64prof_frame_end(unsigned int tag);
/* Writes the header and the hottest buckets that fit into out (outSize bytes); returns the bytes written (a multiple
   of 16). user[] goes into the header as given (6 words, or NULL). */
unsigned int n64prof_serialize(void* out, unsigned int outSize, const unsigned int* user);

#endif
