#!/usr/bin/env python3
"""How many props each tour stop draws, from a placement (gen_scatter.py's), the way fishmod_scatter.c picks them:
in the view cone and within each kind's distance (above water the set distances, under water the tuned ones). For
judging placement changes against the budget without running the game: near (within 500) and far counts per stop.

Reads the game's placement data (src/fishmod_data/fishmod_sc_spot06.c: run gen_scatter.py --c after changing rules).
Usage: fishmod/tools/scatter_budget.py [--save NOW.json] [--compare OLD.json] [--above-only]
"""
import argparse
import importlib.util
import json
import math
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))


def load(n):
    s = importlib.util.spec_from_file_location(n, os.path.join(ROOT, "fishmod", "tools", n + ".py"))
    m = importlib.util.module_from_spec(s)
    s.loader.exec_module(m)
    return m


def stops():
    src = open(os.path.join(ROOT, "fishmod", "scripts", "tour.inc.c")).read()
    out = []
    for m in re.finditer(r"\{ \{ (-?\d+), (-?\d+), (-?\d+) \}, \{ (-?\d+), (-?\d+), (-?\d+) \}, \"([^\"]+)\"", src):
        v = list(map(int, m.groups()[:6]))
        out.append((v[:3], v[3:], m.group(7)))
    return out


def distances():
    """Per kind: (above, under) draw distance, as fishmod_scatter.c ends up with them (under: the tuned, at most)."""
    sc = open(os.path.join(ROOT, "src/overlays/actors/ovl_En_Fishmod/fishmod_scatter.c")).read()
    defaults = {k: 1600.0 for k in []}
    small = re.findall(r"sKinds\[FISHMOD_PROP_(\w+)\]\.drawDist = DIST_SMALL", sc)
    mid = re.findall(r"sKinds\[FISHMOD_PROP_(\w+)\]\.drawDist = DIST_MID", sc)
    lod = dict((k, (int(a), int(u))) for k, a, u in
               re.findall(r"X\((\w+), (-?\d+), (-?\d+)\)", open(os.path.join(ROOT, "include/fishmod_lod.h")).read()))
    kinds = re.findall(r"X\((\w+), \w+\)", open(os.path.join(ROOT, "src/fishmod_data/fishmod_props_kinds.h")).read()
                       .split("#define FISHMOD_PROP_KINDS_GENERATED")[0])
    tuned = {}
    th = os.path.join(ROOT, "include/fishmod_lod_tune.h")
    if os.path.exists(th):
        rows = re.findall(r"/\* (\w+) \*/ \{(.*)\},", open(th).read())
        for name, body in rows:
            nums = list(map(int, re.findall(r"\d+", body)))  # [time][under][setting]
            tuned[name.upper()] = min(max(nums[t * 6 + 3] for t in range(4)), 1400)  # (UNDER_CUT_MAX)
    out = {}
    for k in kinds:
        a = 800.0 if k in small else 1200.0 if k in mid else 1600.0
        if k in lod and lod[k][0] > 0:
            a = min(a, lod[k][0])
        u = min(a, 900.0)
        if k in lod and lod[k][1] > 0:
            u = min(u, lod[k][1])
        if tuned.get(k):
            u = tuned[k]
        out[k.lower()] = (a, u)
    return out


def budget(instances):
    gs = load("gen_scatter")
    gw = gs.gw
    d = os.path.join(gw.SCENES, gs.SCENES["SCENE_LAKE_HYLIA"])
    boxes = gw.parse_waterboxes(gw.one(d, "*_WaterBoxes.inc.c"))
    dist = distances()
    view_cos = 0.657  # (fishmod_scatter.c Scatter_ViewCos at 60 degrees)
    rows = []
    for eye, at, title in stops():
        surf = max((b["y"] for b in boxes if b["xMin"] <= eye[0] <= b["xMin"] + b["xLen"] and
                    b["zMin"] <= eye[2] <= b["zMin"] + b["zLen"]), default=-100000)
        under = eye[1] < surf
        lx, ly, lz = (at[i] - eye[i] for i in range(3))
        ll = math.sqrt(lx * lx + ly * ly + lz * lz)
        lx, ly, lz = lx / ll, ly / ll, lz / ll
        near = far = 0
        for p in instances:
            dx, dy, dz = p["x"] - eye[0], p["y"] - eye[1], p["z"] - eye[2]
            dsq = dx * dx + dz * dz
            cut = dist.get(p["kind"], (1600, 900))[1 if under else 0]
            if dsq > cut * cut:
                continue
            front = dx * lx + dy * ly + dz * lz + 90.0
            if (front < 0 or front * front < view_cos * view_cos * (dsq + dy * dy)) and dsq > 180 * 180:
                continue
            if dsq < 500 * 500:
                near += 1
            else:
                far += 1
        rows.append({"stop": title, "under": under, "near": near, "far": far})
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--save")
    ap.add_argument("--compare")
    args = ap.parse_args()
    inst = load("gen_collision").props("spot06")  # (the game's data, as written)
    rows = budget(inst)
    old = {r["stop"]: r for r in json.load(open(args.compare))} if args.compare else {}
    print(f"{len(inst)} props placed")
    print(f"{'stop':34s} {'near':>5s} {'far':>5s}" + ("   (was near/far)" if old else ""))
    for i, r in enumerate(rows):
        o = old.get(r["stop"])
        print(f"{i + 1:2d} {r['stop'][:31]:31s} {r['near']:5d} {r['far']:5d}" +
              (f"   ({o['near']}/{o['far']})" if o else ""))
    tn, tf = sum(r["near"] for r in rows), sum(r["far"] for r in rows)
    print(f"   {'all stops':31s} {tn:5d} {tf:5d}" +
          (f"   ({sum(o['near'] for o in old.values())}/{sum(o['far'] for o in old.values())})" if old else ""))
    if args.save:
        json.dump(rows, open(args.save, "w"), indent=1)


if __name__ == "__main__":
    main()
