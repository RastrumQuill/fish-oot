#!/usr/bin/env python3
"""Reads fish-oot's frame log (FISHMOD_FRAMELOG: 'FFRM' packets over a SummerCart64's USB, saved by
`sc64deployer debug` as binaryout-*.bin) and says where the slow frames' time goes.

Every frame's record (src/code/fishmod_debug_pcprof.inc.c): its period, the CPU's update (by Play_Update's sections
and by actor category) and draw, the RSP, the RDP and its counters, the fish and props, the costliest actor kinds.
A frame is slow when its period is over 52 ms (the 20 fps cap is 50 ms).

Usage: fishmod/tools/framelog.py FOLDER [--csv OUT.csv] [--by-leg]
The same folder's profile packets, by slow and full-speed frames:
  n64prof/tools/n64prof.py report FOLDER --elf ELF --frames slow
"""
import argparse
import collections
import os
import re
import struct
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
WORDS = 52
SLOW_US = 52000
CONDS = ["clear dawn", "clear midday", "clear dusk", "clear night", "rain dawn", "rain midday", "rain dusk",
         "rain night"]


def enum_names(path, enum, prefix):
    text = open(os.path.join(ROOT, path)).read()
    body = re.search(r"typedef enum " + enum + r" \{(.*?)\}", text, re.S).group(1)
    return [m.group(1)[len(prefix):].lower() for m in re.finditer(r"\b(" + prefix + r"\w+)\s*,", body)
            if not m.group(1).endswith("MAX")]


def actor_names():
    out = {}
    for line in open(os.path.join(ROOT, "include", "tables", "actor_table.h")):
        m = re.match(r"/\* (0x[0-9A-Fa-f]+) \*/ DEFINE_ACTOR(?:_INTERNAL)?\((\w+),", line)
        if m:
            out[int(m.group(1), 16)] = m.group(2)
    return out


def records(folder):
    out = []
    for f in sorted(os.listdir(folder)):
        d = open(os.path.join(folder, f), "rb").read()
        if d[:4] != b"FFRM":
            continue
        _, version, count, _ = struct.unpack(">4I", d[:16])
        for i in range(count):
            out.append(struct.unpack_from(f">{WORDS}I", d, 16 + 4 * WORDS * i))
    out.sort(key=lambda r: r[0])
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("folder")
    ap.add_argument("--csv")
    ap.add_argument("--by-leg", action="store_true")
    ap.add_argument("--all-frames", action="store_true", help="every frame, not just the bench's counted ones")
    args = ap.parse_args()
    recs = records(args.folder)
    if not recs:
        sys.exit(f"no frame log in {args.folder}")
    if not args.all_frames and any(r[1] & 1 for r in recs):
        recs = [r for r in recs if r[1] & 1]  # the bench's counted frames (inside its blocks): like for like
        print("(the bench's counted frames only: --all-frames for every frame)")
    sects = enum_names("include/fishmod.h", "FishmodSect", "FISHMOD_SECT_")
    cats = enum_names("include/actor_profile.h", "ActorCategory", "ACTORCAT_")
    actors = actor_names()
    gaps = sum(1 for a, b in zip(recs, recs[1:]) if b[0] != a[0] + 1)
    slow = [r for r in recs if r[2] > SLOW_US]
    fast = [r for r in recs if r[2] <= SLOW_US]
    secs = sum(r[2] for r in recs) / 1e6
    print(f"# Frame log: {len(recs)} frames ({secs / 3600:.2f} h, {len(recs) / secs:.1f} fps), "
          f"{len(slow)} over 52 ms ({100 * len(slow) / len(recs):.0f}%); {gaps} gaps in the frame count\n")

    def mean(rs, i):
        return sum(r[i] for r in rs) / max(len(rs), 1) / 1000.0

    rows = [("frame period", 2), ("CPU update", 3), ("CPU draw", 4), ("RSP graphics", 5), ("RSP audio", 6),
            ("RDP", 7), ("fish update", 12), ("fish draw", 13), ("props draw", 14)]
    rows += [(f"  update: {n}", 16 + i) for i, n in enumerate(sects)]
    rows += [(f"  actors: {n}", 36 + i) for i, n in enumerate(cats)]
    print("| ms per frame | slow frames | full-speed frames | difference |")
    print("|---|---|---|---|")
    for name, i in rows:
        a, b = mean(slow, i), mean(fast, i)
        if a > 0.05 or b > 0.05:
            print(f"| {name} | {a:.2f} | {b:.2f} | {a - b:+.2f} |")
    # the RDP's own shares (its counters: cycles at 62.5 MHz)
    for label, rs in (("slow", slow), ("full-speed", fast)):
        clock = sum(r[8] for r in rs) or 1
        print(f"\nRDP in {label} frames: command buffer busy {100 * sum(r[9] for r in rs) / clock:.0f}%, "
              f"pipeline {100 * sum(r[10] for r in rs) / clock:.0f}%, TMEM {100 * sum(r[11] for r in rs) / clock:.0f}%")
    # which actors cost the most in slow frames
    cost = collections.Counter()
    for r in slow:
        for w in r[48:52]:
            if w:
                cost[w >> 20] += (w & 0xFFFFF)
    print("\nCostliest actor kinds in slow frames (ms per slow frame, where among a frame's top four):")
    for aid, us in cost.most_common(12):
        print(f"  {actors.get(aid, hex(aid)):24s} {us / max(len(slow), 1) / 1000:.2f}")
    # what limits each slow frame: the CPU (update + draw) or the RDP
    lim = collections.Counter()
    for r in slow:
        cpu = r[3] + r[4]
        lim["CPU" if cpu >= r[7] else "RDP"] += 1
    print("\nSlow frames by the longer of CPU (update + draw) and RDP:",
          ", ".join(f"{k} {100 * v / max(len(slow), 1):.0f}%" for k, v in lim.most_common()))
    if args.by_leg:
        print("\n| lap | conditions | leg | frames | fps | over 52 ms |")
        print("|---|---|---|---|---|---|")
        legs = collections.defaultdict(list)
        for r in recs:
            legs[(r[1] >> 16 & 0xFF, r[1] >> 8 & 0xFF, r[1] >> 24)].append(r)
        for (lap, cond, leg), rs in sorted(legs.items()):
            t = sum(r[2] for r in rs) / 1e6
            n_slow = sum(1 for r in rs if r[2] > SLOW_US)
            cname = CONDS[cond] if cond < len(CONDS) else str(cond)
            print(f"| {lap + 1} | {cname} | {leg + 1} | {len(rs)} | {len(rs) / t:.1f} | {100 * n_slow / len(rs):.0f}% |")
    if args.csv:
        with open(args.csv, "w") as f:
            names = (["frame", "route", "period", "cpu_update", "cpu_draw", "rsp", "rsp_audio", "rdp", "dp_clock",
                      "dp_buf", "dp_pipe", "dp_tmem", "fish_update", "fish_draw", "props_draw", "props"]
                     + [f"sect_{n}" for n in sects] + [""] * (20 - len(sects))
                     + [f"cat_{n}" for n in cats] + [""] * (12 - len(cats)) + ["top1", "top2", "top3", "top4"])
            f.write(",".join(names) + "\n")
            for r in recs:
                f.write(",".join(str(v) for v in r) + "\n")
        print(f"\nwrote {args.csv}")


if __name__ == "__main__":
    main()
