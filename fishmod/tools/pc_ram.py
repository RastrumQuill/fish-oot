#!/usr/bin/env python3
"""The PC profile between two RAM dumps of an emulator run (a FISHMOD_PCPROF build without a SummerCart64: the samples
count into the table in RAM, fishmod_debug_pcprof.inc.c sPcTable), by function. Times are in the game's own cycles
(the sampler runs off the CPU's count register), cache stalls included, as far as the emulator models them.

Usage: pc_ram.py ELF RAM1 RAM2 [--top N] [--tag T] [--func NAME]
  RAM1, RAM2: 8 MB dumps from 0x80000000 (the later one second); --tag: only the samples taken under gFishmodPcTag
  T (FISHMOD_PCTAG_*), 0 for the untagged; --func: that function's 32-byte buckets
"""
import argparse
import bisect
import collections
import struct
import subprocess

TABLE = 8192  # PCPROF_HASH
SHIFT = 5     # N64PROF_BUCKET_SHIFT


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("elf")
    ap.add_argument("ram1")
    ap.add_argument("ram2")
    ap.add_argument("--top", type=int, default=40)
    ap.add_argument("--tag", type=int)
    ap.add_argument("--func")
    a = ap.parse_args()
    lines = subprocess.run(["mips-linux-gnu-nm", "-n", a.elf], capture_output=True, text=True).stdout.splitlines()
    syms = [(int(l.split()[0], 16), l.split()[2]) for l in lines if len(l.split()) == 3 and l.split()[1] in "tTwW"]
    addrs = [x for x, _ in syms]
    var = {l.split()[2]: int(l.split()[0], 16) for l in lines if len(l.split()) == 3}

    def table(path):
        d = open(path, "rb").read()
        p = struct.unpack_from(">I", d, var["sPcTable"] & 0xFFFFFF)[0] & 0xFFFFFF
        out = collections.Counter()
        for i in range(TABLE):
            k, c = struct.unpack_from(">II", d, p + 8 * i)
            if c:
                out[k] += c
        return out

    t1, t2 = table(a.ram1), table(a.ram2)
    per_fn = collections.Counter()
    per_bucket = collections.Counter()
    tags = collections.Counter()
    for k, c in t2.items():
        c -= t1.get(k, 0)
        if c <= 0:
            continue
        tag = k & ((1 << SHIFT) - 1)
        tags[tag] += c
        if a.tag is not None and tag != a.tag:
            continue
        pc = k & ~((1 << SHIFT) - 1)
        i = bisect.bisect_right(addrs, pc) - 1
        name = syms[i][1] if i >= 0 else "?"
        per_fn[name] += c
        per_bucket[(name, pc)] += c
    total = sum(tags.values())
    print("samples %d (%s)" % (total, ", ".join("tag %d: %d" % kv for kv in sorted(tags.items()))))
    shown = sum(per_fn.values())
    if a.func:
        for (name, pc), c in sorted(per_bucket.items()):
            if a.func in name:
                print("%-40s %08x %7d %6.2f%%" % (name, pc, c, 100.0 * c / shown))
        return
    for name, c in per_fn.most_common(a.top):
        print("%-44s %7d %6.2f%%" % (name, c, 100.0 * c / shown))


if __name__ == "__main__":
    main()
