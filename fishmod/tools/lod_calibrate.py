#!/usr/bin/env python3
"""Reads the LOD calibration's results (fishmod/scripts/lod_calibrate.inc.c, written to SRAM at 0x7A40) from an
emulator save file and writes include/fishmod_lod.h: how far each habitat prop kind is drawn, above water and under it.

Usage: fishmod/tools/lod_calibrate.py save.ram [--write]
  prints what was measured; --write also writes include/fishmod_lod.h
       fishmod/tools/lod_calibrate.py --sweep part0.ram [part1.ram ...]
  the LOD sweep's results (fishmod/scripts/lod_sweep*.inc.c, SRAM 0x0000): per zone, merged over the parts
"""
import argparse
import os
import re
import struct
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sram_map import SRAM  # noqa: E402

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
KINDS_MAX = 48  # FISHMOD_LOD_KINDS
OFF = SRAM["BIG"]
# never drawn nearer than this (a kind that changed nothing at a stop still shows from closer than any stop looked)
FLOOR = 300
# the one-per-kind distances: no nearer than this (what the sampled views didn't see from some other angle)
FLOOR_GLOBAL = 500


def kinds():
    t = open(os.path.join(ROOT, "src/fishmod_data/fishmod_props_kinds.h")).read()
    block = t[t.index("#define FISHMOD_PROP_KINDS(X)"):]
    return re.findall(r"X\((\w+), \w+\)", block.split("\n\n")[0])


def read(path):
    d = open(path, "rb").read()[OFF:OFF + 0x300]
    magic, nk, stops, thr, pix = struct.unpack(">IBBBB", d[:8])
    if magic != 0x464C4F44:
        raise SystemExit("no LOD calibration results in " + path)
    need = struct.unpack(">%dH" % (2 * KINDS_MAX), d[8:8 + 4 * KINDS_MAX])
    o = 8 + 4 * KINDS_MAX
    noise = struct.unpack(">32H", d[o:o + 64])
    verify = struct.unpack(">32H", d[o + 64:o + 128])
    under = d[o + 128:o + 160]
    lc = struct.unpack(">H", d[o + 160:o + 162])[0]
    log = [struct.unpack(">3H", d[o + 162 + 6 * i:o + 168 + 6 * i]) for i in range(min(lc, 64))]
    return nk, stops, thr, pix, need[:KINDS_MAX], need[KINDS_MAX:], noise, verify, under, log


SWEEP_ZONES = 16


def read_sweep(path):
    d = open(path, "rb").read()
    magic, views, done, noisy, empty, tests, nk, nz, part, parts = struct.unpack(">IHHHHIBBBB", d[:0x14])
    if magic != 0x464C5357:
        raise SystemExit("no LOD sweep results in " + path)
    need = d[0x14:0x14 + SWEEP_ZONES * 2 * KINDS_MAX]
    pass_first, splits = struct.unpack(">HH", d[0x614:0x618])
    return dict(views=views, done=done, noisy=noisy, empty=empty, tests=tests, kinds=nk, part=part, parts=parts,
                need=need, pass_first=pass_first, splits=splits)


def sweep(paths):
    names = kinds()
    merged = [[[0] * KINDS_MAX for _ in range(2)] for _ in range(SWEEP_ZONES)]
    for path in paths:
        r = read_sweep(path)
        print(f"{os.path.basename(path)}: part {r['part'] + 1}/{r['parts']}, {r['done']} of ~{r['views'] // r['parts']} "
              f"views ({r['noisy']} noisy, {r['empty']} with nothing in sight), {r['tests']} tests; "
              f"{r['pass_first']} passed on the first, {r['splits']} split")
        for z in range(SWEEP_ZONES):
            for u in range(2):
                for k in range(KINDS_MAX):
                    merged[z][u][k] = max(merged[z][u][k], r["need"][(z * 2 + u) * KINDS_MAX + k])
    for u, label in ((0, "above"), (1, "under")):
        zs = [z for z in range(SWEEP_ZONES) if any(merged[z][u])]
        print(f"\n{label} water, in hundreds of units ('.' never in sight) by zone " + " ".join(f"{z:>3}" for z in zs))
        for k, n in enumerate(names):
            row = [merged[z][u][k] for z in zs]
            if any(row):
                print(f"  {n:14s}" + "".join(f"{(v if v else '.'):>4}" for v in row) + f"   max {max(row) * 100}")
    return merged


def rotate(path):
    d = open(path, "rb").read()[SRAM["LOD_ROTATE"]:SRAM["LOD_ROTATE"] + 0x600]
    magic, stops, yaws = struct.unpack(">IHH", d[:8])
    if magic != 0x46524F54:
        raise SystemExit("no rotation check results in " + path)
    cone = struct.unpack(f">{22 * yaws}H", d[8:8 + 44 * yaws])
    allc = struct.unpack(f">{22 * yaws}H", d[8 + 44 * yaws:8 + 88 * yaws])
    print(f"{stops} stops, {yaws} directions each: pixels the game's frame lacks against a wider view cone (cone), and"
          f" against that with nothing cut by distance (all)")
    for st in range(stops):
        c = cone[st * yaws:(st + 1) * yaws]
        a = allc[st * yaws:(st + 1) * yaws]
        print(f"  stop {st + 1:2d}: cone max {max(c):5d} (dir {c.index(max(c)):2d}), all max {max(a):5d} "
              f"(dir {a.index(max(a)):2d})   cone " + " ".join(f"{v}" for v in c))


def write_global(merged):
    """The sweep's furthest need per kind (over every zone) into include/fishmod_lod.h, keeping the larger of that
    and what it has (the tour stops'). (Per-zone distances were tried and dropped: distances that change with where the camera is make
    props pop as the camera moves or turns; one per kind can't."""
    path = os.path.join(ROOT, "include/fishmod_lod.h")
    text = open(path).read()
    names = kinds()
    have = {m.group(1): [int(m.group(2)), int(m.group(3))]
            for m in re.finditer(r"X\((\w+), (-?\d+), (-?\d+)\)", text)}
    lines = []
    for k, n in enumerate(names):
        need = [max(merged[z][u][k] for z in range(SWEEP_ZONES)) * 100 for u in range(2)]
        old = have.get(n, [-1, -1])
        row = []
        for u in range(2):
            vals = [v for v in (need[u], old[u]) if v > 0]
            row.append(max(max(vals), FLOOR_GLOBAL) if vals else -1)
        if row != [-1, -1]:
            lines.append(f"    X({n}, {row[0]}, {row[1]}) \\")
            print(f"  {n:14s} above {row[0]:>5}  under {row[1]:>5}")
    head = text[:text.index("#define FISHMOD_LOD_DISTANCES(X)")]
    with open(path, "w") as f:
        f.write(head + "#define FISHMOD_LOD_DISTANCES(X) \\\n" + "\n".join(lines) + "\n\n#endif\n")
    print("wrote include/fishmod_lod.h")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep", nargs="+")
    ap.add_argument("--rotate", help="the rotation check's save file (fishmod/scripts/lod_rotate.inc.c)")
    ap.add_argument("save", nargs="?")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--write-global", action="store_true",
                    help="with --sweep: one distance per kind (the furthest any view needed, with what fishmod_lod.h "
                         "has), and no zones")
    ap.add_argument("--log", action="store_true", help="the first tests: kind, distance, pixels changed")
    args = ap.parse_args()
    if args.rotate:
        rotate(args.rotate)
        return
    if args.sweep:
        merged = sweep(args.sweep)
        if args.write_global or args.write:
            write_global(merged)
        return
    nk, stops, thr, pix, above, under, noise, verify, uflag, log = read(args.save)
    names = kinds()
    print(f"{stops} stops; a pixel differs by more than {thr}/31, at most {pix} per kind")
    for i in range(stops):
        print(f"  stop {i + 1:2d} ({'under' if uflag[i] else 'above'}): {noise[i]} restless pixels, "
              f"{verify[i]} changed with every kind at its distance")
    if args.log:
        for k, dist, n in log:
            print(f"    test {names[k] if k < len(names) else k:14s} at {dist:5d}: {n} pixels")
    rows = []
    for k in range(min(nk, len(names))):
        a, u = above[k], under[k]
        if a or u:
            rows.append((names[k], max(a, FLOOR) if a else -1, max(u, FLOOR) if u else -1))
            print(f"  {names[k]:14s} above {a or '-':>5}  under {u or '-':>5}")
    if args.write:
        lines = [f"    X({n}, {a}, {u}) \\" for n, a, u in rows]
        text = open(os.path.join(ROOT, "include/fishmod_lod.h")).read()
        head = text[:text.index("#define FISHMOD_LOD_DISTANCES(X)")]
        with open(os.path.join(ROOT, "include/fishmod_lod.h"), "w") as f:
            f.write(head + "#define FISHMOD_LOD_DISTANCES(X) \\\n" + "\n".join(lines) + "\n\n#endif\n")
        print("wrote include/fishmod_lod.h")


if __name__ == "__main__":
    main()
