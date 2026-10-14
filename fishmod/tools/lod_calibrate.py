#!/usr/bin/env python3
"""Reads the LOD calibration's results (fishmod/scripts/lod_calibrate.inc.c, written to SRAM at 0x7A40) from an
emulator save file and writes include/fishmod_lod.h: how far each habitat prop kind is drawn, above water and under it.

Usage: fishmod/tools/lod_calibrate.py save.ram [--write]
  prints what was measured; --write also writes include/fishmod_lod.h
"""
import argparse
import os
import re
import struct

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
KINDS_MAX = 48  # FISHMOD_LOD_KINDS
OFF = 0x7A40
# never drawn nearer than this (a kind that changed nothing at a stop still shows from closer than any stop looked)
FLOOR = 300


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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("save")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--log", action="store_true", help="the first tests: kind, distance, pixels changed")
    args = ap.parse_args()
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
