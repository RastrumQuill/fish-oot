#!/usr/bin/env python3
"""Where the CPU's time goes, by function, exactly: reads the profile ares writes with ARES_CPUPROF=<file>
(fishmod/tools/ares-cpuprof.patch: the CPU on the interpreter, every instruction counted by PC with its clocks and
the data cache lines it filled and wrote back, and the instruction cache lines filled) and adds it up by the ELF's functions. The counts are the emulator's
model (a data cache fill or write-back is 40 cycles, an instruction cache fill 48), the same from run to run for the same ROM and inputs.

    ARES_CPUPROF=cpu.bin fishmod/tools/ares_run.sh 0 ROM 90
    fishmod/tools/cpuprof.py cpu.bin --elf ELF [--per N] [--match REGEX] [--top N]

--per N: divide by N for per-update numbers; --per-calls FUNC: by the times FUNC was entered (AudioThread_UpdateImpl:
per audio update). --match: only functions whose name
or source file matches (the totals still count everything). Cycles are clocks / 2 (the VR4300's 93.75 MHz).
"""
import argparse
import bisect
import collections
import re
import struct
import subprocess


def functions(elf):
    out = subprocess.run(["mips-linux-gnu-nm", "-n", "--defined-only", elf], capture_output=True, text=True).stdout
    syms = []
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3 and p[1] in "tT" and not p[2].startswith(("$", ".L", "L8")):
            syms.append((int(p[0], 16) & 0xFFFFFFFF, p[2]))
    return syms


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("prof")
    ap.add_argument("--elf", required=True)
    ap.add_argument("--per", type=float, default=0)
    ap.add_argument("--per-calls", default="", help="divide by the times this function was entered")
    ap.add_argument("--match", default="")
    ap.add_argument("--top", type=int, default=40)
    a = ap.parse_args()
    syms = functions(a.elf)
    addrs = [s[0] for s in syms]
    agg = collections.defaultdict(lambda: [0, 0, 0, 0, 0, 0])
    entry = dict((name, addr) for addr, name in syms).get(a.per_calls)
    d = open(a.prof, "rb").read()
    for i in range(0, len(d) - 27, 28):
        pc, n, clocks, fills, wb, ifills = struct.unpack_from("<IIQIII", d, i)
        if pc == entry:
            a.per = n
        k = bisect.bisect_right(addrs, pc) - 1
        e = agg[syms[k][1] if k >= 0 else "?"]
        if k >= 0 and pc == addrs[k]:
            e[4] += n  # (its first instruction: the times it was entered)
        e[0] += n
        e[1] += clocks
        e[2] += fills
        e[3] += wb
        e[5] += ifills
    tot = [sum(e[j] for e in agg.values()) for j in (0, 1, 2, 3, 5)]
    per = a.per or 1
    unit = " per update" if a.per else ""
    print("all: %d instructions, %d cycles, %d data fills, %d write-backs, %d code fills" %
          (tot[0], tot[1] // 2, tot[2], tot[3], tot[4]))
    rows = [(n, e) for n, e in agg.items() if not a.match or re.search(a.match, n)]
    sel = [sum(e[j] for _, e in rows) for j in (0, 1, 2, 3, 5)]
    if a.match:
        print("matched: %.1f%% of the cycles; %.0f instructions, %.0f cycles, %.1f data fills, %.1f write-backs, "
              "%.1f code fills%s" % (100 * sel[1] / tot[1], sel[0] / per, sel[1] / 2 / per, sel[2] / per, sel[3] / per,
                                     sel[4] / per, unit))
    print("%7s %10s %10s %8s %8s %8s %8s  %s" % ("share", "instr", "cycles", "fills", "wbacks", "ifills", "calls",
                                               "function" + unit))
    for n, e in sorted(rows, key=lambda r: -r[1][1])[:a.top]:
        print("%6.2f%% %10.0f %10.0f %8.1f %8.1f %8.1f %8.1f  %s" % (100 * e[1] / tot[1], e[0] / per, e[1] / 2 / per,
                                                                     e[2] / per, e[3] / per, e[5] / per, e[4] / per, n))


if __name__ == "__main__":
    main()
