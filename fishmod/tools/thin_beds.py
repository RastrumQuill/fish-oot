#!/usr/bin/env python3
"""Thin the plant beds only where the console needs it: every sampled spot (section_budget.py's game-like cameras,
looked at in eight directions) must stay under the frame-time limit by shot_search.py's model. Wherever a
direction is over, the plant costing that view the most is turned into a moss mat (nearly free to draw), so the bed
stays covered and green; where a moss mat is still the costliest, it goes. Repeats until every spot holds. Writes the list of turned plants (by position) to
fishmod/data/lake_hylia_thin.json; gen_scatter.py applies it after placing, so nothing else moves.

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


def restore(sc, dirs, limit, thin, moss, turned_to_moss, dropped):
    """The passes take a plant out of every failing direction at once, so they overshoot. Put back, one step at a time
    (a dropped mat back as moss, a plant made moss back as itself), whatever no direction needs gone: cheapest first."""
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
    plant_shape = {}

    def as_plant(j):
        k = sc.kind[j]
        sc.H[j] = S.R.HEIGHT.get(k, 40) * sc.P[j, 3]
        sc.W[j] = (S.CARDS[k][1] if k in S.CARDS else S.R.HEIGHT.get(k, 40)) * sc.P[j, 3]
        sc.NC[j] = S.CARDS.get(k, (2, 0))[0]
        sc.PLANT[j], sc.FLAT[j] = True, False

    def as_moss(j):
        sc.H[j], sc.W[j], sc.NC[j] = MOSS_H * sc.P[j, 3], MOSS_W * sc.P[j, 3], 1
        sc.PLANT[j], sc.FLAT[j] = False, True

    back = [0, 0]
    for rnd in range(4):  # (what can't come back early may fit once its neighbours have settled)
        cand = [j for j in np.nonzero(turned_to_moss | dropped)[0]]
        trial = []
        for j in cand:
            old = prop_vs_dirs(sc, j, D)
            state = (sc.H[j], sc.W[j], sc.NC[j], sc.PLANT[j], sc.FLAT[j], sc.CUT[j], sc.CUT_ABOVE[j])
            if dropped[j]:
                sc.CUT[j], sc.CUT_ABOVE[j] = sc.cut0[j]
                if not moss[j]:
                    as_moss(j)
            else:
                as_plant(j)
            new = prop_vs_dirs(sc, j, D)
            sc.H[j], sc.W[j], sc.NC[j], sc.PLANT[j], sc.FLAT[j], sc.CUT[j], sc.CUT_ABOVE[j] = state
            trial.append((float((new - old).clip(0).sum()), j))
        trial.sort()
        n0 = back[:]
        for _, j in trial:
            old = prop_vs_dirs(sc, j, D)
            state = (sc.H[j], sc.W[j], sc.NC[j], sc.PLANT[j], sc.FLAT[j], sc.CUT[j], sc.CUT_ABOVE[j])
            was_dropped = dropped[j]
            if was_dropped:
                sc.CUT[j], sc.CUT_ABOVE[j] = sc.cut0[j]
                if not moss[j]:
                    as_moss(j)
            else:
                as_plant(j)
            new = prop_vs_dirs(sc, j, D)
            t = total + new - old
            if (t[new > old] <= limit).all():
                total = t
                if was_dropped:
                    dropped[j] = False
                    back[0] += 1
                else:
                    turned_to_moss[j] = False
                    back[1] += 1
            else:
                sc.H[j], sc.W[j], sc.NC[j], sc.PLANT[j], sc.FLAT[j], sc.CUT[j], sc.CUT_ABOVE[j] = state
        print(f"restore round {rnd + 1}: {back[0] - n0[0]} mats back, {back[1] - n0[1]} plants back")
        if back == n0:
            break
    print(f"restored: {back[0]} mats, {back[1]} plants; directions over {limit} ms: {int((total > limit).sum())}")


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
    moss = np.array([k == "moss_patch" for k in sc.kind])
    turned_to_moss = np.zeros(len(sc.kind), bool)
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
            # the costliest plant becomes moss; a moss mat (placed, or made from a plant) costliest of all goes
            cand = np.where((thin | moss | turned_to_moss) & ~dropped, c, 0)
            j = int(np.argmax(cand))
            if cand[j] > 0:
                pick.add(j)
        print(f"pass {p + 1}: {len(still)} directions over {a.limit} ms; turning {len(pick)} plants to moss")
        if not still or not pick:
            break
        for j in pick:
            if moss[j] or turned_to_moss[j]:
                dropped[j] = True
                sc.CUT[j] = sc.CUT_ABOVE[j] = 0  # (not drawn: no cost at all, not even the per-prop one)
                continue
            turned_to_moss[j] = True
            sc.H[j] = MOSS_H * sc.P[j, 3]
            sc.W[j] = MOSS_W * sc.P[j, 3]
            sc.NC[j] = 1
            sc.PLANT[j] = False
            sc.FLAT[j] = True
        failing = still
    if a.restore:
        restore(sc, dirs, a.restore_limit, thin, moss, turned_to_moss, dropped)
    out = [{"kind": sc.kind[j], "x": int(sc.P[j, 0]), "z": int(sc.P[j, 2]), "drop": bool(dropped[j])}
           for j in np.nonzero(turned_to_moss | dropped)[0]]
    by = {}
    for o in out:
        by[o["kind"]] = by.get(o["kind"], 0) + 1
    print(f"{int((turned_to_moss & ~dropped).sum())} plants to moss of {int(thin.sum())}, "
          f"{int(dropped.sum())} dropped (moss mats, or plants turned to moss and then still too much):", by)
    json.dump({"limit": a.limit, "restore_limit": a.restore_limit, "views": a.views, "seeds": a.seeds, "moss": out}, open(OUT, "w"), indent=0)


if __name__ == "__main__":
    main()
