#!/usr/bin/env python3
"""Reads fish-oot's frame log (FISHMOD_FRAMELOG: 'FFRM' packets over a SummerCart64's USB, saved by
`sc64deployer debug` as binaryout-*.bin) and says where the slow frames' time goes.

Every frame's record (src/code/fishmod_debug_pcprof.inc.c): its period, the CPU's update (by Play_Update's sections
and by actor category) and draw, the RSP, the RDP and its counters, the fish and props, the costliest actor kinds.
A frame is slow when its period is over 52 ms (the 20 fps cap is 50 ms).

Usage: fishmod/tools/framelog.py FOLDER [--csv OUT.csv] [--by-leg] [--spikes]
--spikes: by area (each leg of the route, all laps together): the frame period's mean and standard deviation, what
explains the spread (each part's covariance with the period, as a share of its variance), and the spikes (frames over
the area's mean + 2 standard deviations): what's different in them, and which actors.
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


def stop_names():
    text = open(os.path.join(ROOT, "fishmod", "scripts", "tour.inc.c")).read()
    body = text[text.index("sTourStops[]"):]
    return re.findall(r'^\s*\{ \{[^"]*"([^"]+)"', body, re.M)


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
    ap.add_argument("--spikes", action="store_true", help="spread and spikes by area of the route")
    ap.add_argument("--keep-sends", action="store_true",
                    help="keep the frames that sent a USB packet (about 9 ms more each: the measurement's own cost)")
    ap.add_argument("--all-frames", action="store_true", help="every frame, not just the bench's counted ones")
    args = ap.parse_args()
    recs = records(args.folder)
    if not recs:
        sys.exit(f"no frame log in {args.folder}")
    if not args.all_frames and any(r[1] & 1 for r in recs):
        recs = [r for r in recs if r[1] & 1]  # the bench's counted frames (inside its blocks): like for like
        print("(the bench's counted frames only: --all-frames for every frame)")
    sects = enum_names("include/fishmod.h", "FishmodSect", "FISHMOD_SECT_")
    if not args.keep_sends:
        # the frames that packed and sent a USB packet: the bench's own section (fm_debug) runs about 9 ms longer
        # in them, a spike the measurement makes (one frame in ten or so): left out, so the game's own spikes show
        n = len(recs)
        recs = [r for r in recs if r[16 + sects.index("fm_debug")] <= 5000]
        print(f"(left out: {n - len(recs)} frames that sent a USB packet, the profiler's own cost: --keep-sends to keep them)")
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
    if args.spikes:
        spikes(recs, rows, actors)
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


def spikes(recs, rows, actors):
    """Spread and spikes by area: each leg of the route, every lap's pass through it together."""
    stops = stop_names()
    parts = [(n.strip(), i) for n, i in rows if i != 2]
    legs = collections.defaultdict(list)
    for r in recs:
        legs[r[1] >> 24].append(r)

    def stats(xs):
        m = sum(xs) / len(xs)
        return m, (sum((x - m) ** 2 for x in xs) / max(len(xs) - 1, 1)) ** 0.5

    def label(leg):
        if not stops:
            return f"leg {leg + 1}"
        return f"{leg + 1}: {stops[leg % len(stops)]} -> {stops[(leg + 1) % len(stops)]}"

    total_lost = 0
    summary = []
    for leg, rs in legs.items():
        per = [r[2] for r in rs]
        m, sd = stats(per)
        lost = sum(max(p - 50000, 0) for p in per) / 1000.0  # ms over 50 per frame, summed
        total_lost += lost
        summary.append((lost, leg, rs, m, sd))
    summary.sort(reverse=True)
    print("\n## Areas, by time lost over 50 ms a frame (all laps)\n")
    print("| area | frames | mean ms | std dev ms | over 52 ms | spikes (>mean+2sd) | time lost | biggest part of the spread |")
    print("|---|---|---|---|---|---|---|---|")
    details = []
    for lost, leg, rs, m, sd in summary:
        per = [r[2] for r in rs]
        var = sum((p - m) ** 2 for p in per) or 1
        expl = []
        for name, i in parts:
            if name in ("CPU update", "CPU draw", "RSP graphics", "RSP audio", "RDP") or name.startswith(("update", "actors")):
                mi = sum(r[i] for r in rs) / len(rs)
                cov = sum((r[i] - mi) * (p - m) for r, p in zip(rs, per))
                expl.append((cov / var, name))
        expl.sort(reverse=True)
        big = ", ".join(f"{n} {100 * c:.0f}%" for c, n in [e for e in expl if e[1].startswith(("update", "actors"))][:2])
        sp = [r for r in rs if r[2] > m + 2 * sd]
        n_slow = sum(1 for p in per if p > SLOW_US)
        print(f"| {label(leg)} | {len(rs)} | {m / 1000:.1f} | {sd / 1000:.1f} | {100 * n_slow / len(rs):.0f}% | "
              f"{len(sp)} | {lost / 1000:.1f} s ({100 * lost / max(total_lost, 1):.0f}%) | {big} |")
        details.append((leg, rs, sp, m))
    print("\n## Spikes in the five areas losing the most time\n")
    for leg, rs, sp, m in details[:5]:
        if not sp:
            continue
        print(f"### {label(leg)}: {len(sp)} spikes, {sum(r[2] for r in sp) / len(sp) / 1000:.1f} ms on average "
              f"(the area's mean {m / 1000:.1f} ms)\n")
        diff = []
        for name, i in parts:
            if name in ("CPU update", "CPU draw", "RSP graphics", "RDP") or name.startswith(("update", "actors", "fish", "props")):
                a = sum(r[i] for r in sp) / len(sp) / 1000
                b = sum(r[i] for r in rs) / len(rs) / 1000
                if a - b > 0.3:
                    diff.append((a - b, name, a, b))
        diff.sort(reverse=True)
        print("| part | in spikes | area mean | more |")
        print("|---|---|---|---|")
        for d, name, a, b in diff[:8]:
            print(f"| {name} | {a:.1f} | {b:.1f} | +{d:.1f} |")
        ca = collections.Counter()
        cb = collections.Counter()
        for group, c in ((sp, ca), (rs, cb)):
            for r in group:
                for w in r[48:52]:
                    if w:
                        c[w >> 20] += (w & 0xFFFFF)
        more = sorted(((ca[k] / len(sp) - cb[k] / len(rs)) / 1000, k) for k in ca)[::-1][:4]
        print("\nActors costing more in the spikes (ms a frame, spikes vs the area): " + ", ".join(
            f"{actors.get(k, hex(k))} {ca[k] / len(sp) / 1000:.1f} vs {cb[k] / len(rs) / 1000:.1f}" for d, k in more if d > 0.2)
            + "\n")


if __name__ == "__main__":
    main()
