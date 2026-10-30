#!/usr/bin/env python3
"""Reads the LOD tuning's saves (fishmod/scripts/lod_tune.inc.c, one per part: SRAM 0x0000) and merges them: per plant
kind, time of day and above or under water, the shortest distance that changed no view, for the cut (how far it's
drawn), the single card and the half-size texture. A setting needing the most of any view is what's safe everywhere.

Usage: fishmod/tools/lod_tune.py part00.sav [part01.sav ...] [--write]   (or a results folder: every *.sav in it)
  --write: writes include/fishmod_lod_tune.h (the tables fishmod_scatter.c uses)
"""
import argparse
import glob
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import prop_kinds as pk  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
KINDS_MAX = 48
TIMES = ["dawn 6:30", "noon", "dusk 18:30", "midnight"]
KNOBS = ["cut", "single card", "half texture"]
STEP = 50
STEPS = 60


def kinds():
    return [k.upper() for k in pk.GAME]


def read(path):
    d = open(path, "rb").read()
    magic, slots, done, noisy, empty, tests, nk, nz, part, parts = struct.unpack(">IHHHHIBBBB", d[:0x14])
    if magic != 0x464C5357:
        raise SystemExit(f"no LOD tuning results in {path}")
    n = len(TIMES) * 2 * len(KNOBS) * KINDS_MAX
    need = d[0x14:0x14 + n]
    return dict(slots=slots, done=done, noisy=noisy, empty=empty, tests=tests, part=part, parts=parts, need=need)


def idx(t, u, k, kind):
    return ((t * 2 + u) * len(KNOBS) + k) * KINDS_MAX + kind


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("saves", nargs="+")
    ap.add_argument("--write", action="store_true")
    args = ap.parse_args()
    paths = []
    for p in args.saves:
        paths += sorted(glob.glob(os.path.join(p, "**", "*.sav"), recursive=True)) if os.path.isdir(p) else [p]
    runs = [read(p) for p in paths]
    names = kinds()
    total = sum(r["done"] for r in runs)
    slots = runs[0]["slots"]
    parts = {r["part"] for r in runs}
    print(f"{len(runs)} saves, parts {sorted(parts)} of {runs[0]['parts']}: {total} of {slots} view-times measured "
          f"({100 * total / max(slots, 1):.0f}%); {sum(r['noisy'] for r in runs)} skipped as noisy, "
          f"{sum(r['empty'] for r in runs)} with no props in sight; {sum(r['tests'] for r in runs)} tests")
    if len(parts) < runs[0]["parts"]:
        print(f"  (missing parts: {sorted(set(range(runs[0]['parts'])) - parts)})")
    need = [max(r["need"][i] for r in runs) for i in range(len(runs[0]["need"]))]

    for u, where in enumerate(("above water", "under water")):
        print(f"\n{where}: distances in units (- never in sight; 3000 = needed all the way)")
        print(f"{'kind':14s} " + " | ".join(f"{kn:^23s}" for kn in KNOBS))
        print(f"{'':14s} " + " | ".join(" ".join(f"{t.split()[0][:5]:>5s}" for t in TIMES) for _ in KNOBS))
        for kind, name in enumerate(names):
            row = []
            seen = False
            for k in range(len(KNOBS)):
                cells = []
                for t in range(len(TIMES)):
                    v = need[idx(t, u, k, kind)]
                    seen |= v > 0
                    cells.append("    -" if v == 0 else f"{v * STEP:5d}")
                row.append(" ".join(cells))
            if seen:
                print(f"{name.lower():14s} " + " | ".join(row))

    if args.write:
        out = os.path.join(ROOT, "include", "fishmod_lod_tune.h")
        with open(out, "w") as f:
            f.write("#ifndef FISHMOD_LOD_TUNE_H\n#define FISHMOD_LOD_TUNE_H\n\n")
            f.write("/*\n * Per prop kind: the shortest distances that changed no view of the LOD tuning (fishmod/tools/"
                    "lod_tune.py,\n * from fishmod/scripts/lod_tune.inc.c run on the PC), in units, per time of day "
                    f"({', '.join(TIMES)}),\n * above and under water: the cut, the single card, the half-size "
                    "texture. 0: never in sight (the defaults stay).\n" + f" * {total} of {slots} view-times measured.\n */\n")
            f.write(f"#define FISHMOD_LOD_TUNE_TIMES {len(TIMES)}\n")
            f.write("// [kind][time][under][setting]\n")
            f.write(f"static const u16 sLodTuned[][{len(TIMES)}][2][{len(KNOBS)}] = {{\n")
            for kind, name in enumerate(names):
                cells = ", ".join("{ " + ", ".join("{ " + ", ".join(str(need[idx(t, u, k, kind)] * STEP)
                                                                     for k in range(len(KNOBS))) + " }"
                                                   for u in range(2)) + " }" for t in range(len(TIMES)))
                f.write(f"    /* {name} */ {{ {cells} }},\n")
            f.write("};\n\n#endif\n")
        print(f"\nwrote {os.path.relpath(out, ROOT)}")


if __name__ == "__main__":
    main()
