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


ZONE_CELL = 200


def write_zones(merged):
    """include/fishmod_lod_zones.h: which zone the camera is in (a grid over the lake, from the zone map) and each
    kind's distance there, above and under water (hundreds of units; 0: not measured there, the global one used)."""
    import importlib.util
    import json

    spec = importlib.util.spec_from_file_location("gen_scatter", os.path.join(ROOT, "fishmod/tools/gen_scatter.py"))
    gs = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gs)
    zones = json.load(open(os.path.join(ROOT, "fishmod/data/lake_hylia_zones.json")))["zones"]
    x0, z0, x1, z1 = -4800, 2200, 2400, 9400
    w, h = (x1 - x0) // ZONE_CELL, (z1 - z0) // ZONE_CELL
    grid = []
    for iz in range(h):
        for ix in range(w):
            x, z = x0 + (ix + 0.5) * ZONE_CELL, z0 + (iz + 0.5) * ZONE_CELL
            best = max(((gs.in_zone(zn, x, z), zn["id"]) for zn in zones), default=(0, 0))
            grid.append(best[1] if best[0] > 0 else 0)
    names = kinds()
    with open(os.path.join(ROOT, "include/fishmod_lod_zones.h"), "w") as f:
        f.write("#ifndef FISHMOD_LOD_ZONES_H\n#define FISHMOD_LOD_ZONES_H\n\n")
        f.write("/*\n * Draw distances per zone of Lake Hylia, from the LOD sweep (fishmod/scripts/lod_sweep.inc.c: ~1100 views\n"
                " * over the lake, each checked on the game's own frames). Made by fishmod/tools/lod_calibrate.py --sweep\n"
                " * --write. The zone is the camera's (fishmod/data/lake_hylia_zones.json on a grid); distances are in\n"
                " * hundreds of units, [zone][under water][kind], 0 where the kind was never in sight from that zone.\n */\n")
        f.write(f"#define FISHMOD_LOD_ZONE_X0 {x0}\n#define FISHMOD_LOD_ZONE_Z0 {z0}\n#define FISHMOD_LOD_ZONE_CELL {ZONE_CELL}\n")
        f.write(f"#define FISHMOD_LOD_ZONE_W {w}\n#define FISHMOD_LOD_ZONE_H {h}\n#define FISHMOD_LOD_ZONE_COUNT {SWEEP_ZONES}\n")
        f.write(f"#define FISHMOD_LOD_ZONE_KINDS {len(names)}\n\n")
        f.write("static const u8 sLodZoneGrid[FISHMOD_LOD_ZONE_W * FISHMOD_LOD_ZONE_H] = {\n")
        for iz in range(h):
            f.write("    " + ", ".join(str(v) for v in grid[iz * w:(iz + 1) * w]) + ",\n")
        f.write("};\n\nstatic const u8 sLodZoneDist[FISHMOD_LOD_ZONE_COUNT][2][FISHMOD_LOD_ZONE_KINDS] = {\n")
        for zn in range(SWEEP_ZONES):
            f.write("    {\n")
            for u in range(2):
                f.write("        { " + ", ".join(str(merged[zn][u][k]) for k in range(len(names))) + " },\n")
            f.write("    },\n")
        f.write("};\n\n#endif\n")
    print("wrote include/fishmod_lod_zones.h")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep", nargs="+")
    ap.add_argument("save", nargs="?")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--log", action="store_true", help="the first tests: kind, distance, pixels changed")
    args = ap.parse_args()
    if args.sweep:
        merged = sweep(args.sweep)
        if args.write:
            write_zones(merged)
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
