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
#define N64PROF_VERSION 1
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
    /* 0x14 */ unsigned int entries;  /* N64ProfEntry records that follow, hottest first by powers of two */
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

/* Writes the header and the hottest buckets that fit into out (outSize bytes); returns the bytes written (a multiple
   of 16). user[] goes into the header as given (6 words, or NULL). */
unsigned int n64prof_serialize(void* out, unsigned int outSize, const unsigned int* user);

#endif
