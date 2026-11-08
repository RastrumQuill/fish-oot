#ifndef VRTIME_H
#define VRTIME_H

/*
 * vrtime: timing tests for the N64's CPU (NEC VR4300), for a compiler's cost model: how many cycles each kind of
 * instruction takes, alone (latency: a chain where each needs the last one's result) and side by side
 * (throughput), and what loads, stores, branches and cache misses cost. Each test is a tight assembly loop timed
 * with the CPU's Count register (one tick every two CPU cycles), interrupts off, best of several runs, with the
 * loop's own cost (an empty loop) taken off by the reader (n64prof/tools/vrtime.py).
 *
 * Needs GCC (inline assembly) building for the VR4300; no libultra or libdragon: the program gives it a 64 KB
 * buffer in cached RDRAM and two functions to turn interrupts off and on.
 */

#define VRTIME_MAGIC 0x56525431 /* 'VRT1' */
#define VRTIME_MAX_TESTS 64

typedef struct VrtimeResult {
    char name[16];      /* the test's name (vrtime.py describes each) */
    unsigned int loops; /* times round the loop */
    unsigned int ops;   /* the operations measured per loop (16: the body repeated) */
    unsigned int best;  /* Count ticks for the whole run, the best of the runs */
    unsigned int worst; /* and the worst (a spread says something disturbed it) */
} VrtimeResult; /* size = 0x20 */

typedef struct VrtimeHeader {
    unsigned int magic; /* VRTIME_MAGIC */
    unsigned int count; /* results that follow */
    unsigned int runs;  /* runs of each test */
    unsigned int pad[5];
} VrtimeHeader; /* size = 0x20 */

/* Runs every test into out (at most max); returns how many. buf: 64 KB, 16-byte aligned, cached (KSEG0). */
int vrtime_run(VrtimeResult* out, int max, void* buf, unsigned int (*irqOff)(void), void (*irqOn)(unsigned int));

#endif
