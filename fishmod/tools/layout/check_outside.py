#!/usr/bin/env python3
"""Checks that nothing outside the audio driver has moved in the CPU's caches between two builds: every symbol of the
code segment that both have, outside the driver (src/audio/ and the scheduler functions linked with it), must be at
the same address modulo 16 KB if it is code (the instruction cache) or modulo 8 KB if it is data (the data cache),
and the heap must start at the same place modulo 16 KB (actors' code is loaded into it).

    check_outside.py REF_ELF NEW_ELF

Prints each run of symbols that moved (first symbol, how many, by how much); exit status 1 if any did."""
import os, subprocess, sys


def read(elf):
    """(address, type, name, source file) of each symbol; names that occur twice are left out (two files' statics)"""
    syms = []
    count = {}
    for line in subprocess.run(["mips-linux-gnu-nm", "-n", "--defined-only", elf], capture_output=True, text=True).stdout.splitlines():
        p = line.split()
        if len(p) >= 3 and not p[2].startswith(("$", ".L", "L8")):
            syms.append((int(p[0], 16) & 0xFFFFFFFF, p[1], p[2], p[3] if len(p) > 3 else ""))
            count[p[2]] = count.get(p[2], 0) + 1
    return [s for s in syms if count[s[2]] == 1]


def main():
    ref, new = read(sys.argv[1]), read(sys.argv[2])
    rn, nn = {s[2]: s[0] for s in ref}, {s[2]: s[0] for s in new}
    moved = set()
    for line in open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../data/sched_text_slots.txt")):
        p = line.split()
        if len(p) == 2 and not line.startswith("#"):
            moved.add(p[0])
    # the segment's four parts (code, data, read-only data, zeroed data), and in each the stretch that is the driver's:
    # from its first symbol to its last (its objects are together in spec)
    bounds = [rn["_codeSegmentTextStart"], rn["_codeSegmentDataStart"], rn["_codeSegmentRoDataStart"], rn["_codeSegmentBssStart"],
              rn["_codeSegmentBssEnd"]]
    part = lambda a: max(i for i in range(4) if bounds[i] <= a)
    # (the driver's code by its source file; its data by the pins that end it in the new build and the first of its
    # symbols there: src/audio/internal/data.c's)
    text = [(a, n) for a, t, n, f in ref if bounds[0] <= a < bounds[1] and t in "tT"]
    files = subprocess.run(["mips-linux-gnu-addr2line", "-e", sys.argv[1]] + ["0x%x" % a for a, _ in text], capture_output=True,
                           text=True).stdout.splitlines()
    audio = [a for (a, n), f in zip(text, files) if "/src/audio/" in f]
    span = {0: (min(audio), max(audio))}
    for k, first, pin in ((1, "gWaveSamples", "gFishmodAudioPinData"), (2, "sDefaultSequenceRandom", "gFishmodAudioPinRodata"), (3, "gAudioCtx", "gFishmodAudioPinBss")):
        if pin in nn:
            inside = [rn[n] for n in rn if n in nn and nn.get(first, 1 << 32) <= nn[n] < nn[pin] and bounds[k] <= rn[n] < bounds[k + 1]]
            if inside:
                span[k] = (min(inside), max(inside))
    bad = 0
    run = None

    def flush():
        nonlocal run
        if run:
            print("  %08x %-40s %5d symbols  moved %+#x (%#x in the cache)" % run)
        run = None

    for a, t, n, f in ref:
        if n not in nn or not (bounds[0] <= a < bounds[4]) or n.startswith("_"):
            continue
        k = part(a)
        if (k in span and span[k][0] <= a <= span[k][1]) or n in moved:
            continue
        mod = 0x4000 if k == 0 else 0x2000
        d = nn[n] - a
        if d % mod:
            if run and run[3] == d:
                run = (run[0], run[1], run[2] + 1, d, d % mod)
            else:
                flush()
                run = (a, n, 1, d, d % mod)
                bad += 1
        else:
            flush()
    flush()
    # (main.c starts the heap at the next 16 KB boundary after the buffers)
    hr, hn = (rn["_buffersSegmentEnd"] + 0x3FFF) & ~0x3FFF, (nn["_buffersSegmentEnd"] + 0x3FFF) & ~0x3FFF
    print("heap start: %08x, was %08x: %s" % (hn, hr, "same place" if (hn - hr) % 0x4000 == 0 else "MOVED %#x" % ((hn - hr) % 0x4000)))
    bad += (hn - hr) % 0x4000 != 0
    print("moved runs: %d" % bad)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
