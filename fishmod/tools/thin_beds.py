#!/usr/bin/env python3
"""Thin the plant beds only where the console needs it: every sampled spot (section_budget.py's game-like cameras,
looked at in eight directions) must stay under the frame-time limit by shot_search.py's model. Wherever a
direction is over, the plant costing that view the most goes; repeats until every spot holds, then puts back, one at a
time, whatever no view needs gone. Writes the list of plants that go (by position) to fishmod/data/lake_hylia_thin.json;
gen_scatter.py applies it after placing, so nothing else moves.

Usage: thin_beds.py [--limit 48] [--views 150] [--seeds 7,19,23]   (then gen_scatter.py --c; check with
       section_budget.py --seed 11, spots it never saw)
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


def prop_vs_dirs(sc, j, D):
    """Prop j's share of each direction's RDP time (the same sums as shot_search.Scene.contrib, for one prop over
    many directions)."""
    E, FW, RT, UP, U = D["eye"], D["fw"], D["rt"], D["up"], D["under"]
    d = sc.P[j, :3][None, :] - E
    dsq = d[:, 0] ** 2 + d[:, 2] ** 2
    ok = dsq < np.where(U, sc.CUT[j], sc.CUT_ABOVE[j]) ** 2
    if not ok.any():
        return np.zeros(len(E))
    H, W, flat = sc.H[j], sc.W[j], sc.FLAT[j]
    c = d.copy()
    if not flat:
        c[:, 1] += H / 2
    z, x, y = (c * FW).sum(1), (c * RT).sum(1), (c * UP).sum(1)
    ok &= z > -W
    zc = np.maximum(z, 15.0)
    x0, x1 = (x - W / 2) * S.F / zc + 160, (x + W / 2) * S.F / zc + 160
    if flat:
        fh = FW * np.array([1.0, 0.0, 1.0])
        fh /= np.maximum(np.linalg.norm(fh, axis=1), 1e-6)[:, None]
        near_pt, far_pt = c - (W / 2) * fh, c + (W / 2) * fh
        zn = np.maximum((near_pt * FW).sum(1), 15.0)
        zf = np.maximum((far_pt * FW).sum(1), 15.0)
        yn, yf = (near_pt * UP).sum(1) * S.F / zn + 120, (far_pt * UP).sum(1) * S.F / zf + 120
        y0, y1 = np.minimum(yn, yf), np.maximum(yn, yf)
    else:
        y0, y1 = (y - H / 2) * S.F / zc + 120, (y + H / 2) * S.F / zc + 120
    cx = np.clip(x1, 0, 320) - np.clip(x0, 0, 320)
    cy = np.clip(y1, 0, 240) - np.clip(y0, 0, 240)
    ok &= (cx > 0) & (cy > 0)
    near = dsq < 500 ** 2
    return np.where(ok, cy * sc.NC[j] * np.where(near, S.KN, S.KF) + np.where(near, S.PN, S.PF), 0.0)


def restore(sc, dirs, limit, dropped):
    """The passes take a plant out of every failing direction at once, so they overshoot. Put back, one at a time,
    whatever no direction needs gone: cheapest first."""
    E = np.array([d[0] for d in dirs], float)
    T = np.array([d[1] for d in dirs], float)
    FW = T - E
    FW /= np.linalg.norm(FW, axis=1)[:, None]
    RT = np.cross(FW, [0, 1, 0])
    RT /= np.linalg.norm(RT, axis=1)[:, None]
    D = {"eye": E, "fw": FW, "rt": RT, "up": np.cross(RT, FW), "under": np.array([d[2] for d in dirs])}
    total = np.array([d[3] for d in dirs], float)
    for j in range(len(sc.kind)):
        total += prop_vs_dirs(sc, j, D)

    def cost_back(j):
        """(its share of each direction if put back, as it is now)"""
        sc.CUT[j], sc.CUT_ABOVE[j] = sc.cut0[j]
        new = prop_vs_dirs(sc, j, D)
        sc.CUT[j] = sc.CUT_ABOVE[j] = 0
        return new

    back = 0
    for rnd in range(4):  # (what can't come back early may fit once its neighbours have settled)
        trial = sorted((float(cost_back(j).sum()), j) for j in np.nonzero(dropped)[0])
        n0 = back
        for _, j in trial:
            new = cost_back(j)
            t = total + new
            if (t[new > 0] <= limit).all():
                total = t
                dropped[j] = False
                sc.CUT[j], sc.CUT_ABOVE[j] = sc.cut0[j]
                back += 1
        print(f"restore round {rnd + 1}: {back - n0} plants back")
        if back == n0:
            break
    print(f"restored: {back} plants; directions over {limit} ms: {int((total > limit).sum())}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--limit", type=float, default=48.0)
    ap.add_argument("--views", type=int, default=60)
    ap.add_argument("--seeds", default="7,19,23", help="sets of spots (section_budget.py --seed 11 is kept for checking)")
    ap.add_argument("--passes", type=int, default=60)
    ap.add_argument("--no-restore", dest="restore", action="store_false",
                    help="skip putting back what the passes took and no direction needs gone")
    ap.add_argument("--restore-limit", type=float, default=46.0,
                    help="what's put back keeps every direction under this (a margin for spots it never saw)")
    a = ap.parse_args()
    if os.path.exists(OUT):
        os.remove(OUT)  # (measured on the placement without it)
        print("note: run gen_scatter.py --c first, so the placement has no thinning applied")
    finder, boxes, zones, gs = B.setting()
    sc = S.Scene()
    sc.cut0 = list(zip(sc.CUT.copy(), sc.CUT_ABOVE.copy()))
    bases = B.stop_bases()
    dirs = []
    for seed in (int(x) for x in a.seeds.split(",")):
      rng = np.random.default_rng(seed)
      for z in zones:
        for kind, eye, at, under in B.views(z, finder, boxes, gs, rng, a.views):
            for e, t, u in turned({"kind": kind, "eye": eye, "at": at, "under": under}):
                dirs.append((e, t, u, B.base_for(bases, e, u), z["id"]))
    print(f"{len(dirs)} view directions")
    thin = np.array([k in THINNABLE for k in sc.kind])
    dropped = np.zeros(len(sc.kind), bool)
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
            # the costliest plant in the view goes
            cand = np.where(thin & ~dropped, c, 0)
            j = int(np.argmax(cand))
            if cand[j] > 0:
                pick.add(j)
        print(f"pass {p + 1}: {len(still)} directions over {a.limit} ms; dropping {len(pick)} plants")
        if not still or not pick:
            break
        for j in pick:
            dropped[j] = True
            sc.CUT[j] = sc.CUT_ABOVE[j] = 0  # (not drawn: no cost at all, not even the per-prop one)
        failing = still
    if a.restore:
        restore(sc, dirs, a.restore_limit, dropped)
    out = [{"kind": sc.kind[j], "x": int(sc.P[j, 0]), "z": int(sc.P[j, 2])} for j in np.nonzero(dropped)[0]]
    by = {}
    for o in out:
        by[o["kind"]] = by.get(o["kind"], 0) + 1
    print(f"{int(dropped.sum())} plants of {int(thin.sum())} go:", by)
    json.dump({"limit": a.limit, "restore_limit": a.restore_limit, "views": a.views, "seeds": a.seeds, "drop": out},
              open(OUT, "w"), indent=0)


if __name__ == "__main__":
    main()
