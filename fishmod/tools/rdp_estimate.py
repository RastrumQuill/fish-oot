#!/usr/bin/env python3
"""Estimated console frame time per tour stop for a placement, scaled from hardware bench 4's measurements.

Each stop's measured prop cost (hwbench-4, GCC save: near = base - nonear, far = base - nofar) is scaled by how much
this placement draws there against what bench 4 drew there (the same view, the same selection rules):
  near props (within 500): cost follows covered pixels, proxy sum((height*scale/dist)^2)
  far props: cost follows triangles and short spans, proxy sum(height*scale/dist) (scanlines touched)
The stop's no-prop time is kept as measured. Far props now have cheaper tiers (single card, half texture) that bench 4
didn't, so the far estimate is an upper bound.

Usage: rdp_estimate.py [--placement src/fishmod_data/fishmod_sc_spot06.c] [--ref REF_sc.c] [--hw hw4.json]
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


sb = load("scatter_budget")
gs = sb.load("gen_scatter")
KINDS = re.findall(r"X\(\w+, (\w+)\)", open(os.path.join(ROOT, "src/fishmod_data/fishmod_props_kinds.h")).read()
                   .split("#define FISHMOD_PROP_KINDS_GENERATED")[0])
HEIGHT = dict(gs.HEIGHT)
# flat kinds: about how tall they look from the low under-water camera (a dome or decal seen at a slant)
HEIGHT.update({"moss_patch": 20, "pebble_patch": 20, "leaf_litter": 12})
DEFAULT_H = 40


def read_placement(path):
    out = []
    for m in re.findall(r"\{ (-?\d+), (-?\d+), (-?\d+), (\d+), (\d+), (\d+), (\d+), (-?\d+), (-?\d+) \}",
                        open(path).read()):
        x, y, z, k, yaw, sc, tint, tx, tz = map(int, m)
        out.append({"x": x, "y": y, "z": z, "kind": KINDS[k], "scale": sc / 64.0})
    return out


def hw4():
    """Per stop: base, nonear, nofar, noprops RDP ms from the GCC save in docs/hwbench-4.md."""
    t = open(os.path.join(ROOT, "fishmod/docs/hwbench-4.md")).read()
    g = t.split("## GCC")[1].split("\n## ")[0]
    rows = {}
    for line in g.splitlines():
        c = [x.strip() for x in line.split("|")]
        if len(c) < 13 or not c[1] or not c[1][0].isdigit():
            continue
        rows.setdefault(int(c[1].split()[0]), {})[c[2]] = float(c[7])
    return rows


def proxies(instances, dist, stop_views):
    """Per stop: (near pixel proxy, far span proxy) of what the selection draws."""
    gw = gs.gw
    d = os.path.join(gw.SCENES, gs.SCENES["SCENE_LAKE_HYLIA"])
    boxes = gw.parse_waterboxes(gw.one(d, "*_WaterBoxes.inc.c"))
    view_cos = 0.657
    out = []
    for eye, at, title in stop_views:
        surf = max((b["y"] for b in boxes if b["xMin"] <= eye[0] <= b["xMin"] + b["xLen"] and
                    b["zMin"] <= eye[2] <= b["zMin"] + b["zLen"]), default=-100000)
        under = eye[1] < surf
        lx, ly, lz = (at[i] - eye[i] for i in range(3))
        ll = math.sqrt(lx * lx + ly * ly + lz * lz)
        lx, ly, lz = lx / ll, ly / ll, lz / ll
        pn = pf = 0.0
        for p in instances:
            dx, dy, dz = p["x"] - eye[0], p["y"] - eye[1], p["z"] - eye[2]
            dsq = dx * dx + dz * dz
            cut = dist.get(p["kind"], (1600, 900))[1 if under else 0]
            if dsq > cut * cut:
                continue
            front = dx * lx + dy * ly + dz * lz + 90.0
            if (front < 0 or front * front < view_cos * view_cos * (dsq + dy * dy)) and dsq > 180 * 180:
                continue
            r = max(math.sqrt(dsq + dy * dy), 60.0)
            h = HEIGHT.get(p["kind"], DEFAULT_H) * p["scale"]
            if dsq < 500 * 500:
                pn += (h / r) ** 2
            else:
                pf += h / r
        out.append((pn, pf, under, title))
    return out


def bench4_distances():
    """The selection as bench 4 drew it: no tuned under-water table."""
    th = os.path.join(ROOT, "include/fishmod_lod_tune.h")
    hidden = th + ".off"
    os.rename(th, hidden)
    try:
        return sb.distances()
    finally:
        os.rename(hidden, th)


def estimate(placement, ref):
    views = sb.stops()
    new = proxies(read_placement(placement), sb.distances(), views)
    old = proxies(read_placement(ref), bench4_distances(), views)
    hw = hw4()
    rows = []
    for i, ((nn, nf, under, title), (on, of, _, _)) in enumerate(zip(new, old)):
        m = hw[i + 1]
        near_ms = max(m["base"] - m["nonear"], 0.0)
        far_ms = max(m["base"] - m["nofar"], 0.0)
        props_ms = max(m["base"] - m["noprops"], 0.0)
        # the split's near and far don't add up to the whole (they overlap); share the whole by the split
        tot = near_ms + far_ms
        wn, wf = (near_ms / tot, far_ms / tot) if tot > 0 else (0.5, 0.5)
        sn = nn / on if on > 0 else (1.0 if nn == 0 else 2.0)
        sf = nf / of if of > 0 else (1.0 if nf == 0 else 2.0)
        est = m["noprops"] + props_ms * (wn * sn + wf * sf)
        rows.append({"stop": i + 1, "title": title, "under": under, "measured": m["base"], "noprops": m["noprops"],
                     "est": est, "near_x": sn, "far_x": sf})
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--placement", default=os.path.join(ROOT, "src/fishmod_data/fishmod_sc_spot06.c"))
    ap.add_argument("--ref", required=True, help="bench 4's placement (gen_scatter.py --c at 5eed119)")
    ap.add_argument("--save")
    a = ap.parse_args()
    rows = estimate(a.placement, a.ref)
    print(f"{'stop':36s} {'bench4':>6s} {'est':>6s} {'fps':>5s}  near x  far x")
    for r in rows:
        fps = min(20.0, 1000.0 / max(r['est'], 50.0))
        print(f"{r['stop']:2d} {r['title'][:33]:33s} {r['measured']:6.1f} {r['est']:6.1f} {fps:5.1f}  "
              f"{r['near_x']:5.2f}  {r['far_x']:5.2f}")
    if a.save:
        json.dump(rows, open(a.save, "w"), indent=1)


if __name__ == "__main__":
    main()
