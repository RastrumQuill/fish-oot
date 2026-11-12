#!/usr/bin/env python3
"""Reads the profiled stress bench (fishmod/scripts/stress_prof.inc.c): for each bench (fish, then plants) and step, the
count of fish or plants, the frame rate, where the frame's time went (CPU update and draw, RSP, RDP, the fish's own
update and draw, the props' drawing) and, from the profile, the functions that cost the most; then the largest count
that held 20 fps.

Usage: fishmod/tools/stress_prof.py FOLDER --elf ELF [--top N] [--csv OUT.csv]
FOLDER: what `sc64deployer debug` saved (binaryout-*.bin), or the morning runner's packs (packets-NNN.zip, unpacked
there first). ELF: the build's (build/ntsc-1.2/oot-ntsc-1.2.elf right after building the same ROM).

Frame records carry the stress tag in word 1 (bench << 24 | step << 16 | pass << 8 | measured); the frame numbers
start again when the bench moves on to the plants, so records and profile chunks are taken in the order they arrived.
A profile chunk goes to a step when nearly all its frames are that step's.
"""
import argparse
import collections
import os
import re
import struct
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "..", "n64prof", "tools"))
import framelog  # noqa: E402
import n64prof  # noqa: E402

BENCHES = {1: "fish", 2: "plants"}
FULL_SPEED_US = 50800


def arrival(folder):
    """The folder's packets in the order they arrived (sc64deployer's names sort that way)."""
    return sorted(f for f in os.listdir(folder) if f.startswith("binaryout"))


def frame_records(folder):
    """Every frame record, in arrival order, with its segment (0 the fish bench's frames, 1 after the move...)."""
    out = []
    seg = 0
    last = -1
    for f in arrival(folder):
        d = open(os.path.join(folder, f), "rb").read()
        if d[:4] != b"FFRM":
            continue
        _, version, count, _ = struct.unpack(">4I", d[:16])
        words = 58 if version & 0x10000 else (56 if (version & 0xFF) >= 2 else 52)
        for i in range(count):
            rec = struct.unpack_from(f">{words}I", d, 16 + 4 * words * i)
            if rec[0] + 200 < last:
                seg += 1
            last = rec[0]
            out.append((seg, rec))
    return out


def chunks(folder):
    """The profile's chunks in order (by their number), each with its segment and the frame it was sent at."""
    found = {}
    for f in arrival(folder):
        p = os.path.join(folder, f)
        with open(p, "rb") as fh:
            if fh.read(4) != struct.pack(">I", n64prof.MAGIC):
                continue
        head, buckets = n64prof.read_dump(p)
        found.setdefault(head["dumps"], (head["u1"], buckets))
    out = []
    seg = 0
    last = -1
    for n in sorted(found):
        frame, buckets = found[n]
        if frame + 200 < last:
            seg += 1
        last = frame
        out.append((seg, frame, buckets))
    return out


def stress_lines(folder):
    """The FISHMOD-STRESS lines: in text packets among the binaryouts (ares), or in the debug.log the morning runner
    keeps beside them (the console: sc64deployer prints text packets instead of saving them)."""
    out = []
    paths = [os.path.join(folder, f) for f in arrival(folder)]
    for d in (folder, os.path.dirname(os.path.abspath(folder))):
        paths += [os.path.join(d, f) for f in sorted(os.listdir(d)) if f.endswith(".log")]
    seen = set()
    for p in paths:
        t = open(p, "rb").read().decode("utf-8", "replace")
        for m in re.finditer(r"FISHMOD-STRESS (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)", t):
            line = tuple(int(m.group(k)) for k in range(1, 8))
            if line not in seen:
                seen.add(line)
                out.append(line)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("folder")
    ap.add_argument("--elf", required=True)
    ap.add_argument("--top", type=int, default=6)
    ap.add_argument("--csv")
    args = ap.parse_args()
    folder = args.folder
    for f in os.listdir(folder):
        if f == "packets.zip" or (f.startswith("packets-") and f.endswith(".zip")):
            zipfile.ZipFile(os.path.join(folder, f)).extractall(folder)

    recs = frame_records(folder)
    if not recs:
        sys.exit(f"no frame log in {folder}")
    # each step's measured frames (bench, step): the counts come from the FISHMOD-STRESS lines
    steps = collections.defaultdict(list)
    by_frame = {}
    for seg, r in recs:
        by_frame[(seg, r[0])] = r
        bench = r[1] >> 24
        if bench in BENCHES and (r[1] & 1):
            steps[(bench, (r[1] >> 16) & 0xFF)].append(r)
    counts = {}
    passes = collections.defaultdict(list)
    for bench, pss, step, count, frames, mean, slow in stress_lines(folder):
        counts[(bench, step)] = count
        passes[(bench, step)].append((frames, mean, slow))

    # the profile: each chunk to the step nearly all its frames belong to
    acc = collections.defaultdict(collections.Counter)
    prev = {}
    for seg, frame, buckets in chunks(folder):
        start = prev.get(seg, 0)
        prev[seg] = frame
        tags = collections.Counter()
        for k in range(start + 1, frame + 1):
            r = by_frame.get((seg, k))
            if r is not None and (r[1] >> 24) in BENCHES:
                tags[(r[1] >> 24, (r[1] >> 16) & 0xFF)] += 1
        if not tags:
            continue
        key, n = tags.most_common(1)[0]
        if n >= 0.9 * sum(tags.values()):
            acc[key].update(n64prof.untag(buckets, "all"))
    names = {}
    if acc:
        expanded = {k: n64prof.expand(b, 5) for k, b in acc.items()}
        frames = n64prof.symbolize(args.elf, set().union(*[set(e) for e in expanded.values()]))
        for k, e in expanded.items():
            funcs, unknown = n64prof.by_function(e, frames)
            names[k] = (funcs, sum(e.values()))

    def mean(rs, i):
        return sum(r[i] for r in rs) / len(rs) / 1000.0

    rows = []
    for bench in sorted(BENCHES):
        keys = sorted(k for k in steps if k[0] == bench)
        if not keys:
            continue
        what = BENCHES[bench]
        print(f"## The {what} bench\n")
        print(f"| {what} | frames | fps | on screen | late pictures | over 52 ms | CPU update | CPU draw | RSP | RDP | fish update | fish draw | "
              f"props draw | limited by | costliest functions (ms a frame) |")
        print("|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|")
        best = None
        for k in keys:
            rs = steps[k]
            period = mean(rs, 2)
            slow = sum(1 for r in rs if r[2] > 52000)
            cpu = mean(rs, 3) + mean(rs, 4)
            rdp = mean(rs, 7)
            # what reached the screen (words 56-57: pictures and the longest held, refreshes): with run-ahead's third
            # frame buffer, a frame period over 52 ms is often only wobble the screen never shows
            pictures = sum(r[56] >> 16 for r in rs)
            refreshes = sum(r[57] for r in rs)
            late = sum(1 for r in rs if (r[56] & 0xFFFF) >= 4)
            screen = 60.0 * pictures / refreshes if refreshes else 0.0
            if pictures:
                full = (screen >= 19.7) and (late * 20 <= pictures)
            else:
                full = (period * 1000 <= FULL_SPEED_US) and (slow * 20 <= len(rs))
            if full:
                best = counts.get(k, best)
            top = ""
            if k in names:
                funcs, total = names[k]
                ms_per_sample = period / (total / len(rs)) if total else 0  # (the step's own frame time per sample)
                top = ", ".join(f"{n} {c * ms_per_sample / len(rs):.1f}" for n, c in funcs.most_common(args.top))
            row = {"bench": what, "count": counts.get(k, k[1]), "frames": len(rs), "fps": 1000 / period,
                   "screenFps": screen, "late": late,
                   "slow": slow, "cpuUpdate": mean(rs, 3), "cpuDraw": mean(rs, 4), "rsp": mean(rs, 5), "rdp": rdp,
                   "fishUpdate": mean(rs, 12), "fishDraw": mean(rs, 13), "propsDraw": mean(rs, 14),
                   "limit": "CPU" if cpu >= rdp else "RDP", "top": top}
            rows.append(row)
            print(f"| {row['count']} | {len(rs)} | {row['fps']:.1f} | {screen:.1f} | {late} | {slow} | {row['cpuUpdate']:.1f} | "
                  f"{row['cpuDraw']:.1f} | {row['rsp']:.1f} | {rdp:.1f} | {row['fishUpdate']:.1f} | "
                  f"{row['fishDraw']:.1f} | {row['propsDraw']:.1f} | {row['limit']} | {top} |")
        print(f"\nThe most {what} at full speed (on screen: 19.7 fps or more, at most 1 picture in 20 late): {best}\n")
    if args.csv:
        import csv

        with open(args.csv, "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
            w.writeheader()
            w.writerows(rows)


if __name__ == "__main__":
    main()
