#!/usr/bin/env python3
"""Thin the plant beds only where the console needs it: every sampled spot (section_budget.py's game-like cameras,
looked at in eight directions) must stay under the frame-time limit by shot_search.py's model. Wherever a
direction is over, the plant costing that view the most is turned into a moss mat (nearly free to draw), so the bed
stays covered and green. Repeats until every spot holds. Writes the list of turned plants (by position) to
fishmod/data/lake_hylia_thin.json; gen_scatter.py applies it after placing, so nothing else moves.

Usage: thin_beds.py [--limit 48] [--views 60] [--seed 7]   (then gen_scatter.py --c)
"""
import argparse
import json
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import section_budget as B  # noqa: E402
import shot_search as S  # noqa: E402

OUT = os.path.join(S.R.ROOT, "fishmod", "data", "lake_hylia_thin.json")
THINNABLE = {"grass_tuft", "broadleaf", "bush_weed", "big_pondweed", "ribbon_weed", "vine_strand", "kelp_frond",
             "tall_weed", "fan_weed"}
MOSS_H, MOSS_W = S.R.HEIGHT.get("moss_patch", 20), S.CARDS["moss_patch"][1]


def turned(v):
    """The eight directions of a spot: (eye, at, under)."""
    pivot = np.array(v["at"] if v["kind"] != "fish" else v["eye"], float)
    out = []
    for deg in range(0, 360, 45):
        a = math.radians(deg)
        c, s = math.cos(a), math.sin(a)

        def rot(p):
            d = np.array(p, float) - pivot
            return pivot + np.array([d[0] * c - d[2] * s, d[1], d[0] * s + d[2] * c])
        out.append((rot(v["eye"]), rot(v["at"]), v["under"]))
    return out


def contributions(sc, eye, at, under):
    """Per prop: its share of the view's RDP time (shot_search.Scene.contrib)."""
    return sc.contrib(eye, at, under)[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--limit", type=float, default=48.0)
    ap.add_argument("--views", type=int, default=60)
    ap.add_argument("--seed", type=int, default=7)
    ap.add_argument("--passes", type=int, default=60)
    a = ap.parse_args()
    if os.path.exists(OUT):
        os.remove(OUT)  # (measured on the placement without it)
        print("note: run gen_scatter.py --c first, so the placement has no thinning applied")
    finder, boxes, zones, gs = B.setting()
    sc = S.Scene()
    bases = B.stop_bases()
    rng = np.random.default_rng(a.seed)
    dirs = []
    for z in zones:
        for kind, eye, at, under in B.views(z, finder, boxes, gs, rng, a.views):
            for e, t, u in turned({"kind": kind, "eye": eye, "at": at, "under": under}):
                dirs.append((e, t, u, B.base_for(bases, e, u), z["id"]))
    print(f"{len(dirs)} view directions")
    thin = np.array([k in THINNABLE for k in sc.kind])
    turned_to_moss = np.zeros(len(sc.kind), bool)
    failing = list(range(len(dirs)))
    for p in range(a.passes):
        still = []
        pick = set()
        for i in failing:
            e, t, u, base, zid = dirs[i]
            c = contributions(sc, e, t, u)
            if base + c.sum() <= a.limit:
                continue
            still.append(i)
            cand = np.where(thin & ~turned_to_moss, c, 0)
            j = int(np.argmax(cand))
            if cand[j] > 0:
                pick.add(j)
        print(f"pass {p + 1}: {len(still)} directions over {a.limit} ms; turning {len(pick)} plants to moss")
        if not still or not pick:
            break
        for j in pick:
            turned_to_moss[j] = True
            sc.H[j] = MOSS_H * sc.P[j, 3]
            sc.W[j] = MOSS_W * sc.P[j, 3]
            sc.NC[j] = 1
            sc.PLANT[j] = False
            sc.FLAT[j] = True
        failing = still
    out = [{"kind": sc.kind[j], "x": int(sc.P[j, 0]), "z": int(sc.P[j, 2])} for j in np.nonzero(turned_to_moss)[0]]
    by = {}
    for o in out:
        by[o["kind"]] = by.get(o["kind"], 0) + 1
    print(f"{len(out)} plants to moss of {int(thin.sum())} thinnable:", by)
    json.dump({"limit": a.limit, "views": a.views, "seed": a.seed, "moss": out}, open(OUT, "w"), indent=0)


if __name__ == "__main__":
    main()
