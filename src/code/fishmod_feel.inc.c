// Part of fishmod.c (#included there): the fishing's feel tuned from the desktop (test builds: FISHMOD_TESTMODE).
/*
 * The feel's numbers (include/fishmod_feel.h) live in gFishmodFeel. With a SummerCart64, the desktop tuner
 * (fishmod/pc/feel.bat, through `sc64deployer debug`) sends lines of text over its USB; once a frame the game reads
 * any that came in and answers:
 *
 *   set NAME VALUE   that number changed (answered "feel NAME VALUE" with what the table now holds)
 *   get              every number said back ("feel NAME VALUE" each, then "feel-end")
 *   reset            every number back to its default (then as get)
 *
 * The cart's commands (its firmware, sw/controller/src/usb.c and cfg.c): 'u' USB_READ_STATUS answers DATA0 bit 31
 * busy (a read still being moved), bits 7-0 the waiting packet's type, DATA1 its length (0: nothing waiting); 'm'
 * USB_READ (address, length) has the cart put it at an address (here its 8 KB data buffer, 0x1FFE0000), done when
 * 'u' is no longer busy; the game then copies it in by DMA. 'U' USB_WRITE_STATUS bit 31 busy, 'M' USB_WRITE (address,
 * type << 24 | length) sends. `sc64deployer debug` sends each typed line as text (type 1) without its newline, with a
 * 0 byte after it. Without the cart (another cart, an emulator) nothing is read and the numbers stay the defaults.
 */
#if FISHMOD_TESTMODE
#include "carthandle.h"
#include "ultra64/osint.h"
#include "alignment.h"

#define FEEL_TABLE(id, name, def, lo, hi, what) def,
f32 gFishmodFeel[FEEL_MAX] = { FISHMOD_FEEL_LIST(FEEL_TABLE) };
#undef FEEL_TABLE
#define FEEL_NAME(id, name, def, lo, hi, what) name,
static const char* sFeelName[FEEL_MAX] = { FISHMOD_FEEL_LIST(FEEL_NAME) };
#undef FEEL_NAME
#define FEEL_DEF(id, name, def, lo, hi, what) def,
static const f32 sFeelDefault[FEEL_MAX] = { FISHMOD_FEEL_LIST(FEEL_DEF) };
// (each number's range, as the tuner's sliders have it: a number typed past it is held to it, so no setting can make
// the fight divide by nothing or run backwards)
#define FEEL_LO(id, name, def, lo, hi, what) lo,
#define FEEL_HI(id, name, def, lo, hi, what) hi,
static const f32 sFeelLo[FEEL_MAX] = { FISHMOD_FEEL_LIST(FEEL_LO) };
static const f32 sFeelHi[FEEL_MAX] = { FISHMOD_FEEL_LIST(FEEL_HI) };
#undef FEEL_DEF

#define FEEL_SC64_BUFFER 0x1FFE0000 // the cart's 8 KB data buffer; its registers are 64 KB above
#define FEEL_SC64_SCR 0x10000
#define FEEL_SC64_DATA0 0x10004
#define FEEL_SC64_DATA1 0x10008
#define FEEL_SC64_IDENTIFIER 0x1000C
#define FEEL_SC64_KEY 0x10010
#define FEEL_SC64_BUSY (1u << 31)
#define FEEL_SC64_ERROR (1u << 30)
#define FEEL_LINE_MAX 256

static OSPiHandle sFeelPi;
static OSIoMesg sFeelIo;
static OSMesgQueue sFeelQueue;
static OSMesg sFeelMsg;
static s32 sFeelState = -1; // -1 not looked yet, 0 no cart, 1 the cart
static u8 sFeelBuf[FEEL_LINE_MAX + 16] ALIGNED(16);
static s32 sFeelBackoff;

static void Feel_Write(u32 reg, u32 value) {
    osEPiWriteIo(&sFeelPi, reg, value);
}

static u32 Feel_Read(u32 reg) {
    u32 value = 0;

    osEPiReadIo(&sFeelPi, reg, &value);
    return value;
}

/** A cart command; 0 when it finished without an error (its two answers then in d0, d1). */
static s32 Feel_Command(u8 id, u32 arg0, u32 arg1, u32* d0, u32* d1) {
    s32 tries;

    Feel_Write(FEEL_SC64_DATA0, arg0);
    Feel_Write(FEEL_SC64_DATA1, arg1);
    Feel_Write(FEEL_SC64_SCR, id);
    for (tries = 0; tries < 2000; tries++) {
        u32 scr = Feel_Read(FEEL_SC64_SCR);

        if (!(scr & FEEL_SC64_BUSY)) {
            if (scr & FEEL_SC64_ERROR) {
                return -1;
            }
            if (d0 != NULL) {
                *d0 = Feel_Read(FEEL_SC64_DATA0);
            }
            if (d1 != NULL) {
                *d1 = Feel_Read(FEEL_SC64_DATA1);
            }
            return 0;
        }
    }
    sFeelBackoff = 60; // (the cart didn't answer: left alone for a while, the game not slowed)
    return -1;
}

static s32 Feel_Init(void) {
    u32 prevInt;

    if (gCartHandle == NULL) {
        return 0;
    }
    sFeelPi = *gCartHandle;
    sFeelPi.baseAddress = (u32)OS_PHYSICAL_TO_K1(FEEL_SC64_BUFFER);
    bzero(&sFeelPi.transferInfo, sizeof(__OSTranxInfo));
    prevInt = __osDisableInt();
    sFeelPi.next = __osPiTable;
    __osPiTable = &sFeelPi;
    __osRestoreInt(prevInt);
    osCreateMesgQueue(&sFeelQueue, &sFeelMsg, 1);
    Feel_Write(FEEL_SC64_KEY, 0x5F554E4C); // "_UNL"
    Feel_Write(FEEL_SC64_KEY, 0x4F434B5F); // "OCK_"
    return Feel_Read(FEEL_SC64_IDENTIFIER) == 0x53437632; // "SCv2"
}

static void Feel_Dma(void* ram, u32 len, s32 dir) {
    OSMesg msg;

    sFeelIo.hdr.pri = OS_MESG_PRI_NORMAL;
    sFeelIo.hdr.retQueue = &sFeelQueue;
    sFeelIo.dramAddr = ram;
    sFeelIo.devAddr = 0;
    sFeelIo.size = len;
    if (dir == OS_READ) {
        osInvalDCache(ram, len);
    } else {
        osWritebackDCache(ram, len);
    }
    osEPiStartDma(&sFeelPi, &sFeelIo, dir);
    osRecvMesg(&sFeelQueue, &msg, OS_MESG_BLOCK);
}

/** A line to the desktop (text: `sc64deployer debug` prints it, the tuner reads it). */
static void Feel_Say(const char* text) {
    u32 len = strlen(text);
    u32 d0;
    s32 i;

    for (i = 0; i < 200; i++) {
        if (Feel_Command('U', 0, 0, &d0, NULL) != 0) {
            return;
        }
        if (!(d0 & FEEL_SC64_BUSY)) {
            break;
        }
    }
    if ((i == 200) || (len > FEEL_LINE_MAX)) {
        return;
    }
    bcopy(text, sFeelBuf, len);
    Feel_Dma(sFeelBuf, (len + 7) & ~7, OS_WRITE);
    Feel_Command('M', FEEL_SC64_BUFFER, (1u << 24) | len, NULL, NULL);
}

/** A line from the desktop into sFeelBuf (0-ended); false if none came in. */
static s32 Feel_Hear(void) {
    u32 d0;
    u32 d1;
    u32 len;
    u32 n;
    s32 i;

    if (Feel_Command('u', 0, 0, &d0, &d1) != 0) {
        return false;
    }
    len = d1 & 0xFFFFFF;
    if (len == 0) {
        return false;
    }
    // the whole packet into the cart's buffer (else it stays waiting), then as much of it as a line holds
    n = MIN(len, 0x2000);
    if (Feel_Command('m', FEEL_SC64_BUFFER, n, NULL, NULL) != 0) {
        return false;
    }
    for (i = 0; i < 2000; i++) {
        if (Feel_Command('u', 0, 0, &d0, &d1) != 0) {
            return false;
        }
        if (!(d0 & FEEL_SC64_BUSY)) {
            break;
        }
    }
    n = MIN(n, FEEL_LINE_MAX);
    Feel_Dma(sFeelBuf, (n + 7) & ~7, OS_READ);
    sFeelBuf[n] = '\0';
    return true;
}

/** "1.25", "-3", ".5" */
static f32 Feel_Number(const char* p, s32* ok) {
    f32 v = 0.0f;
    f32 scale = 1.0f;
    s32 neg = false;
    s32 digits = 0;

    while (*p == ' ') {
        p++;
    }
    if (*p == '-') {
        neg = true;
        p++;
    }
    while ((*p >= '0') && (*p <= '9')) {
        v = v * 10.0f + (*p++ - '0');
        digits++;
    }
    if (*p == '.') {
        p++;
        while ((*p >= '0') && (*p <= '9')) {
            scale *= 0.1f;
            v += (*p++ - '0') * scale;
            digits++;
        }
    }
    *ok = digits > 0;
    return neg ? -v : v;
}

/** Does p start with the n characters of word? */
static s32 Feel_Starts(const char* p, const char* word, s32 n) {
    s32 i;

    for (i = 0; i < n; i++) {
        if (p[i] != word[i]) {
            return false;
        }
    }
    return true;
}

static void Feel_SayOne(s32 k) {
    char line[64];
    s32 milli = (s32)(gFishmodFeel[k] * 1000.0f + ((gFishmodFeel[k] >= 0.0f) ? 0.5f : -0.5f));
    s32 a = ABS(milli);

    sprintf(line, "feel %s %s%d.%03d", sFeelName[k], (milli < 0) ? "-" : "", a / 1000, a % 1000);
    Feel_Say(line);
}

static void Feel_SayAll(void) {
    s32 k;

    for (k = 0; k < FEEL_MAX; k++) {
        Feel_SayOne(k);
    }
    Feel_Say("feel-end");
}

static void Feel_Line(const char* line) {
    s32 k;

    while (*line == ' ') {
        line++;
    }
    if (Feel_Starts(line, "get", 3)) {
        Feel_SayAll();
        return;
    }
    if (Feel_Starts(line, "reset", 5)) {
        for (k = 0; k < FEEL_MAX; k++) {
            gFishmodFeel[k] = sFeelDefault[k];
        }
        Feel_SayAll();
        return;
    }
    if (Feel_Starts(line, "set ", 4)) {
        const char* p = line + 4;

        while (*p == ' ') {
            p++;
        }
        for (k = 0; k < FEEL_MAX; k++) {
            s32 n = strlen(sFeelName[k]);

            if (Feel_Starts(p, sFeelName[k], n) && (p[n] == ' ')) {
                s32 ok;
                f32 v = Feel_Number(p + n, &ok);

                if (ok && (v == v)) { // (a number, and not "nan")
                    gFishmodFeel[k] = CLAMP(v, sFeelLo[k], sFeelHi[k]);
                }
                Feel_SayOne(k);
                return;
            }
        }
        Feel_Say("feel-unknown");
    }
}

void Fishmod_FeelUpdate(void) {
    s32 lines;

    if (sFeelState < 0) {
        sFeelState = Feel_Init();
        if (sFeelState) {
            Feel_SayAll(); // (a tuner already open gets the build's numbers)
        }
    }
    if (!sFeelState) {
        return;
    }
    if (sFeelBackoff > 0) {
        sFeelBackoff--;
        return;
    }
    for (lines = 0; (lines < 8) && Feel_Hear(); lines++) {
        Feel_Line((const char*)sFeelBuf);
    }
}
#endif
