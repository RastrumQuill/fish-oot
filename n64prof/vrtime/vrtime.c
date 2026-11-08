/* vrtime: see vrtime.h. GCC only (inline assembly), built for the VR4300 (-march=vr4300). */
#include "vrtime.h"

#define RUNS 5
#define BODY ".rept 16\n"

/* every test may use these; the compiler keeps its own values elsewhere */
#define CLOBBERS                                                                                                   \
    "$8", "$10", "$11", "$12", "$13", "$14", "$15", "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", \
        "$f9", "$31", "hi", "lo", "memory"

/*
 * One test: `setup` once, then the loop: `body` 16 times, n times round. Returns the Count ticks for the loop.
 * %3 is the 64 KB buffer (or what the test was given). noreorder: the assembler leaves the code exactly as written.
 */
#define VRT_TEST(fn, setup, body)                                                                                  \
    static unsigned int fn(unsigned int n, void* p) {                                                              \
        unsigned int s;                                                                                            \
        unsigned int e;                                                                                            \
        __asm__ volatile(".set push\n.set noreorder\n.set noat\n" setup "\n"                                       \
                         "mfc0 %0, $9\n"                                                                           \
                         "1:\n" BODY body "\n.endr\n"                                                              \
                         "addiu %2, %2, -1\n"                                                                      \
                         "bnez %2, 1b\n"                                                                           \
                         "nop\n"                                                                                   \
                         "mfc0 %1, $9\n"                                                                           \
                         ".set pop\n"                                                                              \
                         : "=&r"(s), "=&r"(e), "+r"(n)                                                             \
                         : "r"(p)                                                                                  \
                         : CLOBBERS);                                                                              \
        return e - s;                                                                                              \
    }

#define FP1 "li $8, 0x3F800000\nmtc1 $8, $f0\nmtc1 $8, $f2\nmtc1 $8, $f4\nmtc1 $8, $f6\nmtc1 $8, $f8\n"
#define FPD FP1 "cvt.d.s $f0, $f0\ncvt.d.s $f2, $f2\ncvt.d.s $f4, $f4\ncvt.d.s $f6, $f6\ncvt.d.s $f8, $f8\n"
#define INTS "li $10, 1000003\nli $11, 7\nli $12, 13\n"

/* the loop alone, and the simplest things */
VRT_TEST(T_loop, "", "")
VRT_TEST(T_nop, "", "nop")
VRT_TEST(T_aluDep, "li $8, 1", "addu $8, $8, $8")
VRT_TEST(T_aluInd, INTS, "addu $13, $11, $12")
VRT_TEST(T_shiftDep, "li $8, 1", "sll $8, $8, 1")
VRT_TEST(T_daluDep, "li $8, 1", "daddu $8, $8, $8")
VRT_TEST(T_dshiftDep, "li $8, 1", "dsll32 $8, $8, 1")
VRT_TEST(T_luiOri, "", "lui $8, 0x8012\nori $8, $8, 0x3456")
/* loads and stores (a line that stays in the cache) */
VRT_TEST(T_ldUse0, "", "lw $10, 0(%3)\naddu $11, $10, $10")
VRT_TEST(T_ldUse1, "", "lw $10, 0(%3)\nnop\naddu $11, $10, $10")
VRT_TEST(T_ldInd, "", "lw $10, 0(%3)")
VRT_TEST(T_ldChain, "move $8, %3", "lw $8, 0($8)")
VRT_TEST(T_ld64, "", "ld $10, 0(%3)")
VRT_TEST(T_lbuUse0, "", "lbu $10, 0(%3)\naddu $11, $10, $10")
VRT_TEST(T_st, INTS, "sw $10, 0(%3)")
VRT_TEST(T_st64, INTS, "sd $10, 0(%3)")
VRT_TEST(T_stLd, INTS, "sw $10, 0(%3)\nlw $11, 0(%3)")
VRT_TEST(T_stLdUse, INTS, "sw $10, 0(%3)\nlw $11, 0(%3)\naddu $12, $11, $11")
VRT_TEST(T_stSt, INTS, "sw $10, 0(%3)\nsw $11, 4(%3)")
/* multiply and divide: latency (the result read at once) and back to back */
VRT_TEST(T_multLat, INTS, "mult $10, $11\nmflo $10")
VRT_TEST(T_multuLat, INTS, "multu $10, $11\nmflo $10")
VRT_TEST(T_multThr, INTS, "mult $11, $12")
VRT_TEST(T_multHi, INTS, "mult $10, $11\nmfhi $13")
VRT_TEST(T_dmultLat, INTS, "dmult $10, $11\nmflo $10")
VRT_TEST(T_dmultuLat, INTS, "dmultu $10, $11\nmflo $10")
VRT_TEST(T_divLat, INTS, "div $zero, $10, $11\nmflo $13")
VRT_TEST(T_divuLat, INTS, "divu $zero, $10, $11\nmflo $13")
VRT_TEST(T_ddivLat, INTS, "ddiv $zero, $10, $11\nmflo $13")
VRT_TEST(T_ddivuLat, INTS, "ddivu $zero, $10, $11\nmflo $13")
VRT_TEST(T_multAlu, INTS, "mult $11, $12\naddu $13, $11, $12\naddu $14, $11, $12\naddu $15, $11, $12\nmflo $13")
/* the FPU: latency (a chain) and throughput (independent) */
VRT_TEST(T_addsLat, FP1, "add.s $f0, $f0, $f2")
VRT_TEST(T_addsThr, FP1, "add.s $f4, $f6, $f8")
VRT_TEST(T_mulsLat, FP1, "mul.s $f0, $f0, $f2")
VRT_TEST(T_mulsThr, FP1, "mul.s $f4, $f6, $f8")
VRT_TEST(T_divsLat, FP1, "div.s $f0, $f0, $f2")
VRT_TEST(T_sqrtsLat, FP1, "sqrt.s $f0, $f0")
VRT_TEST(T_adddLat, FPD, "add.d $f0, $f0, $f2")
VRT_TEST(T_muldLat, FPD, "mul.d $f0, $f0, $f2")
VRT_TEST(T_muldThr, FPD, "mul.d $f4, $f6, $f8")
VRT_TEST(T_divdLat, FPD, "div.d $f0, $f0, $f2")
VRT_TEST(T_sqrtdLat, FPD, "sqrt.d $f0, $f0")
VRT_TEST(T_mulAddS, FP1, "mul.s $f4, $f6, $f8\nadd.s $f0, $f0, $f4")
VRT_TEST(T_fpAluMix, FP1 INTS, "mul.s $f4, $f6, $f8\naddu $13, $11, $12\naddu $14, $11, $12\naddu $15, $11, $12")
VRT_TEST(T_cvtsw, FP1, "cvt.s.w $f4, $f6")
VRT_TEST(T_truncws, FP1, "trunc.w.s $f4, $f6")
VRT_TEST(T_cvtds, FP1, "cvt.d.s $f4, $f6")
VRT_TEST(T_mfc1mtc1, FP1, "mfc1 $8, $f0\nmtc1 $8, $f0")
VRT_TEST(T_lwc1Use, FP1, "lwc1 $f4, 8(%3)\nadd.s $f6, $f4, $f4") /* (word 2: 1.0f, not a denormal) */
VRT_TEST(T_fcmpBr, FP1, "c.lt.s $f0, $f2\nbc1t 2f\nnop\n2:")
/* branches */
VRT_TEST(T_brTaken, "", "b 2f\nnop\n2:")
VRT_TEST(T_brNot, "", "bnez $zero, 2f\nnop\n2:")
VRT_TEST(T_brlTaken, "", "beql $zero, $zero, 2f\nnop\n2:")
VRT_TEST(T_brlNot, "", "bnezl $zero, 2f\nnop\n2:")
VRT_TEST(T_bal, "", "bal 2f\nnop\n2:")
/* memory past the caches (the buffer set up for each: see vrtime_run) */
VRT_TEST(T_dcChase, "move $8, %3", "lw $8, 0($8)")
VRT_TEST(T_dcSeqRead, "move $8, %3", "lw $10, 0($8)\naddiu $8, $8, 16")
VRT_TEST(T_dcSeqWrite, "move $8, %3", "sw $zero, 0($8)\naddiu $8, $8, 16")

/* 64 KB of straight code (the instruction cache is 16 KB): every 32-byte line a miss. Count ticks for n passes. */
static unsigned int T_icMiss(unsigned int n, void* p) {
    unsigned int s;
    unsigned int e;

    (void)p;
    __asm__ volatile(".set push\n.set noreorder\n"
                     "mfc0 %0, $9\n"
                     "1:\n.rept 16384\nnop\n.endr\n"
                     "addiu %2, %2, -1\n"
                     "bnez %2, 1b\n"
                     "nop\n"
                     "mfc0 %1, $9\n"
                     ".set pop\n"
                     : "=&r"(s), "=&r"(e), "+r"(n)
                     :
                     : "memory");
    return e - s;
}

typedef unsigned int (*VrtFn)(unsigned int n, void* p);

typedef struct VrtTest {
    const char* name;
    VrtFn fn;
    unsigned int loops;
    unsigned int ops; /* per loop */
} VrtTest;

#define N 1000 /* times round: 16000 operations */

static const VrtTest sCoreTests[] = {
    { "loop", T_loop, N, 16 },          { "nop", T_nop, N, 16 },
    { "alu.dep", T_aluDep, N, 16 },     { "alu.ind", T_aluInd, N, 16 },
    { "sll.dep", T_shiftDep, N, 16 },   { "daddu.dep", T_daluDep, N, 16 },
    { "dsll32.dep", T_dshiftDep, N, 16 }, { "lui+ori", T_luiOri, N, 16 },
    { "lw.use0", T_ldUse0, N, 16 },     { "lw.use1", T_ldUse1, N, 16 },
    { "lw.ind", T_ldInd, N, 16 },       { "lw.chain", T_ldChain, N, 16 },
    { "ld.ind", T_ld64, N, 16 },        { "lbu.use0", T_lbuUse0, N, 16 },
    { "sw", T_st, N, 16 },              { "sd", T_st64, N, 16 },
    { "sw+lw", T_stLd, N, 16 },         { "sw+lw.use", T_stLdUse, N, 16 },
    { "sw+sw", T_stSt, N, 16 },         { "mult.lat", T_multLat, N, 16 },
    { "multu.lat", T_multuLat, N, 16 }, { "mult.thr", T_multThr, N, 16 },
    { "mult+mfhi", T_multHi, N, 16 },   { "dmult.lat", T_dmultLat, N, 16 },
    { "dmultu.lat", T_dmultuLat, N, 16 }, { "div.lat", T_divLat, N, 16 },
    { "divu.lat", T_divuLat, N, 16 },   { "ddiv.lat", T_ddivLat, N, 16 },
    { "ddivu.lat", T_ddivuLat, N, 16 }, { "mult+3alu", T_multAlu, N, 16 },
    { "add.s.lat", T_addsLat, N, 16 },  { "add.s.thr", T_addsThr, N, 16 },
    { "mul.s.lat", T_mulsLat, N, 16 },  { "mul.s.thr", T_mulsThr, N, 16 },
    { "div.s.lat", T_divsLat, N, 16 },  { "sqrt.s.lat", T_sqrtsLat, N, 16 },
    { "add.d.lat", T_adddLat, N, 16 },  { "mul.d.lat", T_muldLat, N, 16 },
    { "mul.d.thr", T_muldThr, N, 16 },  { "div.d.lat", T_divdLat, N, 16 },
    { "sqrt.d.lat", T_sqrtdLat, N, 16 }, { "mul+add.s", T_mulAddS, N, 16 },
    { "mul.s+3alu", T_fpAluMix, N, 16 }, { "cvt.s.w", T_cvtsw, N, 16 },
    { "trunc.w.s", T_truncws, N, 16 },  { "cvt.d.s", T_cvtds, N, 16 },
    { "mfc1+mtc1", T_mfc1mtc1, N, 16 }, { "lwc1.use", T_lwc1Use, N, 16 },
    { "c.lt+bc1t", T_fcmpBr, N, 16 },   { "b.taken", T_brTaken, N, 16 },
    { "b.not", T_brNot, N, 16 },        { "bl.taken", T_brlTaken, N, 16 },
    { "bl.not", T_brlNot, N, 16 },      { "bal", T_bal, N, 16 },
};

/* The RSP halted and the RDP idle, so nothing else uses memory during a test (with interrupts off, nothing can
   start them again until it's done). Gives up after a while: the result then shows the contention. */
static void WaitRcpIdle(void) {
    volatile unsigned int* spStatus = (volatile unsigned int*)0xA4040010;
    volatile unsigned int* dpcStatus = (volatile unsigned int*)0xA410000C;
    unsigned int tries;

    for (tries = 0; tries < 2000000; tries++) {
        /* SP: halted (bit 0); DPC: not busy (pipe busy bit 6, command busy bit 5, DMA busy bit 8) */
        if ((*spStatus & 1) && !(*dpcStatus & ((1 << 5) | (1 << 6) | (1 << 8)))) {
            return;
        }
    }
}

static void RunOne(VrtimeResult* r, const char* name, VrtFn fn, unsigned int loops, unsigned int ops, void* p,
                   unsigned int (*irqOff)(void), void (*irqOn)(unsigned int)) {
    unsigned int best = 0xFFFFFFFF;
    unsigned int worst = 0;
    int run;
    int i;

    for (i = 0; (i < 15) && name[i]; i++) {
        r->name[i] = name[i];
    }
    for (; i < 16; i++) {
        r->name[i] = 0;
    }
    /* run 0 warms the caches (and the instruction cache with the test's code); RUNS more are timed */
    for (run = 0; run <= RUNS; run++) {
        unsigned int mask = irqOff();
        unsigned int t;

        WaitRcpIdle();
        t = fn(loops, p);
        irqOn(mask);
        if (run == 0) {
            continue;
        }
        if (t < best) {
            best = t;
        }
        if (t > worst) {
            worst = t;
        }
    }
    r->loops = loops;
    r->ops = ops;
    r->best = best;
    r->worst = worst;
}

/* The buffer as a chain through every 16-byte line in a shuffled order (each load's address is the last one's
   value), so no line is near the last and the D-cache (8 KB) can't keep up. or: added to every pointer (KSEG1:
   uncached). */
static void MakeChase(unsigned int* buf, unsigned int or) {
    static unsigned short sOrder[4096];
    unsigned int rnd = 0x12345678;
    int i;

    for (i = 0; i < 4096; i++) {
        sOrder[i] = i;
    }
    for (i = 4095; i > 0; i--) {
        int j;
        unsigned short t;

        rnd ^= rnd << 13;
        rnd ^= rnd >> 17;
        rnd ^= rnd << 5;
        j = rnd % (i + 1);
        t = sOrder[i];
        sOrder[i] = sOrder[j];
        sOrder[j] = t;
    }
    for (i = 0; i < 4096; i++) {
        buf[sOrder[i] * 4] = ((unsigned int)&buf[sOrder[(i + 1) % 4096] * 4]) | or;
    }
}

/* Writes back and empties the whole data cache (8 KB: 512 lines of 16 bytes, by index). */
static void FlushDcache(void) {
    unsigned int a;

    for (a = 0x80000000; a < 0x80000000 + 0x2000; a += 16) {
        __asm__ volatile(".set push\n.set noreorder\ncache 0x01, 0(%0)\n.set pop\n" : : "r"(a) : "memory");
    }
}

int vrtime_run(VrtimeResult* out, int max, void* buf, unsigned int (*irqOff)(void), void (*irqOn)(unsigned int)) {
    unsigned int* b = (unsigned int*)buf;
    int n = 0;
    unsigned int i;

    /* the core tests: one line of the buffer, pointing at itself (lw.chain) */
    b[0] = (unsigned int)b;
    b[1] = 0;
    b[2] = 0x3F800000; /* 1.0f for the FPU's loads: the VR4300 traps on denormals (a pointer read as a float is one) */
    for (i = 0; (i < sizeof(sCoreTests) / sizeof(sCoreTests[0])) && (n < max); i++) {
        const VrtTest* t = &sCoreTests[i];

        RunOne(&out[n++], t->name, t->fn, t->loops, t->ops, buf, irqOff, irqOn);
    }
    /* past the caches: a chase through 64 KB (every load a miss), read and written in order, uncached, code */
    if (n + 5 <= max) {
        MakeChase(b, 0);
        RunOne(&out[n++], "dc.chase", T_dcChase, 256, 16, buf, irqOff, irqOn);
        RunOne(&out[n++], "dc.seq.read", T_dcSeqRead, 256, 16, buf, irqOff, irqOn);
        RunOne(&out[n++], "dc.seq.write", T_dcSeqWrite, 256, 16, buf, irqOff, irqOn);
        /* uncached: the cache emptied first (so no dirty line can be written back over the chain later), and the
           chain written straight to memory */
        FlushDcache();
        MakeChase((unsigned int*)((unsigned int)b | 0x20000000), 0);
        RunOne(&out[n++], "uncached.chase", T_dcChase, 64, 16, (void*)((unsigned int)buf | 0x20000000), irqOff,
               irqOn);
        RunOne(&out[n++], "ic.miss", T_icMiss, 4, 16384, buf, irqOff, irqOn);
    }
    return n;
}
