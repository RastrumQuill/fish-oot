#!/usr/bin/env python3
"""Estimated console frame time across each Lake Hylia section (the zones of fishmod/data/lake_hylia_zones.json), from
cameras like the game's own, not picked ones:
  fish:  Link on the shore or a dock near the section, the rod's camera (75 behind, 28 aside, 60 up) on a bobber
         300-1100 out in the section (fishmod_rod.c Rod_CameraUpdate)
  swim:  Link swimming at the surface in the section, the follow camera 220 behind and 80 up, looking at him
  dive:  Link under water in the section (half on the bed in iron boots, half swimming), the camera 200 behind
         and 40 up
Each view's RDP time is shot_search.py's model: its no-prop time (the nearest hwbench-4 stop on the same side of the
surface) plus the props' card-scanlines. A frame over 50 ms drops below 20 fps. Each spot is looked at in eight directions (the camera swung round Link, or
the bobber cast all round him), and it counts by its worst one: a spot holds 20 fps only if you can look around.

Usage: section_budget.py [--views 150] [--save out.json] [--png sections.png]
"""
import argparse
import json
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import shot_search as S  # noqa: E402

R = S.R
SURF = S.SURFACE
LIMIT = 50.0


def setting():
    gs, gw = R.gs, R.gs.gw
    d = os.path.join(gw.SCENES, gs.SCENES["SCENE_LAKE_HYLIA"])
    vtx = gw.parse_vtx(gw.one(d, "*_VtxList.inc.c"))
    finder = gw.FloorFinder(gw.parse_floors(gw.one(d, "*_PolyList.inc.c"), vtx))
    boxes = gw.parse_waterboxes(gw.one(d, "*_WaterBoxes.inc.c"))
    zones = json.load(open(os.path.join(R.ROOT, "fishmod/data/lake_hylia_zones.json")))["zones"]
    gsz = gs
    return finder, boxes, zones, gsz


def stop_bases():
    """(eye, under, no-prop ms) for each hwbench-4 stop."""
    hw = R.hw4()
    out = []
    for i, (eye, at, title) in enumerate(R.sb.stops()):
        out.append((eye, "(under)" in title or title in ("Under the lilies", "Water Temple gate"), hw[i + 1]["noprops"]))
    return out


def base_for(bases, eye, under):
    same = [b for b in bases if b[1] == under] or bases
    return min(same, key=lambda b: (b[0][0] - eye[0]) ** 2 + (b[0][2] - eye[2]) ** 2)[2]


def floor(finder, x, z, top=100000):
    return finder.floor(int(x), int(z), top)


def in_water(boxes, x, z):
    return any(b["xMin"] <= x <= b["xMin"] + b["xLen"] and b["zMin"] <= z <= b["zMin"] + b["zLen"] for b in boxes)


def views(zone, finder, boxes, gs, rng, n):
    """Up to n of each kind of camera for the zone: (kind, eye, at, under)."""
    out = []
    tries = 0
    want = {"fish": n, "swim": n, "dive": n}
    while any(v > 0 for v in want.values()) and tries < n * 400:
        tries += 1
        # a point well inside the zone (the drop-off has no shape: wherever the bed falls fast, 100-400 deep)
        sh = zone["shape"]
        if isinstance(sh, str):
            px, pz = rng.uniform(-4800, 3200), rng.uniform(2600, 9800)
        elif sh[0] == "circle":
            a, r = rng.uniform(0, 2 * math.pi), sh[3] * math.sqrt(rng.uniform(0, 1))
            px, pz = sh[1] + math.cos(a) * r, sh[2] + math.sin(a) * r
        else:
            ax, az, bx, bz, w = sh[1:]
            t = rng.uniform(0, 1)
            a = rng.uniform(0, 2 * math.pi)
            r = w * math.sqrt(rng.uniform(0, 1))
            px, pz = ax + (bx - ax) * t + math.cos(a) * r, az + (bz - az) * t + math.sin(a) * r
        if not isinstance(sh, str) and gs.in_zone(zone, px, pz) < 0.25:
            continue
        if not in_water(boxes, px, pz):
            continue
        fl = floor(finder, px, pz, SURF)
        if fl is None or fl > SURF - 10:
            continue
        if isinstance(zone["shape"], str):
            f2 = [floor(finder, px + dx, pz + dz, SURF) for dx, dz in ((40, 0), (-40, 0), (0, 40), (0, -40))]
            f2 = [v for v in f2 if v is not None]
            if not (100 <= SURF - fl <= 400) or not f2 or (max(f2) - min(f2)) / 80 < 0.6:
                continue
        yaw = rng.uniform(0, 2 * math.pi)
        fx, fz = math.cos(yaw), math.sin(yaw)
        kind = max(want, key=lambda k: want[k])
        if kind == "fish":
            # Link on dry ground (or a dock) 300-1100 back from the bobber at (px, pz)
            back = rng.uniform(300, 1100)
            lx, lz = px - fx * back, pz - fz * back
            g = floor(finder, lx, lz)
            if g is None or g < SURF - 2 or g > SURF + 250:
                continue
            ly = g
            eye = (lx - fx * 75 - fz * 28, ly + 60, lz - fz * 75 + fx * 28)
            at = (px, SURF, pz)
            under = False
        elif kind == "swim":
            eye = (px - fx * 220, SURF + 80, pz - fz * 220)
            g = floor(finder, eye[0], eye[2])
            if g is not None and g > eye[1] - 20:
                continue
            at = (px, SURF + 30, pz)
            under = False
        else:
            if SURF - fl < 70:
                continue
            # half with iron boots on the bed, half swimming in mid-water
            ly = fl + 2 if rng.uniform(0, 1) < 0.5 else rng.uniform(fl + 30, SURF - 40)
            eye = (px - fx * 200, min(ly + 40, SURF - 15), pz - fz * 200)
            g = floor(finder, eye[0], eye[2], SURF)
            if g is None or g > eye[1] - 15:
                continue
            at = (px, ly + 20, pz)
            under = True
        want[kind] -= 1
        out.append((kind, eye, at, under))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--views", type=int, default=150, help="per kind of camera per section")
    ap.add_argument("--save")
    ap.add_argument("--seed", type=int, default=7, help="11 or another: spots the thinning never saw")
    a = ap.parse_args()
    finder, boxes, zones, gs = setting()
    scene = S.Scene()
    bases = stop_bases()
    rng = np.random.default_rng(a.seed)
    rows = []
    print("Each spot by its worst direction of eight (looking all the way round):")
    print(f"{'section':38s} {'spots':>5s} {'median':>7s} {'p90':>6s} {'worst':>6s} {'over 50':>8s}  worst kind")
    for z in zones:
        vs = views(z, finder, boxes, gs, rng, a.views)
        res = []
        for kind, eye, at, under in vs:
            # looking all the way round from the same spot: the camera swung about Link (swim, dive) or about Link
            # standing on the shore (fish: the bobber cast in eight directions); the spot holds only if every one does
            pivot = np.array(at, float) if kind != "fish" else np.array(eye, float)
            turns = []
            for deg in range(0, 360, 45):
                a_ = math.radians(deg)
                c_, s_ = math.cos(a_), math.sin(a_)
                def rot(p):
                    d = np.array(p, float) - pivot
                    return pivot + np.array([d[0] * c_ - d[2] * s_, d[1], d[0] * s_ + d[2] * c_])
                e2, a2 = rot(eye), rot(at)
                turns.append(scene.cost(e2, a2, under, base_for(bases, e2, under))[0])
            res.append({"kind": kind, "eye": [round(v) for v in eye], "at": [round(v) for v in at], "under": under,
                        "ms": round(turns[0], 1), "worst": round(max(turns), 1),
                        "turns": [round(t, 1) for t in turns]})
        if not res:
            print(f"{z['id']:2d} {z['name'][:35]:35s}  (no views)")
            continue
        ms = np.array([r["worst"] for r in res])  # (each spot by its worst direction)
        over = (ms > LIMIT).mean()
        worst = max(res, key=lambda r: r["ms"])
        print(f"{z['id']:2d} {z['name'][:35]:35s} {len(res):5d} {np.median(ms):7.1f} {np.percentile(ms, 90):6.1f} "
              f"{ms.max():6.1f} {over:8.0%}  {worst['kind']}")
        rows.append({"zone": z["id"], "name": z["name"], "views": res})
    if a.save:
        json.dump(rows, open(a.save, "w"))


if __name__ == "__main__":
    main()
