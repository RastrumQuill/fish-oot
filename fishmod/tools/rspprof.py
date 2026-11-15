#!/usr/bin/env python3
"""Where the audio microcode's RSP time goes, by command: reads the profile ares writes with ARES_RSPPROF=<file>
(fishmod/tools/ares-rspprof.patch: every RSP instruction and its clocks counted by PC, on the interpreter, so the
counts are exact and the same from run to run) and adds it up by the command handler each PC belongs to (the
microcode's own dispatch table).

    ARES_RSPPROF=prof.bin ARES_RSPPROF_KEY=00400008 fishmod/tools/ares_run.sh 0 ROM 90
    fishmod/tools/rspprof.py prof.bin [--ticks N] [--text aspMainText --data aspMainData]

ARES_RSPPROF_KEY is the audio microcode's word at IMEM 0x80 (aspMain is its own boot code, loaded at IMEM 0: its
text's word at 0x80). --ticks: the audio updates the run made (gFishmodAudLogCount in an audlog build), for per-tick
numbers. A handler's range runs to the next handler's start, so a routine shared by several commands (the DMA and its
wait, after LOADADPCM) is counted with the handler before it.
"""
import argparse
import bisect
import collections
import struct

NAMES = {0: "SPNOOP", 1: "ADPCM", 2: "CLEARBUFF", 3: "UNK3", 4: "ADDMIXER", 5: "RESAMPLE", 6: "RESAMPLE_ZOH",
         7: "FILTER", 8: "SETBUFF", 9: "DUPLICATE", 10: "DMEMMOVE", 11: "LOADADPCM", 12: "MIXER", 13: "INTERLEAVE",
         14: "HILOGAIN", 15: "SETLOOP", 17: "INTERL", 18: "ENVSETUP1", 19: "ENVMIXER", 20: "LOADBUFF", 21: "SAVEBUFF",
         22: "ENVSETUP2", 23: "S8DEC", 25: "UNK19"}


def dispatch_table(data):
    """The longest run of 16-bit IMEM addresses in the microcode's data: one per command."""
    w = struct.unpack(">%dH" % (len(data) // 2), data[:len(data) & ~1])
    best = (0, 0)
    for i in range(len(w)):
        j = i
        while j < len(w) and 0x1000 <= w[j] < 0x2000 and w[j] % 4 == 0:
            j += 1
        if j - i > best[1] - best[0]:
            best = (i, j)
    return w[best[0]:best[1]]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("prof")
    ap.add_argument("--ticks", type=int, default=0)
    ap.add_argument("--data", default="extracted/ntsc-1.2/incbin/aspMainData")
    ap.add_argument("--kind", type=int, default=1)
    ap.add_argument("--pcs", type=int, default=0, help="also the N hottest PCs")
    a = ap.parse_args()
    starts = {}
    for op, addr in enumerate(dispatch_table(open(a.data, "rb").read())):
        starts.setdefault(addr - 0x1000, []).append(NAMES.get(op, "op%d" % op))
    bounds = sorted(starts)
    d = open(a.prof, "rb").read()
    clocks, instrs, pcs = collections.Counter(), collections.Counter(), []
    for i in range(0, len(d) - 23, 24):
        kind, pc, n, c = struct.unpack_from("<IIQQ", d, i)
        if kind != a.kind:
            continue
        k = bisect.bisect_right(bounds, pc) - 1
        name = "%s @%03x" % ("/".join(starts[bounds[k]])[:30], bounds[k]) if k >= 0 else "boot, dispatch @000"
        clocks[name] += c
        instrs[name] += n
        pcs.append((c, n, pc))
    tc, ti = sum(clocks.values()), sum(instrs.values())
    if not ti:
        raise SystemExit("no samples of kind %d (is ARES_RSPPROF_KEY the microcode's word at IMEM 0x80?)" % a.kind)
    per = a.ticks or 1
    print("%d instructions, %d clocks%s" % (ti, tc, ", per tick %.0f and %.0f" % (ti / per, tc / per) if a.ticks else ""))
    for name, c in clocks.most_common():
        print("%5.1f%% clocks %5.1f%% instr %9.0f instr%s  %s" %
              (100 * c / tc, 100 * instrs[name] / ti, instrs[name] / per, "/tick" if a.ticks else "", name))
    for c, n, pc in sorted(pcs, reverse=True)[:a.pcs]:
        print("  pc %03x: %5.2f%% clocks, %d instr" % (pc, 100 * c / tc, n))


if __name__ == "__main__":
    main()
